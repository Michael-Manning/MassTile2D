import subprocess
import os
import json
import flatbuffers

# Summary:
# Converts all json game assets into flatbuffer format, then combines them into a single binary file
# along with resource files and a header that describes the layout of the binary file

builder = flatbuffers.Builder(1024)

packagedFileName = "Assets.bin"

current_directory = os.path.dirname(os.path.abspath(__file__))
projDir = root_directory = os.path.dirname(os.path.dirname(os.path.dirname(current_directory)))

packageExportDir = os.path.join(projDir, "MyGame")

assetSrcDir = os.path.join(os.path.join(projDir, "data"), "Assets")
prefabsDir = os.path.join(os.path.join(projDir, "data"), "Prefabs")
shadersSrcDir = os.path.join(os.path.join(projDir, "shaders"), "compiled")

flatcPath = os.path.join(os.path.dirname(current_directory), "flatc")

# just use this path with strings in the script to avoid unreadable os.path notation
toolsDir = os.path.dirname(current_directory) + "/" 

def getFlatBufferFromJson(json_path, schema_path):

   # Derive the binary file path based on the json_path
   bin_name = os.path.splitext(os.path.basename(json_path))[0] + ".bin"
   
   # Call flatc to generate the binary file
   command = [flatcPath, "-b", schema_path, json_path]
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
   command = [flatcPath, "-b", schema_path, json_path, "--no-warnings"]
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
               
   return lengths;


excludeJson = getJson( assetSrcDir + "/package_exclude.json")
excludeList = []
excludeList.append("package_exclude.json")
for filename in excludeJson["ignoredFiles"]:
   excludeList.append(filename)

# Create a list of all files in the asset source directory
allAssetFiles = []
for root, dirs, files in os.walk(assetSrcDir):
   for file in files:
      allAssetFiles.append(os.path.join(root, file))
for root, dirs, files in os.walk(prefabsDir):
   for file in files:
      allAssetFiles.append(os.path.join(root, file))



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

print("Gathering assets...")
for file in allAssetFiles:

   print("{0}".format(os.path.basename(file)))

    # Get the file extension
   fileExt = os.path.splitext(file)[1]

   if fileExt == ".sprite":
      spr = getJson(file)
      spriteNames.append(spr["name"])
      spriteIDs.append(spr["ID"])
      sprites.append(spr)
   elif fileExt == ".fjson": 
      fnt = getJson(file)
      fontNames.append(fnt["name"])
      fontIDs.append(fnt["ID"])
      fonts.append(fnt)
   elif fileExt == ".prefab": 
      pfb = getJson(file)
      prefabNames.append(pfb["name"])
      prefabs.append(pfb)
   elif fileExt == ".scene": 
      scn = getJson(file)
      sceneNames.append(scn["name"])
      scenes.append(scn)
   else:
      if os.path.basename(file) not in excludeList:
         resourceFileNames.append(os.path.basename(file))
         resourceFilePaths.append(file)

# add shaders as assets
for root, dirs, files in os.walk(shadersSrcDir):
   for file in files:
      print("{0}".format(os.path.basename(file)))
      resourceFileNames.append(os.path.basename(file))
      resourceFilePaths.append(os.path.join(root, file))

print("")

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

print("serializing layout...")
with open(LayoutFileName + ".json", 'w') as json_file:
    json.dump(PackageLayoutJson, json_file, indent=3)
jsonToFlatbuffer(LayoutFileName + ".json", toolsDir + "/schemas/PackageLayout.fbs")
os.remove(LayoutFileName + ".json")

print("serializing assets...")
with open(AssetsFileName + ".json", 'w') as json_file:
    json.dump(PackageAssetsJson, json_file, indent=3)
jsonToFlatbuffer(AssetsFileName + ".json", toolsDir + "/schemas/PackageAssets.fbs")
os.remove(AssetsFileName + ".json")

combinedLayoutAssetsFileName = "layout_assets.bin"
layout_assetsFilesSizes = combineFiles(combinedLayoutAssetsFileName, [LayoutFileName + ".bin", AssetsFileName + ".bin"])

PackageHeaderJson = {
   "HeaderSize": 32,
   "LayoutSize": layout_assetsFilesSizes[0],
   "AssetsSize": layout_assetsFilesSizes[1],
   "ResourcesSize": sum(rourceFileSizes)
}

print("creating Header...")
with open(HeaderFileName + ".json", 'w') as json_file:
    json.dump(PackageHeaderJson, json_file, indent=3)
jsonToFlatbuffer(HeaderFileName + ".json", toolsDir + "/schemas/PackageHeader.fbs")
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

print("Putting it all together...\n")

with open(HeaderFileName + ".json", 'w') as json_file:
    json.dump(PackageHeaderJson, json_file, indent=3)
jsonToFlatbuffer(HeaderFileName + ".json", toolsDir + "/schemas/PackageHeader.fbs")
os.remove(HeaderFileName + ".json")

combineFiles( os.path.join(packageExportDir, packagedFileName), [HeaderFileName + ".bin", combinedLayoutAssetsFileName, combinedResourceFileName])

print("exported assets to: {0}".format(os.path.join(packageExportDir, packagedFileName)))

os.remove(HeaderFileName + ".bin");
os.remove(LayoutFileName + ".bin");
os.remove(AssetsFileName + ".bin");
os.remove(combinedLayoutAssetsFileName);
os.remove(combinedResourceFileName);