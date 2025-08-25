import subprocess
import sys
import os
import json
import time

opimization_enabled = False
source_debuging_enabled = False


n = len(sys.argv)
if(n > 3):
   raise Exception("too many arguments")

if(n > 1):
   opimization_enabled = int(sys.argv[1]) == 1
if(n > 2):
   source_debuging_enabled = int(sys.argv[2]) == 1

current_directory = os.path.dirname(os.path.abspath(__file__))
src_folder = os.path.join(os.path.dirname(current_directory), "shaders")
out_folder = os.path.join(os.path.dirname(current_directory), "compiled")
scripts_folder = os.path.join(os.path.dirname(current_directory), "scripts")

#assums this python file is in a folder called scripts
if(current_directory != scripts_folder):
   raise Exception("Invalid folder layout in shader workspace")

if not os.path.exists(src_folder):
   raise Exception('Expected folder "shaders"')

mod_time_file = os.path.join(scripts_folder, "mod_times.json")


changedShaders = set()
allShaders = set()

# Define the file extensions to monitor
extensions = ['.frag', '.vert', '.comp']

outstructure = json.loads("{}")

# Create the mod time file if it doesn't exist
if not os.path.exists(mod_time_file):
   for filename in os.listdir(src_folder):
      if any(filename.endswith(ext) for ext in extensions):
         basename = os.path.basename(filename)
         allShaders.add(basename)
         changedShaders.add(basename)
else:
   try:
      with open(mod_time_file) as f:
            j = json.load(f)
            outstructure = j
      for filename in os.listdir(src_folder):
         if any(filename.endswith(ext) for ext in extensions):
            basename = os.path.basename(filename)
            mod_time = os.path.getmtime(os.path.join(src_folder, filename))
            allShaders.add(basename)
            if(basename not in j or j[basename]["mod_time"] != mod_time or j[basename]["optimized"] != opimization_enabled or j[basename]["debug"] != source_debuging_enabled):
               changedShaders.add(basename)
   except:
      #json was probably edited or inavlid
      for filename in os.listdir(src_folder):
         if any(filename.endswith(ext) for ext in extensions):
            basename = os.path.basename(filename)
            allShaders.add(basename)
            changedShaders.add(basename)
      outstructure = json.loads("{}")


# create compiled folder to put spv files if it doesn't exist
if not os.path.exists(out_folder):
   os.mkdir(out_folder) 
   # assume all files invalid
   changedShaders = allShaders

# check if the number of shaders and compiled shaders is the same
compiledFileCount = 0
for file in os.listdir(out_folder):
   if file.endswith(".spv"):
      compiledFileCount += 1
if compiledFileCount < len(allShaders):
   # new shader or deleted spv file. assume all files invalid
   changedShaders = allShaders
elif compiledFileCount > len(allShaders):
   # shader file could have been deleted. Clear compiled folder an rebuild
   print("rebuilding all shaders...")
   for file in os.listdir(out_folder):
      if file.endswith(".spv"):
         file_path = os.path.join(out_folder, file)
         if os.path.isfile(file_path):
            os.remove(file_path)
   changedShaders = allShaders


if(len(changedShaders) == 0):
   print("nothing to compile")
   exit(0)

frag_files = set()
vert_files = set()
comp_files = set()

# Scan directory for .frag and .vert files
for filename in os.listdir(src_folder):
   if filename.endswith('.frag'):
      frag_files.add(filename[:-5])
   elif filename.endswith('.vert'):
      vert_files.add(filename[:-5])
   elif filename.endswith('.comp'):
      comp_files.add(filename[:-5])

# Group files
paired_files = frag_files & vert_files
unpaired_files = (frag_files | vert_files) - paired_files

if unpaired_files:
   print(f"Error: Found unpaired files with extensions '.frag' or '.vert': {', '.join(unpaired_files)}")
   exit(1)

compilerPath = os.getenv('VULKAN_SDK') + "/Bin/glslc.exe"

errors = False

options = ""
if opimization_enabled:
   options += " -O "

if source_debuging_enabled:
   options += " -g "
options += " -o "

str = "Building "
if(opimization_enabled):
   str += "optimized "
str += "shaders"
if(source_debuging_enabled):
   str += " with source debugging enabled"
str += "\n"
print(str);

for base_name in paired_files:
   vert_file = os.path.join(src_folder, base_name + '.vert')
   frag_file = os.path.join(src_folder, base_name + '.frag')
   vchanged = (base_name + '.vert') in changedShaders
   fchanged = (base_name + '.frag') in changedShaders
   if(fchanged or vchanged):
      print(f"compiling: {base_name}")
   if(vchanged):
      cmd = compilerPath + " " + vert_file + options + os.path.join(out_folder, base_name + '_vert.spv')
      result = subprocess.run(cmd, capture_output=True, text=True, shell=True)
      if result.returncode != 0:
         errors = True
         print(result.stderr)
      else:
         cur_time =  os.path.getmtime(vert_file)
         outstructure[(base_name + '.vert')] = {"mod_time": cur_time, "optimized": opimization_enabled, "debug": source_debuging_enabled}
   if(fchanged):
      cmd = compilerPath + " " + frag_file + options + os.path.join(out_folder, base_name + '_frag.spv')
      result = subprocess.run(cmd, capture_output=True, text=True, shell=True)
      if result.returncode != 0:
         errors = True
         print(result.stderr)
      else:
         cur_time =  os.path.getmtime(frag_file)
         outstructure[(base_name + '.frag')] = {"mod_time": cur_time, "optimized": opimization_enabled, "debug": source_debuging_enabled}
   
for base_name in comp_files:
   comp_file = os.path.join(src_folder, base_name + '.comp')
   changed = (base_name + '.comp') in changedShaders
   if(changed):
      print(f"compiling: {base_name}")
      cmd = compilerPath + " " + comp_file + options + os.path.join(out_folder, base_name + '_comp.spv')
      result = subprocess.run(cmd, capture_output=True, text=True, shell=True)
      if result.returncode != 0:
         errors = True
         print(result.stderr)
      else: 
         cur_time =  os.path.getmtime(comp_file)
         outstructure[(base_name + '.comp')] = {"mod_time": cur_time, "optimized": opimization_enabled, "debug": source_debuging_enabled}

if(errors):
   print("\ncompleted with some errors")
else:
   print("\nnice :)")

with open(mod_time_file, 'w') as f:
   json.dump(outstructure, f, indent=4)

exit(0)