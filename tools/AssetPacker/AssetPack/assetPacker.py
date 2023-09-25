import subprocess
import os
import flatbuffers
import Sprite
import Package
# import AssetPack.Package

builder = flatbuffers.Builder(1024)

assetSrcDir = "../../data/Assets/";
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

# Create a list of all files in the asset source directory
jsonFiles = []
for root, dirs, files in os.walk(assetSrcDir):
    for file in files:
        jsonFiles.append(os.path.join(root, file))



spriteNames = []
spriteIDs = []
sprites = []

for file in jsonFiles:
    # Get the file extension
    fileExt = os.path.splitext(file)[1]

    if fileExt == ".sprite":
        data = getFlatBufferFromJson(file, "schemas/Sprite.fbs");
        spr = Sprite.Sprite.GetRootAs(data, 0);
        spriteNames.append(spr.Name())
        spriteIDs.append(spr.Id())
        sprites.append(spr)



strings_offsets = [builder.CreateString(s) for s in spriteNames]
Package.StartSpriteNamesVector(builder, len(spriteNames))
for s in reversed(strings_offsets):
    builder.PrependUOffsetTRelative(s)
spriteNameVec = builder.EndVector();

Package.StartSpriteIdsVector(builder, len(spriteIDs))
for i in reversed(spriteIDs):
    builder.PrependUint32(i)
spriteIDsVec = builder.EndVector();

Package.StartSpritesVector(builder, len(sprites))
sprite_offsets = [Sprite.c(builder, elem.Value()) for elem in elements]
for s in reversed(sprites):
    builder.PrependUOffsetTRelative(s)
spritesVec = builder.EndVector();

Package.Start(builder)
Package.AddSpriteNames(builder, spriteNameVec)
Package.AddSpriteIds(builder, spriteIDsVec)
Package.addSprites(builder, spritesVec)

package = Package.End(builder)

# save package data as a bin file
builder.Finish(package)
buf = builder.Output()
with open("Assets.bin", 'wb') as f:
    f.write(buf)