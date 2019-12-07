#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "string.h"
#include "stdlib.h"

#include <Windows.h>
#include <dirent.h>

class FileSystem{
    public:
        FileSystem(){
            this->ignoredDirectoriesLength = 0;
            this->directoryLength          = 0;
            this->ignoredDirectories       = NULL;
            this->directories              = NULL;
            this->parentDirectory          = NULL;
        }

        ~FileSystem(){
            this->freeIgnoredDirectories();
            this->freeSubDirectories();

            free(this->parentDirectory);
        }

        int getDirectoriesCount(){
            return this->directoryLength;
        }

        void addIgnoredDirectory(const char* directory){
            this->ignoredDirectories = (char**)realloc(ignoredDirectories, sizeof(char*)*(this->ignoredDirectoriesLength+1));
            this->ignoredDirectories[this->ignoredDirectoriesLength] = (char*)malloc(strlen(directory)+1);
            memcpy(this->ignoredDirectories[this->ignoredDirectoriesLength], directory, strlen(directory)+1);
            this->ignoredDirectoriesLength += 1;
        }

        bool openDirectory(const char* parentDirectory){
            int parentDirectoryLength = strlen(parentDirectory);
            char* usableDirectory;
            bool freeDirectory;

            this->parentDirectory = (char*)malloc(parentDirectoryLength+1);
            memcpy(this->parentDirectory, parentDirectory, parentDirectoryLength+1);

            if(parentDirectory[parentDirectoryLength-1] != '/' && parentDirectory[parentDirectoryLength-1] != '\\'){
                usableDirectory = (char*)malloc(parentDirectoryLength+2);
                memcpy(usableDirectory, parentDirectory, parentDirectoryLength);
                usableDirectory[parentDirectoryLength] = '/';
                usableDirectory[parentDirectoryLength+1] = 0;
                parentDirectoryLength += 1;
                freeDirectory = true;

            }else{
                usableDirectory = (char*)parentDirectory;
                freeDirectory = false;
            }

            DIR* directoryStream = opendir(usableDirectory);
            dirent* directory;

            if(!directoryStream){ return false; }

            this->freeSubDirectories();

            while((directory = readdir(directoryStream))){
                int directoryNameLength = strlen(directory->d_name);

                if(directoryNameLength < 3 && (memcmp(directory->d_name, "..", 2) || memcmp(directory->d_name, ".\0", 2))){ continue; }

                bool contains = false;
                for(int i=0; i<this->ignoredDirectoriesLength; i++){
                    if(!strcmp(this->ignoredDirectories[i], directory->d_name)){
                        contains = true;
                        break;
                    }
                }

                if(!contains){
                    this->directoryLength += 1;
                    this->directories = (char**)realloc(this->directories, sizeof(char**) * this->directoryLength);
                    this->directories[this->directoryLength-1] = (char*)malloc(parentDirectoryLength + directoryNameLength + 1);

                    memcpy(this->directories[this->directoryLength-1], usableDirectory, parentDirectoryLength);
                    memcpy(this->directories[this->directoryLength-1]+parentDirectoryLength, directory->d_name, directoryNameLength+1);
                }
            }

            closedir(directoryStream);
            if(freeDirectory){ free(usableDirectory); }
            return true;
        }

        void appendSubDirectory(const char* subDirectory){
            int subDirectoryLength = strlen(subDirectory);

            for(int i=0; i<this->directoryLength; i++){
                int directoryLength = strlen(this->directories[i]);

                this->directories[i] = (char*)realloc(this->directories[i], directoryLength + subDirectoryLength + 1);
                memcpy(this->directories[i]+directoryLength, subDirectory, subDirectoryLength+1);
            }
        }

        void printDirectories(){
            for(int i=0; i<this->directoryLength; i++){
                printf("%s\n", this->directories[i]);
            }
        }

        void createDirectory(const char* name){
            int newDirectoryLen = strlen(this->parentDirectory) + strlen(name);
            char* newDirectory = (char*)malloc(newDirectoryLen+1);
            sprintf(newDirectory, "%s/%s", this->parentDirectory, name);

            CreateDirectory(newDirectory, NULL);

            free(newDirectory);
        }

        char** ignoredDirectories;
        char** directories;
    private:
        void freeIgnoredDirectories(){
            for(int i=0; i<this->ignoredDirectoriesLength; i++){ free(this->ignoredDirectories[i]); }
            free(this->ignoredDirectories);

            this->ignoredDirectories       = NULL;
            this->ignoredDirectoriesLength = 0;
        }

        void freeSubDirectories(){
            for(int i=0; i<this->directoryLength; i++){ free(this->directories[i]); }
            free(this->directories);

            this->directories     = NULL;
            this->directoryLength = 0;
        }

        int ignoredDirectoriesLength;
        int directoryLength;

        char* parentDirectory;
};

#endif