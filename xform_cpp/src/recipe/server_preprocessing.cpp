/* -----------------------------------------------------------------
 * File:    server_preprocessing.cpp
 * Author:  Michael Gharbi <gharbi@mit.edu>
 * Created: 2015-07-27
 * -----------------------------------------------------------------
 * 
 * 
 * 
 * ---------------------------------------------------------------*/


#include "recipe/server_preprocessing.h"
#include <cmath>
#include "recipe/hl_upsample.h"
#include "recipe/hl_gaussian_downsample.h"
#include "recipe/hl_laplacian_pyramid_level_with_noise.h"
#include "recipe/hl_collapse_pyramid.h"
#include "utils/histogram/histogram.h"
#include "utils/histogram/histogram_io.h"
#include "utils/image_io.h"

#include "perf_measure.h"
#include "print_helper.h"

#include <cmath>

Image<uint32_t> server_preprocessing(
        Image<uint32_t> &raw_input,
        const uint8_t* hdata, unsigned long hdatasize,
        const char* noise_data, int noise_datasize,
        int upsampling_factor, 
        int width,
        int height
) {
    int noisesize = sqrt(noise_datasize/sizeof(float));
    buffer_t noise_buffer;
    noise_buffer.host = (uint8_t*) noise_data;
    noise_buffer.extent[0] = noisesize;
    noise_buffer.extent[1] = noisesize;
    noise_buffer.extent[2] = 0;
    noise_buffer.extent[3] = 0;
    noise_buffer.stride[0] = 1;
    noise_buffer.stride[1] = noisesize;
    noise_buffer.stride[2] = noisesize*noisesize;
    noise_buffer.stride[3] = 0;
    noise_buffer.elem_size = sizeof(float);

    // upsampled input
    auto start = get_time();
    Image<uint32_t> upsampled(width, height);
    hl_upsample(raw_input, upsampling_factor,upsampled);
    PRINT("  - upsample %ldms\n", get_duration(start,get_time()));

    // Get histograms from input
    vector<vector<Histogram> > input_hist = decompress_histogram(hdata, hdatasize);

    int nhists = input_hist.size();
    int nlevels = nhists-1;
    int nbins   = input_hist[0][0].nbins;

    // downsample nlevels times and get laplacian histograms
    start = get_time();
    vector<Image<uint32_t> >    ds(nlevels);
    vector<Histogram>           hists;
    vector<Image<float> >       outPyramid(3);

    for (int i = 0; i < nlevels; ++i) {
        ds[i] = Image<uint32_t>(width/pow(2,i+1), height/pow(2,i+1));
        Image<float> level;
        if(i == 0) {
            hl_gaussian_downsample(upsampled,ds[i]);
            level = Image<float>(upsampled.width(), upsampled.height(), 3);
            hl_laplacian_pyramid_level_with_noise(upsampled,ds[i],&noise_buffer, level);
            hists = histograms(level,nbins);
        } else {
            level = Image<float>(ds[i-1].width(), ds[i-1].height(),3);
            hl_gaussian_downsample(ds[i-1],ds[i]);
            hl_laplacian_pyramid_level_with_noise(ds[i-1],ds[i],&noise_buffer,level);
            hists = histograms(level,nbins);
        }
        vector<TransferFunction> tfs;
        for (int j = 0; j < 3; ++j) {
            TransferFunction f(input_hist[i][j],hists[j]);
            tfs.insert(tfs.end(),f);
        }
        outPyramid[i] = level;
        // outPyramid[i] = transfer(level,tfs);
    }

    PRINT("  - laplacian histograms transfer %ldms\n", get_duration(start,get_time()));

    Image<uint32_t> &lp_residual = ds[nlevels-1];

    // collapse pyramid
    start = get_time();
    hl_collapse_pyramid(lp_residual, outPyramid[2], outPyramid[1],outPyramid[0],upsampled);
    PRINT("  - collapse pyramid %ldms\n", get_duration(start,get_time()));

    // transfer histogram of colors
    start = get_time();
    vector<Histogram> vhist = histograms(upsampled,nbins);
    vector<TransferFunction> tfs;
    for (int j = 0; j < 3; ++j) {
        TransferFunction f(input_hist[3][j],vhist[j]);
        tfs.insert(tfs.end(),f);
    }
    Image<uint32_t> final = transfer(upsampled,tfs);
    PRINT("  - color histogram transfer %ldms\n", get_duration(start,get_time()));

    // return final;
    return upsampled;
}
