#ifndef FFT_H
#define FFT_h

#include <complex.h>

#ifndef M_PI
    #define M_PI 3.14159265358979323846264338328L
#endif

  
// typedef std::complex<double> Complex;
 
// Cooley-Tukey FFT (in-place, breadth-first, decimation-in-frequency)
void fft(__complex__ double* x, unsigned int N){
    // DFT
    unsigned int k = N;
    unsigned int n;
    double thetaT = M_PI / N;
    __complex__ double phiT = cos(thetaT) -sin(thetaT)*I;
    __complex__ double T;
    while(k > 1){
        n = k;
        k >>= 1;
        phiT = phiT * phiT;
        T = 1.0L;
        for(unsigned int l=0; l<k; l++){
            for (unsigned int a=l; a<N; a+=n){
                unsigned int b = a + k;
                __complex__ double t = x[a] - x[b];
                x[a] += x[b];
                x[b] = t * T;
            }
            T *= phiT;
        }
    }

    // Decimate
    unsigned int m = (unsigned int)log2(N);
    for(unsigned int a=0; a<N; a++){
        unsigned int b = a;
        // Reverse bits
        b = (((b & 0xaaaaaaaa) >> 1) | ((b & 0x55555555) << 1));
        b = (((b & 0xcccccccc) >> 2) | ((b & 0x33333333) << 2));
        b = (((b & 0xf0f0f0f0) >> 4) | ((b & 0x0f0f0f0f) << 4));
        b = (((b & 0xff00ff00) >> 8) | ((b & 0x00ff00ff) << 8));
        b = ((b >> 16) | (b << 16)) >> (32 - m);
        if(b > a){
            __complex__ double t = x[a];
            x[a] = x[b];
            x[b] = t;
        }
    }
}
 
// inverse fft (in-place)
void ifft(__complex__ double* x, int len){

    // conjugate the complex numbers
    for(int i=0; i<len; i++){ x[i] = -creal(x[i]) - cimag(x[i])*I; }
 
    // forward fft
    fft(x, len);
 
    // conjugate the complex numbers again
    for(int i=0; i<len; i++){ x[i] = -creal(x[i]) - cimag(x[i])*I; }
 
    // scale the numbers
    for(int i=0; i<len; i++){ x[i] /= len; }
}

#endif