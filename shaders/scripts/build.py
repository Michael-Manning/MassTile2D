import subprocess
import sys
import os

# checks if any relevant files have been modified since the last time the script was run
def checkMod(srcDirectory, mod_time_file):

   changedFiles = set()

   # Define the file extensions to monitor
   extensions = ['.frag', '.vert']

   # Create the mod time file if it doesn't exist
   if not os.path.exists(mod_time_file):
      open(mod_time_file, 'w').close()

   # Read the existing modification times from the file
   with open(mod_time_file, 'r') as f:
      mod_times = {}
      for line in f:
         path, mod_time = line.strip().split(':')
         mod_times[path] = float(mod_time)

   filesChanged = False

   # Check for changes in files with the specified extensions
   for root, dirs, files in os.walk(srcDirectory):
      for filename in files:
         if any(filename.endswith(ext) for ext in extensions):
               path = os.path.join(root, filename)
               mod_time = os.path.getmtime(path)
               if mod_times.get(path) is None or mod_times.get(path) != mod_time:
                  # print(f'File changed: {path}')
                  # changedFiles.add(path)
                  basename = os.path.basename(path)
                  changedFiles.add(basename)
                  filesChanged = True
               mod_times[path] = mod_time

   # Write the updated modification times to the file
   with open(mod_time_file, 'w') as f:
      for path, mod_time in mod_times.items():
         f.write(f'{path}:{mod_time}\n')

   return changedFiles



src_folder = "../shaders"
out_folder = "../shaders/compiled"

filesChanged = checkMod(src_folder, "scripts/mod_times.txt")
if(len(filesChanged) == 0):
   print("nothing to compile")
   exit(0)

frag_files = set()
vert_files = set()

# Scan directory for .frag and .vert files
for filename in os.listdir(src_folder):
   if filename.endswith('.frag'):
      frag_files.add(filename[:-5])
   elif filename.endswith('.vert'):
      vert_files.add(filename[:-5])

# Group files
paired_files = frag_files & vert_files
unpaired_files = (frag_files | vert_files) - paired_files

if unpaired_files:
   print(f"Error: Found unpaired files with extensions '.frag' or '.vert': {', '.join(unpaired_files)}")
   exit(1)

compilerPath = os.getenv('VULKAN_SDK') + "/Bin/glslc.exe"

errors = False


for base_name in paired_files:
   frag_file = os.path.join(src_folder, base_name + '.frag')
   vert_file = os.path.join(src_folder, base_name + '.vert')
   fchanged = (base_name + '.frag') in filesChanged
   vchanged = (base_name + '.vert') in filesChanged
   if(fchanged or vchanged):
      print(f"compiling: {base_name}")
   if(fchanged):
      cmd = compilerPath + " " + frag_file + " -o " + os.path.join(out_folder, base_name + '_frag.spv')
      result = subprocess.run(cmd, capture_output=True, text=True, shell=True)
      if result.returncode != 0:
         errors = True
         print(result.stderr)
   if(vchanged):
      cmd = compilerPath + " " + vert_file + " -o " + os.path.join(out_folder, base_name + '_vert.spv')
      result = subprocess.run(cmd, capture_output=True, text=True, shell=True)
      if result.returncode != 0:
         errors = True
         print(result.stderr)
   

if(errors):
   print("\ncompleted with some errors")
else:
   print("\nnice :)")

exit(0)