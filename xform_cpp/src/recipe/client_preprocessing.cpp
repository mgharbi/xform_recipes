/* -----------------------------------------------------------------
 * File:    client_preprocessing.cpp
 * Author:  Michael Gharbi <gharbi@mit.edu>
 * Created: 2015-07-24
 * -----------------------------------------------------------------
 * 
 * 
 * 
 * ---------------------------------------------------------------*/

#include "recipe/hl_gaussian_downsample.h"
#include "recipe/client_preprocessing.h"
#include "utils/histogram/histogram.h"
#include "utils/histogram/histogram_io.h"
#include "perf_measure.h"

#include <cstdio>
#include <cmath>
#include <vector>

#include "print_helper.h"

using std::vector;

#ifndef __ANDROID__
    #include "utils/image_io.h"
#endif

void client_preprocessing(Image<uint32_t> input, uint8_t **hdata, unsigned long * hdatasize) {
    int nbins = 256;
    int nlevels = 3;

    int w = input.width();
    int h = input.height();

    // downsample nlevels times and get laplacian histograms
    auto start = get_time();
    vector<Image<uint32_t> >ds(nlevels);
    vector<vector<Histogram> > hists(nlevels+1);
    for (int i = 0; i < nlevels; ++i) {
        ds[i] = Image<uint32_t>(w/pow(2,i+1), h/pow(2,i+1));
        if (i==0) {
            auto start2                   = get_time();
            hl_gaussian_downsample(input,ds[i]);
            PRINT("  - Gaussian downsample %ldms\n", get_duration(start2,get_time()));
            auto start3                   = get_time();
            hists[i] = histograms_of_laplacian(input,ds[i], nbins);
            PRINT("  - Laplacian histogram %ldms\n", get_duration(start3,get_time()));
        } else {
            auto start2                   = get_time();
            hl_gaussian_downsample(ds[i-1],ds[i]);
            PRINT("  - Gaussian downsample %ldms\n", get_duration(start2,get_time()));
            auto start3                   = get_time();
            hists[i] = histograms_of_laplacian(ds[i-1],ds[i], nbins);
            PRINT("  - Laplacian histogram %ldms\n", get_duration(start3,get_time()));
        }
    }
    PRINT("  - laplacian histograms %ldms\n", get_duration(start,get_time()));

    // get histogram of colors
    start = get_time();
    hists[3] = histograms(input,nbins);
    PRINT("  - color histogram %ldms\n", get_duration(start,get_time()));

    // Copy histogram data to float pointer and gzip compress
    start = get_time();
    *hdatasize = 0;
    *hdata = nullptr;
    compress_histogram(hists, hdata, hdatasize);
}
