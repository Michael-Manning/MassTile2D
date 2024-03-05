import os
import csv
import hash_extension

current_directory = os.path.dirname(os.path.abspath(__file__))
projDir = root_directory = os.path.dirname(os.path.dirname(current_directory))
assetSrcDir = os.path.join(os.path.join(projDir, "data"), "Assets")
headerDstDir = os.path.join(projDir, "MyGame")

srcCSVs = ["Tools.csv", "Consumables.csv", "Blocks.csv", "MapEntities.csv"]

outHeaderName = "ItemIDs.h"
outSourceName = "ItemIDs.cpp"


class itemBase:
   ID = 0
   type = ""
   name = ""
   description = ""
   maxStack = 0
   sprite = 0
   atlasIndex = 0
   
   def __lt__(self, other):
      return self.ID < other.ID

items = []
blockIDmap = []


ID_Names = []
mapEntityName_spriteIDs = []

with open(os.path.join(assetSrcDir,"Blocks.csv"), mode='r', encoding='utf-8') as csv_file:
   csv_reader = csv.DictReader(csv_file)
   for row in csv_reader:
      item = itemBase()
      item.ID = int(row['itemID'])
      item.type = "ItemBase::Type::Block"
      item.name = row["name"]
      item.description = row["description"]
      item.maxStack = row["maxStack"]
      item.sprite = hash_extension.hash_string("tilemapSprites")
      item.atlasIndex = "static_cast<int>(GetFloatingTile(" + row["blockID"] + "))"

      blockIDmap.append((row['itemID'], row["blockID"]))

      items.append(item);
      ID_Names.append((int(row['itemID']), row['name']))

with open(os.path.join(assetSrcDir,"Tools.csv"), mode='r', encoding='utf-8') as csv_file:
   csv_reader = csv.DictReader(csv_file)
   for row in csv_reader:

      item = itemBase()
      item.ID = int(row['itemID'])
      item.type = "ItemBase::Type::Tool"
      item.name = row["name"]
      item.description = row["description"]
      item.maxStack = row["maxStack"]
      item.sprite = hash_extension.hash_string("itemSprites")
      item.atlasIndex = row["inventorySpriteAtlasIndex"]

      items.append(item);
      ID_Names.append((int(row['itemID']), row['name']))

with open(os.path.join(assetSrcDir,"Consumables.csv"), mode='r', encoding='utf-8') as csv_file:
   csv_reader = csv.DictReader(csv_file)
   for row in csv_reader:
      item = itemBase()
      item.ID = int(row['itemID'])
      item.type = "ItemBase::Type::Consumable"
      item.name = row["name"]
      item.description = row["description"]
      item.maxStack = row["maxStack"]
      item.sprite = hash_extension.hash_string("itemSprites")
      item.atlasIndex = row["inventorySpriteAtlasIndex"]

      items.append(item);
      ID_Names.append((int(row['itemID']), row['name']))

with open(os.path.join(assetSrcDir,"MapEntities.csv"), mode='r', encoding='utf-8') as csv_file:
   csv_reader = csv.DictReader(csv_file)
   for row in csv_reader:
      item = itemBase()
      item.ID = int(row['itemID'])
      item.type = "ItemBase::Type::MapEntity"
      item.name = row["name"]
      item.description = row["description"]
      item.maxStack = row["maxStack"]
      item.sprite = hash_extension.hash_string("item_" + str(item.ID))
      item.atlasIndex = '0'

      items.append(item);
      ID_Names.append((int(row['itemID']), row['name']))
      mapEntityName_spriteIDs.append((row['name'], item.sprite))

items.sort()






# Generate the required string format
output_lines = []
for itemID, name in ID_Names:
   # Replace spaces with underscores and remove special characters for variable names
   sanitized_name = ''.join([c if c.isalnum() else '_' for c in name]).strip('_')
   line = f"constexpr itemID {sanitized_name}_ItemID = {itemID};"
   output_lines.append(line)

# Join all lines into a single string
output_string = "#pragma once\n\n"
output_string += "#include <stdint.h>\n\n"
output_string += '#include "typedefs.h"\n\n'
output_string += "typedef uint32_t itemID;\n\n"

output_string += '\n'.join(output_lines)

output_string += "\n\n"

for name, id in mapEntityName_spriteIDs:
   output_string += "spriteID " + name +"_spriteID = " + str(id) +  ";\n"

f = open(os.path.join(headerDstDir, outHeaderName), "w")
f.write(output_string)
f.close()










itemscpp = """#include "stdafx.h"

#include <vector>

#include "Inventory.h"
#include "ItemIDs.h"


ItemLibrary::ItemLibrary() : itemBaseLookup{
"""

currentItemIndex = 0
nextItemID = items[currentItemIndex].ID
currentID = 0
while True:
   if currentID == nextItemID:
      it = items[currentItemIndex]
      itemscpp += '\tItemBase({ID}, {type}, "{name}", "{description}", {maxStack}, {sprite}, {atlasIndex}),'.format(ID = it.ID, type = it.type, name = it.name, description = it.description, maxStack = it.maxStack, sprite = it.sprite, atlasIndex = it.atlasIndex)
      currentItemIndex += 1; 
      if(currentItemIndex == len(items)):
         break
      nextItemID = items[currentItemIndex].ID
   else:
      itemscpp += "\tItemBase(), // " + str(currentID)
   itemscpp += "\n"
   currentID += 1

itemscpp += "\n\t} {\n"

itemscpp += "\tblockLookup = {\n"
for item, block in blockIDmap:
   itemscpp += "\t\t{" + str(item) + ", " + block + "},\n"
itemscpp += "\t};\n\n"

itemscpp += "\tblockItemLookup = {\n"
for item, block in blockIDmap:
   itemscpp += "\t\t{" + block + ", " + str(item) + "},\n"
itemscpp += "\t};\n"

itemscpp += "}"

f = open(os.path.join(headerDstDir, outSourceName), "w")
f.write(itemscpp)
f.close()