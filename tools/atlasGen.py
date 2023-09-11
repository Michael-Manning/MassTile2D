from PIL import Image

import os
import json
import math

# Directory containing the images
image_directory = '../data/images/player'

# Output combined image file
output_image_path = 'combined_image.png'

# Output JSON metadata file
output_json_path = 'texture_atlas.json'

# Get all the image files in the directory
image_files = [f for f in os.listdir(image_directory) if os.path.isfile(os.path.join(image_directory, f))]

# make image_files only contain .png files
image_files = [f for f in image_files if f.endswith('.png')]

# Load images
images = [Image.open(os.path.join(image_directory, file)) for file in image_files]

# Get the size of an individual image (assuming all images have the same size)
image_width, image_height = images[0].size

# Calculate grid size
grid_size = int(math.ceil(math.sqrt(len(images))))

# Create a blank image to paste the individual images onto
combined_image = Image.new('RGBA', (image_width * grid_size, image_height * grid_size))

# List to hold metadata entries
atlas_entries = []

# Iterate through images and paste them onto the combined image
for i, image in enumerate(images):
    row = i // grid_size
    col = i % grid_size
    x_offset = col * image_width
    y_offset = row * image_height
    combined_image.paste(image, (x_offset, y_offset))

    # Calculate normalized UV coordinates
    u_min = x_offset / (image_width * grid_size)
    v_min = y_offset / (image_height * grid_size)
    u_max = (x_offset + image_width) / (image_width * grid_size)
    v_max = (y_offset + image_height) / (image_height * grid_size)

    atlas_entries.append({
        'name': image_files[i],
        'u_min': u_min,
        'v_min': v_min,
        'u_max': u_max,
        'v_max': v_max
    })

# Save the combined image
combined_image.save(output_image_path)

# Write the JSON metadata file
with open(output_json_path, 'w') as json_file:
    json.dump({"AtlasEntries": atlas_entries}, json_file, indent=4)

print(f'Combined image saved to {output_image_path}')
print(f'JSON metadata saved to {output_json_path}')
