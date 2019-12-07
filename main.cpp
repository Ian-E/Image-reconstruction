#include <string.h>
#include <stdlib.h>
#include <stdio.h>



#include "FileSystem.h"
#include "imgUtil.cpp"
// #include "fft.cpp"


#define BLOCKSIZE 50000





int getCsvOffset(char* csv, int index){
    for(int i=0, c=0; ; i++){
        if(csv[i] == EOF){ return -1; }
        if(c == index){ return i; }
        c += csv[i]==',';
    }
}

int indexOf(int* array, int arrayLen, int value){
    for(int i=0; i<arrayLen; i++){
        if(array[i] == value){
            return i;
        }
    }

    return -1;
}


int main(int argc, char** argv){
    const char* targetFolderDefault = "fragments";
    const char* targetFolder = argc==2 ? argv[1] : targetFolderDefault;

    FileSystem fragmentsFolder;
    if(!fragmentsFolder.openDirectory(targetFolder)){
        printf("Target folder not found\n");
        return 0;
    }

    FileSystem binsFolder;
    if(!binsFolder.openDirectory("bins/")){
        // TODO: should create folder here
        printf("No bins folder found\n");
        return 0;
    }


    const int saveTop = 3; // each image fragment is put into 3 bins
    const int windowSize = 10;


    int* imageCounts = NULL;
    int* bins = NULL;
    int binCount = 0;

    unsigned char bgrContents[BLOCKSIZE];
    char csvContents[256];



    char pathName[64];
    int directoryCount = fragmentsFolder.getDirectoriesCount();














    // discover all headers
    int* headerFrequencies = NULL;
    int* headerSizes = NULL;
    int headerCount = 0;

    for(int i=0; i<directoryCount/3; i++){
        sprintf(pathName, "%s/%d.csv", targetFolder, i);
        FILE* csvFile = fopen(pathName, "r");
        fgets(csvContents, 256, csvFile);
        fclose(csvFile);

        if(csvContents[getCsvOffset(csvContents, 7)] == '1'){
            sprintf(pathName, "%s/%d.byte", targetFolder, i);
            FILE* bgrFile = fopen(pathName, "r");
            fread(bgrContents, 1, BLOCKSIZE, bgrFile);
            fclose(bgrFile);

            int width;
            sscanf(csvContents+getCsvOffset(csvContents, 2), "%d", &width);

            int index = indexOf(headerSizes, headerCount, width);
            if(index != -1){
                headerFrequencies[index] += 1;
            }else{
                headerCount += 1;
                headerFrequencies = (int*)realloc(headerFrequencies, sizeof(int) * headerCount);
                headerSizes = (int*)realloc(headerSizes, sizeof(int) * headerCount);

                headerFrequencies[headerCount-1] = 1;
                headerSizes[headerCount-1] = width;
                index = headerCount-1;
            }

            char binNameBuff[128];
            sprintf(binNameBuff, "bins/image_%d_%d.csv", width, headerFrequencies[index]);
            FILE* binFile = fopen(binNameBuff, "w");
            fprintf(binFile, "fragments/%d,1,0,1", i, headerFrequencies[index], i);
            fclose(binFile);
        }
    }




    for(int i=0; i<directoryCount/3; i++){
        sprintf(pathName, "%s/%d.csv", targetFolder, i);

        FILE* csvFile = fopen(pathName, "r");
        fgets(csvContents, 256, csvFile);
        fclose(csvFile);

        if(csvContents[getCsvOffset(csvContents, 7)] != '1'){
            sprintf(pathName, "%s/%d.byte", targetFolder, i);
            FILE* bgrFile = fopen(pathName, "r");
            fread(bgrContents, 1, BLOCKSIZE, bgrFile);
            fclose(bgrFile);

            ImageFragment img(bgrContents, BLOCKSIZE);
            SuspectedValueList sWidth = img.detectWidth(windowSize, saveTop);

            bool found = false;
            for(int j=0; j<saveTop; j++){
                for(int k=0; k<headerCount; k++){
                    if(abs(headerSizes[k] - sWidth[j]->value) < windowSize){
                        SuspectedValueList sOffset = img.detectOffset(sWidth[j]->value, 1);
                        found = true;

                        for(int l=0; l<headerFrequencies[k]; l++){
                            sprintf(pathName, "bins/image_%d_%d.csv", headerSizes[k], l+1); // ew, the FileSystem class should handle stuff like this
                            FILE* binFile = fopen(pathName, "a");
                            fprintf(binFile, "\nfragments/%d,%lf,%d,%lf", i, sWidth[j]->confidence, sOffset[0]->value, sOffset[0]->confidence);
                            fclose(binFile);
                        }
                    }
                }
            }
            
            // put the fragment in all of them
            if(!found){
                printf("not found: %s %d %d %d\n", pathName, sWidth[0]->value, sWidth[1]->value, sWidth[2]->value);

                for(int j=0; j<headerCount; j++){
                    SuspectedValueList sOffset = img.detectOffset(headerSizes[j], 1);

                    for(int k=0; k<headerFrequencies[j]; k++){
                        sprintf(pathName, "bins/image_%d_%d.csv", headerSizes[j], k+1); // ew, the FileSystem class should handle stuff like this
                        FILE* binFile = fopen(pathName, "a");
                        fprintf(binFile, "\nfragments/%d,0,%d,%lf", i, sOffset[0]->value, sOffset[0]->confidence);
                        fclose(binFile);
                    }
                }
            }
        }

        printf("%d / %d\n", i, directoryCount/3);
    }

/*


    for(int i=0; i<directoryCount/3; i++){
        sprintf(pathName, "%s/%d.byte", targetFolder, i);
        FILE* bgrFile = fopen(pathName, "r");
        fread(bgrContents, 1, BLOCKSIZE, bgrFile);
        fclose(bgrFile);

        sprintf(pathName, "%s/%d.csv", targetFolder, i);
        FILE* csvFile = fopen(pathName, "r");
        fgets(csvContents, 256, csvFile);
        fclose(csvFile);

        double sWidthConfidences[saveTop];
        int sWidthValues[saveTop];

        double sOffsetConfidences[saveTop];
        int sOffsetValues[saveTop];

        int putInBinCount;

        int insertionIndex;

        // get the suspected widths
        char binNameBuff[128];
        if(csvContents[getCsvOffset(csvContents, 7)] == '1'){
            sscanf(csvContents+getCsvOffset(csvContents, 2), "%d", &sWidthValues[0]);

            sOffsetConfidences[0] = 1;
            sOffsetValues[0] = 0;

            sWidthConfidences[0] = 1;
            putInBinCount = 1;

            binsFolder.createDirectory(binNameBuff);

            bool found = false;
            insertionIndex = 0;
            while(insertionIndex<binCount && bins[insertionIndex]<=sWidthValues[0]){
                if(bins[insertionIndex] == sWidthValues[0]){
                    found = true;
                    break;
                }

                insertionIndex += 1;
            }

            // add new bins if they dont exist
            if(!found){
                binCount += 1;

                bins = (int*)realloc(bins, sizeof(int)*binCount);
                memmove(bins+insertionIndex+1, bins+insertionIndex, sizeof(int)*(binCount-insertionIndex-1));
                bins[insertionIndex] = sWidthValues[insertionIndex];

                imageCounts = (int*)realloc(imageCounts, sizeof(int)*binCount);
                memmove(imageCounts+insertionIndex+1, imageCounts+insertionIndex, sizeof(int)*(binCount-insertionIndex-1));
                imageCounts[insertionIndex] = 0;
            }

            sprintf(binNameBuff, "image_%d_%d", imageCounts[insertionIndex]++, sWidthValues[0]);
            binsFolder.createDirectory(binNameBuff);
        }else{
            ImageFragment img(bgrContents, BLOCKSIZE);
            SuspectedValueList sWidth = img.detectWidth(saveTop);


            bool found = false;
            insertionIndex = 0;
            while(insertionIndex<binCount && bins[insertionIndex]<=sWidthValues[0]){
                if(bins[insertionIndex] == sWidthValues[0]){
                    found = true;
                    break;
                }

                insertionIndex += 1;
            }


            for(int j=0; j<saveTop; j++){
                SuspectedValueList sOffset = img.detectOffset(sWidth[j]->value, 1);
                sWidthConfidences[j] = sWidth[j]->confidence;
                sWidthValues[j] = sWidth[j]->value;
                
                sOffsetConfidences[j] = sOffset[0]->confidence;
                sOffsetValues[j] = sOffset[0]->value;
                
                for(int j=0; j<imageCounts[insertionIndex]; j++){
                    printf("here\n");
                    sprintf(pathName, "bins/image_%d_%d/%d.byte", j, sWidthValues[j], i); // ew, the FileSystem class should handle stuff like this
                    FILE* bgrCopy = fopen(pathName, "w");
                    fwrite(bgrContents, 1, BLOCKSIZE, bgrCopy);
                    fclose(bgrCopy);

                    sprintf(pathName, "bins/image_%d_%d/%d.csv", j, sWidthValues[j], i); // ew, the FileSystem class should handle stuff like this
                    FILE* csvCopy = fopen(pathName, "w");
                    fprintf(csvCopy, "%s,%lf,%d,%lf", csvContents, sWidthConfidences[j], sOffsetValues[j], sOffsetConfidences[j]);
                    fclose(csvCopy);
                }
            }
        }
*/

/*
        for(int j=0; j<putInBinCount; j++){

            // add new bins if they dont exist
            if(!found){
                binCount += 1;

                bins = (int*)realloc(bins, sizeof(int)*binCount);
                memmove(bins+i+1, bins+i, sizeof(int)*(binCount-i-1));
                bins[i] = sWidthValues[j];

                imagesCount = (int*)realloc(imagesCount, sizeof(int)*binCount);
                memmove(imagesCount+i+1, imagesCount+i, sizeof(int)*(binCount-i-1));
                imagesCount[i] = 0;

                binsFolder.createDirectory(binNameBuff);
            }

        }
    }
    */




    return 0;



   

    // Signature bmpHeaderSig("66 77 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? * * * * * * * *");
    // int signatureLength = bmpHeaderSig.getLength();
    // char* headerData = (char*)bmpHeaderSig.getData();

    // bmpHeaderSig.scan(data, signatureLength);
    // int* info = (int*)headerData;

/*


    // TESTS ALL IMAGES
    for(int i=1; i<=20; i++){

        char path[64];
        sprintf(path, "E:/Desktop/Random/C++/Image Carving/images/%04d.bmp", i);

        BMP image(path);
        unsigned long dataLen = image.getDataLen();
        unsigned char* data = image.getData();
        // int bytesPerPixel = image.getChannels();
        int r = ((rand() % 20) * 50) % image.getWidth();

        ImageFragment img(data+r, dataLen / 6);


        SuspectedValueList sWidth = img.detectWidth();
        SuspectedValueList sOffset = img.detectOffset(sWidth[0]->value);

        // if(sWidth[0]->value != image.getWidth()){
            printf("\n%s - Width: %5d         Offset: %5d\n", path+43, image.getWidth(), r);
            printf("                  %5d %.2lf%%           %5d %.2lf%%\n", sWidth[0]->value, sWidth[0]->confidence, sOffset[0]->value, sOffset[0]->confidence);
            printf("                  %5d %.2lf%%           %5d %.2lf%%\n", sWidth[1]->value, sWidth[1]->confidence, sOffset[1]->value, sOffset[1]->confidence);
            printf("                  %5d %.2lf%%           %5d %.2lf%%\n", sWidth[2]->value, sWidth[2]->confidence, sOffset[2]->value, sOffset[2]->confidence);
        // }

    }
*/
    return 0;
}