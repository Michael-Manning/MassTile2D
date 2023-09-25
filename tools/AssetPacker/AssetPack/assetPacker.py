import subprocess
import os
import json
import flatbuffers


builder = flatbuffers.Builder(1024)

packagedFileName = "Assets.bin"

assetSrcDir = "../../data/Assets/";
shadersSrcDir = "../../shaders/compiled/";
flatcPath = "flatc"

def getFlatBufferFromJson(json_path, schema_path):
   # Derive the binary file path based on the json_path
   bin_name = os.path.splitext(os.path.basename(json_path))[0] + ".bin"
   
   # Call flatc to generate the binary file
   command = ["flatc", "-b", schema_path, json_path]
   subprocess.check_call(command)

   # Read the binary file
   with open(bin_name, 'rb') as f:
      buf = bytearray(f.read())

   # Cleanup: remove the binary file
   os.remove(bin_name)

   return buf                                                                                 

def jsonToFlatbuffer(json_path, schema_path):
   # Derive the binary file path based on the json_path
   bin_name = os.path.splitext(os.path.basename(json_path))[0] + ".bin"
   
   # Call flatc to generate the binary file
   command = ["flatc", "-b", schema_path, json_path]
   subprocess.check_call(command)  

def getJson(filepath):
   with open(filepath, 'r') as file:
      return json.loads(file.read())

def combineFiles (output_filepath, filepaths):
   lengths = []
   with open(output_filepath, 'wb') as output_file:
      # Iterate through each file path in the list
      for filepath in filepaths:
         # Open each file in binary read mode
         with open(filepath, 'rb') as input_file:
               # Read the raw binary data from the file
               data = input_file.read()

               lengths.append(len(data))

               # Write the binary data to the destination file
               output_file.write(data)
               # Optionally, you can print the progress
               print(f'Appended data from {filepath} to {output_filepath}')
   return lengths;

# Create a list of all files in the asset source directory
jsonFiles = []
for root, dirs, files in os.walk(assetSrcDir):
   for file in files:
      jsonFiles.append(os.path.join(root, file))



spriteNames = []
spriteIDs = []
sprites = []
fontNames = []
fontIDs = []
fonts = []
prefabNames = []
prefabs = []
sceneNames = []
scenes = []
resourceFileNames = []
rourceFileSizes = []

resourceFilePaths = []

for file in jsonFiles:
    # Get the file extension
   fileExt = os.path.splitext(file)[1]

   if fileExt == ".sprite":
      spr = getJson(file)
      spriteNames.append(spr["name"])
      spriteIDs.append(spr["ID"])
      sprites.append(spr)
   if fileExt == ".fjson": 
      fnt = getJson(file)
      fontNames.append(fnt["name"])
      fontIDs.append(fnt["ID"])
      fonts.append(fnt)
   if fileExt == ".prefab": 
      pfb = getJson(file)
      prefabNames.append(pfb["name"])
      prefabs.append(pfb)
   if fileExt == ".scene": 
      scn = getJson(file)
      sceneNames.append(scn["name"])
      scenes.append(scn)
   else:
      resourceFileNames.append(os.path.basename(file))
      resourceFilePaths.append(file)

# add shaders as assets
for root, dirs, files in os.walk(shadersSrcDir):
   for file in files:
      resourceFileNames.append(os.path.basename(file))
      resourceFilePaths.append(os.path.join(root, file))

combinedResourceFileName = "resources.bin"
rourceFileSizes = combineFiles(combinedResourceFileName, resourceFilePaths)

PackageLayoutJson = {
   "spriteNames": spriteNames,
   "spriteIDs": spriteIDs,
   "fontNames": fontNames,
   "fontIDs": fontIDs,
   "prefabNames": prefabNames,
   "sceneNames" : sceneNames,
   "resourceFileNames": resourceFileNames,
   "resourceFileSizes": rourceFileSizes,
}

PackageAssetsJson = {
   "sprites": sprites,
   "fonts": fonts,
   "prefabs": prefabs,
   "scenes": scenes
}

HeaderFileName = "tempA"
LayoutFileName = "tempB"
AssetsFileName = "tempC"

with open(LayoutFileName + ".json", 'w') as json_file:
    json.dump(PackageLayoutJson, json_file, indent=3)
jsonToFlatbuffer(LayoutFileName + ".json", "schemas/PackageLayout.fbs")
os.remove(LayoutFileName + ".json")

with open(AssetsFileName + ".json", 'w') as json_file:
    json.dump(PackageAssetsJson, json_file, indent=3)
jsonToFlatbuffer(AssetsFileName + ".json", "schemas/PackageAssets.fbs")
os.remove(AssetsFileName + ".json")

combinedLayoutAssetsFileName = "layout_assets.bin"
layout_assetsFilesSizes = combineFiles(combinedLayoutAssetsFileName, [LayoutFileName + ".bin", AssetsFileName + ".bin"])

PackageHeaderJson = {
   "HeaderSize": 32,
   "LayoutSize": layout_assetsFilesSizes[0],
   "AssetsSize": layout_assetsFilesSizes[1],
   "ResourcesSize": sum(rourceFileSizes)
}

with open(HeaderFileName + ".json", 'w') as json_file:
    json.dump(PackageHeaderJson, json_file, indent=3)
jsonToFlatbuffer(HeaderFileName + ".json", "schemas/PackageHeader.fbs")
os.remove(HeaderFileName + ".json")

# read back just to find the size (probably) of the header, than write back
headerFileSize = 0;
with open(HeaderFileName + ".bin", 'rb') as input_file:
   data = input_file.read()
   headerFileSize = len(data)

PackageHeaderJson = {
   "HeaderSize": headerFileSize,
   "LayoutSize": layout_assetsFilesSizes[0],
   "AssetsSize": layout_assetsFilesSizes[1],
   "ResourcesSize": sum(rourceFileSizes)
}

with open(HeaderFileName + ".json", 'w') as json_file:
    json.dump(PackageHeaderJson, json_file, indent=3)
jsonToFlatbuffer(HeaderFileName + ".json", "schemas/PackageHeader.fbs")
os.remove(HeaderFileName + ".json")

combineFiles(packagedFileName, [HeaderFileName + ".bin", combinedLayoutAssetsFileName, combinedResourceFileName])

os.remove(HeaderFileName + ".bin");
os.remove(LayoutFileName + ".bin");
os.remove(AssetsFileName + ".bin");
os.remove(combinedLayoutAssetsFileName);
os.remove(combinedResourceFileName);