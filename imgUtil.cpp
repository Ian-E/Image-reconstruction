#ifndef IMAGEUTIL_H
#define IMAGEUTIL_H

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "fft.cpp"

// class for keeping track of a certain number of 
class SuspectedValueList{
    public:
        struct suspectedValue{
            double confidence;
            double score;
            int value;
        };

        SuspectedValueList(unsigned short keepTop, bool ignoreMultiples=false): keepTop(keepTop), ignoreMultiples(ignoreMultiples){
            this->suspectedValues = (suspectedValue*)calloc(this->keepTop, sizeof(suspectedValue));
        }

        SuspectedValueList(const SuspectedValueList& source){
            this->keepTop = source.keepTop;
            this->ignoreMultiples = source.ignoreMultiples;
            this->suspectedValues = (suspectedValue*)malloc(sizeof(suspectedValue) * this->keepTop);
            memcpy(this->suspectedValues, source.suspectedValues, sizeof(suspectedValue) * this->keepTop);
        }

        SuspectedValueList(){
            this->suspectedValues = NULL;
        }

        ~SuspectedValueList(){
            free(this->suspectedValues);
        }

        SuspectedValueList& operator=(const SuspectedValueList& source){
            this->keepTop = source.keepTop;
            this->ignoreMultiples = source.ignoreMultiples;

            free(this->suspectedValues);
            this->suspectedValues = (suspectedValue*)malloc(sizeof(suspectedValue) * this->keepTop);
            memcpy(this->suspectedValues, source.suspectedValues, sizeof(suspectedValue) * this->keepTop);

            return *this;
        }

        suspectedValue* operator[](const int i){
            if(i > this->keepTop){
                if(this->keepTop){
                    return this->suspectedValues+(this->keepTop-1);
                }

                return NULL;
            }

            return this->suspectedValues+i;
        }

        void setIgnoreMultiples(bool ignoreMultiples){
            this->ignoreMultiples = ignoreMultiples;
        }

        

        bool process(int value, double score){
            if(this->keepTop && score < this->suspectedValues[this->keepTop-1].score){
                return false;
            }

            if(this->ignoreMultiples){
                for(int i=0; i<this->keepTop; i++){
                    if(score < this->suspectedValues[i].score && !(value % this->suspectedValues[i].value)){
                        return false;
                    }
                }
            }

            int insertIndex = this->keepTop - 1;
            while(insertIndex > 0 && this->suspectedValues[insertIndex-1].score < score){
                this->suspectedValues[insertIndex] = this->suspectedValues[insertIndex-1];
                insertIndex -= 1;
            }

            this->suspectedValues[insertIndex].confidence = 0;
            this->suspectedValues[insertIndex].score = score;
            this->suspectedValues[insertIndex].value = value;

            return true;
        }

        // modified implimentation of softmax
        void calcConfidence(){
            if(!this->keepTop){ return; }

            double sum = 0;
            for(int i=0; i<this->keepTop; i++){
                sum += this->suspectedValues[i].score;
            }

            for(int i=0; i<this->keepTop; i++) {
                this->suspectedValues[i].confidence = this->suspectedValues[i].score / sum;
            }
        }

        void print(){
            for(int i=0; i<this->keepTop; i++){
                printf("%8d (%%%.2lf)\n", this->suspectedValues[i].value, this->suspectedValues[i].confidence);
            }
            printf("\n");
        }

    private:

        bool ignoreMultiples;

        suspectedValue* suspectedValues;
        unsigned short keepTop;
};

class MovingMedian{
    public:
        MovingMedian(const int windowSize): windowSize(windowSize){
            this->numbers = (number*)malloc(sizeof(number) * windowSize);
            this->middleIndex = windowSize >> 1;
        }

        ~MovingMedian(){
            free(this->numbers);
        }

        // initialize the moving median
        // array must be at least 'windowSize' elements long
        double initialize(double* array){
            for(int i=0; i<windowSize; i++){
                this->numbers[i].value = array[i];
                this->numbers[i].insertionIndex = i;
            }

            for(int i=0; i<windowSize; i++){
                int minInd = i;
                double minVal = this->numbers[i].value;
                for(int j=i+1; j<windowSize; j++){
                    if(this->numbers[j].value < minVal){
                        minVal = this->numbers[j].value;
                        minInd = j;
                    }
                }

                if(minInd != i){
                    number tmp = this->numbers[i];
                    this->numbers[i] = this->numbers[minInd];
                    this->numbers[minInd] = tmp;
                }
            }

            return this->numbers[this->middleIndex].value;
        }

