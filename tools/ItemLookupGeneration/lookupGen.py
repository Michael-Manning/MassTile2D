import os
import csv

current_directory = os.path.dirname(os.path.abspath(__file__))
projDir = root_directory = os.path.dirname(os.path.dirname(current_directory))
assetSrcDir = os.path.join(os.path.join(projDir, "data"), "Assets")
headerDstDir = os.path.join(projDir, "MyGame")

srcCSVs = ["Tools.csv", "Consumables.csv", "Blocks.csv", "MapEntities.csv"]

outHeaderName = "ItemIDs.h"


import hash_extension

hashed_value = hash_extension.hash_string("demon.png")
print(hashed_value)
hashed_value = hash_extension.hash_string("demon")
print(hashed_value)

# Function to read CSV and extract required data
def read_csv_and_extract_data(file_name):
    data = []
    with open(file_name, mode='r', encoding='utf-8') as csv_file:
        csv_reader = csv.DictReader(csv_file)
        for row in csv_reader:
            if 'itemID' not in row and 'name' in row:
               raise Exception("invalid csv columns")
            data.append((int(row['itemID']), row['name']))
    return data

# Combined list of all items from all CSVs
all_items = []
for csv_file in srcCSVs:
    all_items.extend(read_csv_and_extract_data(os.path.join(assetSrcDir, csv_file)))

# Generate the required string format
output_lines = []
for itemID, name in all_items:
    # Replace spaces with underscores and remove special characters for variable names
    sanitized_name = ''.join([c if c.isalnum() else '_' for c in name]).strip('_')
    line = f"constexpr itemID {sanitized_name}_ItemID = {itemID};"
    output_lines.append(line)

# Join all lines into a single string
output_string = "#pragma once\n\n"
output_string += "#include <stdint.h>\n\n"
output_string += "typedef uint32_t itemID;\n\n"

output_string += '\n'.join(output_lines)
# print(output_string)

f = open(os.path.join(headerDstDir, outHeaderName), "w")
f.write(output_string)
f.close()
