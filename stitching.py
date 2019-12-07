import os



basePath = "E:/Desktop/Random/C++/Image Carving/stitching/"
fragmentsBasePath = basePath + "fragments/"
binsBasePath = basePath + "bins/"
cedBasePath = basePath + "ced/"
reconstructedBasePath = basePath + "reconstructed/"

blockSize = 50000


# TODO: THIS NEEDS TO CHANGE FOR THE FINAL PROGRAM. THIS ALLOWS ONE FRAGMENT TO BE IN MULTIPLE IMAGES
def processBin(path):
    fragments = []
    image = []

    # open the original bin file
    with open(binsBasePath + path, "r") as binFile:

        # read the first line, since we know it should be a header fragment
        line = binFile.readline()
        vList = line.split(",")
        image.append(vList)

        # open the fragment of the header file so i know how many bytes the full file should be
        with open(fragmentsBasePath+vList[0]+".csv") as headerFragFile:
            vList = headerFragFile.readline().split(",")
            fragmentsNeeded = (int(vList[5])+blockSize-1) // blockSize

        # read every new line from the bin file and save its parsed values to the 'fragments' array for easy usage
        # this also establishes what index in the distance array each file name is
        indexMap = {image[0][0]+".bgr":0}
        line = binFile.readline()
        index = 1
        while line:
            vList = line.split(",")
            fragments.append(vList)
            line = binFile.readline()

            indexMap[vList[0] + ".bgr"] = index
            index += 1;

        # open the ced folder and generate distance matrix
        distances = [[None for j in range(len(fragments)+1)] for i in range(len(fragments)+1)]
        with open(cedBasePath + path, "r") as cedFile:

            line = cedFile.readline()
            while line:
                vList = line.split(",")
                distances[indexMap[vList[0]]][indexMap[vList[1]]] = float(vList[2])
                
                line = cedFile.readline()

        # find each fragment that i need, 1 at a time
        # i already have the header fragment, so i can start at index 1
        usedIndexes = [0]
        for i in range(1, fragmentsNeeded):
            previousIndex = usedIndexes[i-1]

            # scan the distances row for the highest scoring (lowest value) fragment
            # TODO: in the actual program, minValue would be a combination score of other values
            minValue = float("inf")
            minIndex = 0;
            for j in range(1, len(fragments)+1):
                if j not in usedIndexes and distances[previousIndex][j] != None and distances[previousIndex][j]  < minValue:
                    minValue = distances[previousIndex][j] 
                    minIndex = j
            image.append(fragments[minIndex-1])
            usedIndexes.append(minIndex)

        # stitch the image back together
        # TODO: the extension replacement shouldnt be hard coded here
        with open(reconstructedBasePath+path.replace(".csv", ".bmp"), "wb") as reconstructedFile:
            for frag in image:
                with open(fragmentsBasePath + frag[0] + ".byte", "rb") as fragmentedFile:
                    reconstructedFile.write(fragmentedFile.read())


# the first element in every bin should be a header
for entry in os.listdir(binsBasePath):
    if os.path.isfile(os.path.join(binsBasePath, entry)) and entry.startswith('image_'):
        processBin(entry)