        double process(double newValue){

            int i = 0;
            for(; i<windowSize && this->numbers[i].insertionIndex; i++){ this->numbers[i].insertionIndex -= 1;  }
            for(; i<windowSize; i++){ this->numbers[i] = this->numbers[i+1]; this->numbers[i].insertionIndex -= 1;  }

            // sorted insert
            i = windowSize - 1;
            for(; i > 0 && this->numbers[i-1].value > newValue; i--){ this->numbers[i] = this->numbers[i-1]; }
            this->numbers[i].value = newValue;
            this->numbers[i].insertionIndex = windowSize - 1;

            return this->numbers[this->middleIndex].value;
        }

    private:
        struct number{
            int insertionIndex;
            double value;
        };

        const int windowSize;
        int middleIndex;

        number* numbers;
};

class MovingAverage{
    public:
        MovingAverage(const int windowSize): windowSize(windowSize){
            this->numbers = (double*)malloc(sizeof(double) * windowSize);
        }

        ~MovingAverage(){
            free(this->numbers);
        }

        // initialize the moving median
        // array must be at least 'windowSize' elements long
        double initialize(double* array){
            memcpy(this->numbers, array, sizeof(double)*this->windowSize);

            this->sum = 0;
            for(int i=0; i<windowSize; i++){ sum += array[i]; }

            return this->sum / this->windowSize;
        }

        double process(double newValue){
            this->sum = this->sum - this->numbers[0] + newValue;
            memmove(this->numbers, this->numbers+1, sizeof(double)*(this->windowSize-1));
            return this->sum / this->windowSize;
        }

    private:
        const int windowSize;
        double* numbers;
        double sum;
};

class ImageFragment{
    public:
        ImageFragment(unsigned char* data, int dataLen):data(data), dataLen(dataLen){}

        // TODO: if auto correlation doesnt produce a confident results, it might be that the segment is too short and needs the old method
        SuspectedValueList detectWidth(const int windowSize=10, const int scoreTop=3){
            const int bytesPerPixel = 3;

            // convolve the score function over the data
            int distsLen = this->dataLen-1;
            unsigned int paddedLen = pow(2, ceil(log2(distsLen)));
            double* paddedDists = (double*)calloc(sizeof(__complex__ double), paddedLen);

            // populate the dists array
            // calculate mean and mean squared for later
            double meanSquared;
            double mean = 0;
            for(int i=0; i<distsLen; i++){
                paddedDists[i<<1] = (this->data[i+1] - this->data[i]) / 255.0;
                mean += paddedDists[i<<1];
            }
            mean /= distsLen;
            meanSquared = mean * mean;

            // calculate the squared standard deviation of the dists
            double stdDevSquared = 0;
            for(int i=0; i<distsLen; i++){ paddedDists[i<<1] -= mean; stdDevSquared += (paddedDists[i<<1])*(paddedDists[i<<1]); }
            stdDevSquared = stdDevSquared / distsLen;

            // if stdDevSquared is 0, than there is no variation in the image and its width cannot be detected
            if(stdDevSquared < 0.000000000000001){ return -1; }

            // convert paddedDists to a complex number array
            __complex__ double* data = (__complex__ double*)paddedDists;

            // np.fft.fft(x)
            fft(data, paddedLen);

            // convert to magnitude
            for(int i=0; i<paddedLen; i++){ data[i] = creal(data[i])*creal(data[i]) + cimag(data[i])*cimag(data[i]); }

            // np.fft.ifft(x)
            ifft(data, paddedLen);

            // keep the correlation at every multiple of 'bytesPerPixel'
            // (x.real/x.shape-np.mean(x)**2)/np.std(x)**2
            for(int i=0; i<distsLen/bytesPerPixel; i++){ paddedDists[i] = (creal(data[i*bytesPerPixel]) / distsLen - meanSquared) / stdDevSquared; }
            distsLen /= bytesPerPixel;

            // simple smoothing function
            // TODO: this could be removed with relatively minimal impact on results
            for(int i=0; i<distsLen-windowSize; i++){
                paddedDists[i] /= 2;
                double factor = 0.5;
                for(int j=1; j<windowSize; j++){
                    factor /= 2;
                    paddedDists[i] += paddedDists[i + j] * factor;
                }
            }
            distsLen -= windowSize;

            // get the second dirivative of the auto correlation to discover concavity
            double* concavity = (double*)malloc(sizeof(double) * distsLen);
            for(int i=0; i<distsLen-windowSize; i++){ concavity[i] = ImageFragment::getSlope(paddedDists+i, windowSize); } // first derivative
            for(int i=0; i<distsLen-windowSize*2; i++){ concavity[i] = ImageFragment::getSlope(concavity+i, windowSize); } // second derivative
            distsLen -= windowSize*2;

            // find the maximum difference of gradients on convex points
            SuspectedValueList svl(3);
            svl.setIgnoreMultiples(true);

            for(int i=0; i<distsLen-windowSize*2; i++){
                paddedDists[i] *= concavity[i]>0 ? 0 : -concavity[i];
            }


            for(int i=1; i<distsLen-windowSize*2-1; i++){
                if(paddedDists[i] > paddedDists[i-1] && paddedDists[i] > paddedDists[i+1]){
                    bool kept = svl.process(i+windowSize-1, paddedDists[i]);
                }
            }

            free(paddedDists);
            svl.calcConfidence();
            return svl;
        }

