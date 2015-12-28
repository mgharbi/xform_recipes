#include "utils/histogram/histogram_io.h"
#include <zlib.h>
#include <cstring>
#include "print_helper.h"

void compress_histogram(vector<vector<Histogram> > histograms, unsigned char **data, unsigned long* datasize) {
    int nhistograms = histograms.size();
    int ncolors = histograms[0].size();
    int nbins = histograms[0][0].nbins;

    unsigned long hdata_size = ncolors*(nhistograms)*(nbins+2);
    float *hdata             = new float[hdata_size];

    // pack histogram
    int idx = 0;
    for (int i = 0; i < nhistograms; ++i) {
        for (int j = 0; j < ncolors; ++j) {
            Histogram &h = histograms[i][j];
            hdata[idx++] = h.mini;
            hdata[idx++] = h.maxi;
            memcpy(hdata+idx, &h.count[0], sizeof(float)*h.nbins);
            idx += h.nbins;
        }
    }

    *datasize = compressBound(hdata_size*sizeof(float));
    *data = new uint8_t[*datasize + 3*sizeof(int)]();

    int* header = (int*) *data;
    header[0] = nbins;
    header[1] = nhistograms;
    header[2] = ncolors;

    compress((*data)+3*sizeof(int), datasize, (uint8_t*) hdata, hdata_size*sizeof(float));
    *datasize += 3*sizeof(int);

    delete[] hdata;
}

vector<vector<Histogram> > decompress_histogram(const unsigned char* data, unsigned long datasize) {
    int* header = (int*) data;
    int nbins       = header[0];
    int nhistograms = header[1];
    int ncolors     = header[2];

    unsigned long hdata_size = ncolors*(nhistograms)*(nbins+2)*sizeof(float);
    float *hdata             = new float[hdata_size]();

    int retval = uncompress((uint8_t*) hdata, &hdata_size, data+3*sizeof(int), datasize-3*sizeof(int));
    if(retval != Z_OK) {
        PRINT(" >>> ZLIB uncompress error %d\n", retval);
    }

    // unpack histogram
    vector<vector<Histogram> > histograms(nhistograms);
    int idx = 0;
    for (int i = 0; i < nhistograms; ++i) {
        histograms[i] = vector<Histogram>();
        for (int j = 0; j < ncolors; ++j) {
            Histogram h(hdata+idx, nbins);
            histograms[i].insert(histograms[i].end(),h);
            idx += h.nbins+2;
        }
    }

    delete[] hdata;

    return histograms;
}
