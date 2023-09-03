import csv
from PIL import Image

outSizex = 32
outSizey = 32
outStride = 16
inStride = 18
tileSize = 16

atlasStride = 16 * 3

imgPaths = ["Tiles_2.png", "Tiles_0.png", "Tiles_1.png", "Tiles_9.png"]
outpath = "tilemapSprites.png"

# parse csv file full of integers into a 2d array
def getNumbersFromCSV(path): 
    with open(path, newline='') as csvfile:
        reader = csv.reader(csvfile, delimiter=',', quotechar='|')
        numbers = []
        for row in reader:
            numbers.append([int(x) for x in row])
        return numbers

numbersList = getNumbersFromCSV("tilemapAtlasHashLayout.csv");

# create dictionary keyed by numbersList values, valued by a list of the columns and rows of the number's positions as it appears in numbersList

numbersDict = {}
for row in range(len(numbersList)):
    for col in range(len(numbersList[row])):
        num = numbersList[row][col]
        if num not in numbersDict:
            numbersDict[num] = []
        numbersDict[num].append([row, col])

outImg = Image.new('RGBA', (outSizex * outStride, outSizey * outStride))
# outImg = Image.open("out.png")

# can only be three of each hash in the csv
atlasCount = 0
for path in imgPaths:
    inImg = Image.open(path)
    for num, locations in numbersDict.items():
        if(num >= 0):
            variation = 0
            for row, col in locations:
                pos = num * 3 + variation + atlasCount * atlasStride
                pastx = pos  % outSizex
                pasty = pos // outSizex
                outImg.paste( inImg.crop((col * inStride, row * inStride, col * inStride + tileSize, row * inStride + tileSize )) , (pastx * outStride, pasty * outStride))
                variation += 1
    atlasCount += 1


outImg.save(outpath)

