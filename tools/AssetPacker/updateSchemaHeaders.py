import shutil
import subprocess
import os
import json
import flatbuffers

# Summary: 
# This script will generate the flatbuffer headers for all the schemas in the schemas directory and move them to the engine assetPack folder


current_directory = os.path.dirname(os.path.abspath(__file__))
projDir = root_directory =os.path.dirname(os.path.dirname(current_directory))
flatcPath = os.path.join(current_directory, "flatc")
schemasDir = os.path.join(current_directory, "schemas")
destinationDir = os.path.join(os.path.join(projDir, "MyEngine"), "assetPack")

command = ["flatc", "--cpp"]


for root, dirs, files in os.walk(schemasDir):
   for file in files:
      if file.endswith(".fbs"):
         schema_path = os.path.join(root, file)
         command.append(schema_path)


command.append("--no-warnings")

subprocess.check_call(command)  

generatedFiles = []
for filename in os.listdir(current_directory):
   if(filename.endswith(".h")):
      print("generated {0}".format(filename))
      generatedFiles.append(filename)

# move all the generated files into the destination directory
for file in generatedFiles:
   shutil.move(file, os.path.join(destinationDir, file))

print("moved generated files to {0}".format(destinationDir))