        SuspectedValueList detectOffset(int width, const int scoreTop=3){
            const int bytesPerPixel = 3;
            const int windowSize = 3; // TODO this means an image can not be under 10 pixels wide

            int byteWidth = width * bytesPerPixel;
            int windowSize2 = windowSize << 1;

            double* avgValW = (double*)malloc(sizeof(double) * bytesPerPixel * 3);
            double* avgValR = avgValW + bytesPerPixel;
            double* avgValL = avgValW + bytesPerPixel*2;

            SuspectedValueList svl(3);

            for(int i=0; i<byteWidth-windowSize2-1; i++){                
                double avgVarR = 0.0001;
                double avgVarL = 0.0001;
                double avgVarW = 0;

                for(int j=0; j<(this->dataLen-i-windowSize2) / byteWidth; j++){
                    unsigned char* side1 = this->data + (i+j*byteWidth);
                    unsigned char* side2 = side1 + bytesPerPixel*windowSize;

                    memset(avgValW, 0, sizeof(double) * bytesPerPixel * 2);

                    for(int l=0; l<bytesPerPixel; l++){
                        for(int k=0; k<windowSize; k++){
                            avgValW[l] += side1[k*bytesPerPixel+l] + side2[k*bytesPerPixel+l];
                            avgValL[l] += side1[k*bytesPerPixel+l];
                            avgValR[l] += side2[k*bytesPerPixel+l];
                        }

                        avgValW[l] /= windowSize2;
                        avgValL[l] /= windowSize;
                        avgValR[l] /= windowSize;
                    }

                    for(int k=0; k<windowSize; k++){
                        for(int l=0; l<bytesPerPixel; l++){
                            double difL = (side1[k*bytesPerPixel + l] - avgValL[l]) / 256.0;
                            double difR = (side2[k*bytesPerPixel + l] - avgValR[l]) / 256.0;

                            double difW1 = (side1[k*bytesPerPixel + l] - avgValW[l]) / 256.0;
                            double difW2 = (side2[k*bytesPerPixel + l] - avgValW[l]) / 256.0;

                            avgVarW += ((difW1 * difW1) + (difW2 * difW2)) / 2;
                            avgVarL += (difL * difL);
                            avgVarR += (difR * difR);
                        }
                    }
                }

                double score = (1/avgVarR) * (1/avgVarL) * (avgVarW);
                svl.process(byteWidth - bytesPerPixel*windowSize - i, score);
            }

            free(avgValW);

            svl.calcConfidence();
            return svl;
        }

    private:
        static double getSlope(double* arr, int len){
            double sum1 = 0;
            double sum2 = 0;

            int i = 0;
            for(; i<len>>1; i++){ sum1 += arr[i]; }
            for(; i<len; i++){ sum2 += arr[i]; }

            return (sum2-sum1) / len;




            // double dif = 0;
            // for(int i=0; i<len-1; i++){
            //     dif += arr[i+1] - arr[i];
            // }

            // return dif / (len-1);
        }

        unsigned char* data;
        unsigned int dataLen;

};

#endif