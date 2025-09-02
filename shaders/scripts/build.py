import subprocess
import sys
import os
import json

opimization_enabled = False
source_debuging_enabled = False

n = len(sys.argv)
if n > 3:
    raise Exception("too many arguments")

if n > 1:
    opimization_enabled = int(sys.argv[1]) == 1
if n > 2:
    source_debuging_enabled = int(sys.argv[2]) == 1

if not (opimization_enabled or source_debuging_enabled):
    raise Exception("no build option selected; enable optimization and/or source debugging")

current_directory = os.path.dirname(os.path.abspath(__file__))
root_directory = os.path.dirname(current_directory)
src_folder = os.path.join(root_directory, "source")
out_folder = os.path.join(root_directory, "compiled")
scripts_folder = os.path.join(root_directory, "scripts")
reflectToolPath = os.path.join(os.path.dirname(root_directory), "tools/ShaderReflector/x64/Debug/ShaderReflector.exe")

# assumes this python file is in a folder called scripts
if current_directory != scripts_folder:
    raise Exception("Invalid folder layout in shader workspace")

if not os.path.exists(src_folder):
    raise Exception('Expected folder "source"')

mod_time_file = os.path.join(scripts_folder, "mod_times.json")

# flavors to build
flavors = []
if opimization_enabled:
    flavors.append({"name": "optimized", "opts": ["-O"]})
if source_debuging_enabled:
    flavors.append({"name": "debug", "opts": ["-g"]})

# ensure output root and flavor subfolders exist
os.makedirs(out_folder, exist_ok=True)
for f in flavors:
    os.makedirs(os.path.join(out_folder, f["name"]), exist_ok=True)

# track shader files
extensions = ['.frag', '.vert', '.comp']
changedShaders = set()
allShaders = set()

# load or init mod times
mod_times = {}
if os.path.exists(mod_time_file):
    try:
        with open(mod_time_file) as f:
            mod_times = json.load(f)
    except Exception:
        mod_times = {}

# scan sources and detect changes
for filename in os.listdir(src_folder):
    if any(filename.endswith(ext) for ext in extensions):
        basename = os.path.basename(filename)
        allShaders.add(basename)
        fullpath = os.path.join(src_folder, filename)
        mt = os.path.getmtime(fullpath)
        if basename not in mod_times or mod_times[basename] != mt:
            changedShaders.add(basename)

# classify by type
frag_files = {f[:-5] for f in os.listdir(src_folder) if f.endswith('.frag')}
vert_files = {f[:-5] for f in os.listdir(src_folder) if f.endswith('.vert')}
comp_files = {f[:-5] for f in os.listdir(src_folder) if f.endswith('.comp')}

paired_files = frag_files & vert_files
unpaired_files = (frag_files | vert_files) - paired_files
if unpaired_files:
    print(f"Error: Found unpaired files with extensions '.frag' or '.vert': {', '.join(sorted(unpaired_files))}")
    sys.exit(1)

# expected .spv names (per flavor)
expected_spv = set()
for base in paired_files:
    expected_spv.add(f"{base}_vert.spv")
    expected_spv.add(f"{base}_frag.spv")
for base in comp_files:
    expected_spv.add(f"{base}_comp.spv")

# clean stale outputs in each flavor folder
for f in flavors:
    fdir = os.path.join(out_folder, f["name"])
    for file in os.listdir(fdir):
        if file.endswith(".spv") and file not in expected_spv:
            try:
                os.remove(os.path.join(fdir, file))
            except FileNotFoundError:
                pass

# compiler path
sdk = os.getenv('VULKAN_SDK')
if not sdk:
    raise Exception("VULKAN_SDK environment variable not set")
if sys.platform.startswith("win"):
    compilerPath = os.path.join(sdk, "Bin", "glslc.exe")
else:
    compilerPath = os.path.join(sdk, "bin", "glslc")

if not os.path.exists(compilerPath):
    raise Exception(f"glslc not found at: {compilerPath}")

errors = False

def run_compile(src_path, out_path, opts):
    args = [compilerPath, src_path] + opts + ["-o", out_path]
    result = subprocess.run(args, capture_output=True, text=True)
    if result.returncode != 0:
        print(result.stderr)
        return False
    return True

# track first-flavor failures per shader file to skip later flavors
failed_first_flavor = set()  # keys like "name.vert", "name.frag", "name.comp"

# build message
msg_parts = []
if opimization_enabled:
    msg_parts.append("optimized")
if source_debuging_enabled:
    msg_parts.append("debug")
print("Building " + " and ".join(msg_parts) + " shaders\n")

# compile paired vert/frag
for base_name in sorted(paired_files):
    vert_file = os.path.join(src_folder, base_name + '.vert')
    frag_file = os.path.join(src_folder, base_name + '.frag')

    v_key = base_name + '.vert'
    f_key = base_name + '.frag'
    v_changed = v_key in changedShaders
    f_changed = f_key in changedShaders

    for fl in flavors:
        fdir = os.path.join(out_folder, fl["name"])
        v_out = os.path.join(fdir, base_name + '_vert.spv')
        f_out = os.path.join(fdir, base_name + '_frag.spv')

        # decide if we need to build each stage for this flavor
        need_v = (v_changed or not os.path.exists(v_out)) and (v_key not in failed_first_flavor)
        need_f = (f_changed or not os.path.exists(f_out)) and (f_key not in failed_first_flavor)

        if need_v or need_f:
            print(f"compiling: {base_name} [{fl['name']}]")

        if need_v:
            ok = run_compile(vert_file, v_out, fl["opts"])
            if not ok:
                errors = True
                # mark so later flavors are skipped for this shader file
                failed_first_flavor.add(v_key)
            else:
                mod_times[v_key] = os.path.getmtime(vert_file)

        if need_f:
            ok = run_compile(frag_file, f_out, fl["opts"])
            if not ok:
                errors = True
                failed_first_flavor.add(f_key)
            else:
                mod_times[f_key] = os.path.getmtime(frag_file)

# compile compute
for base_name in sorted(comp_files):
    comp_file = os.path.join(src_folder, base_name + '.comp')
    c_key = base_name + '.comp'
    c_changed = c_key in changedShaders

    for fl in flavors:
        fdir = os.path.join(out_folder, fl["name"])
        c_out = os.path.join(fdir, base_name + '_comp.spv')

        need_c = (c_changed or not os.path.exists(c_out)) and (c_key not in failed_first_flavor)

        if need_c:
            print(f"compiling: {base_name} [{fl['name']}]")
            ok = run_compile(comp_file, c_out, fl["opts"])
            if not ok:
                errors = True
                failed_first_flavor.add(c_key)
            else:
                mod_times[c_key] = os.path.getmtime(comp_file)

if errors:
    print("\ncompleted with some errors")
else:

    print("\nRunning reflection tool\n")
    args = [reflectToolPath]
    result = subprocess.run(args, capture_output=True, text=True)
    if result.returncode != 0:
        print(result.stderr)
    else:
        print("Done :)")

# persist mod times (per source file only)
with open(mod_time_file, 'w') as f:
    json.dump(mod_times, f, indent=4)

sys.exit(0)
