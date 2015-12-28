/* -----------------------------------------------------------------
 * File:    histogram.h
 * Author:  Michael Gharbi <gharbi@mit.edu>
 * Created: 2015-07-27
 * -----------------------------------------------------------------
 * 
 * 
 * 
 * ---------------------------------------------------------------*/


#ifndef HISTOGRAM_H_YVEGPJZX
#define HISTOGRAM_H_YVEGPJZX

#include "utils/static_image.h"

#include <vector>
#include <cstring>

#define HISTOGRAM_SUBSAMPLING 4

using std::vector;

class Histogram
{
public:
    Histogram(const Image<float> &halide_hist);
    Histogram(const float* src, int nbins);

    int nbins;
    vector<float> count;
    vector<float> cdf;
    int sum;

    float mini;
    float maxi;

private:
    void init();
};


class TransferFunction
{
public:
    TransferFunction(const Histogram &source, const Histogram &target);

    int size;
    float source_mini;
    float source_maxi;
    float target_mini;
    float target_maxi;
    vector<float> values;
};


Histogram         histogram (Image<float>    &input, int nbins, int skip = HISTOGRAM_SUBSAMPLING);
vector<Histogram> histograms(Image<uint32_t> &input, int nbins, int skip = HISTOGRAM_SUBSAMPLING);
vector<Histogram> histograms(Image<float>    &input, int nbins, int skip = HISTOGRAM_SUBSAMPLING);
vector<Histogram> histograms_of_laplacian(Image<uint32_t> &input, Image<uint32_t> &ds2, int nbins, int skip = HISTOGRAM_SUBSAMPLING);
Histogram         histogram_of_gradients(Image<float> &input, int nbins, int skip = HISTOGRAM_SUBSAMPLING);

Image<float>    transfer(Image<float> &image, TransferFunction &f);
Image<float>    transfer(Image<float> &image, vector<TransferFunction> &fs);
Image<uint32_t> transfer(Image<uint32_t> &image, vector<TransferFunction> &fs);

#endif /* end of include guard: HISTOGRAM_H_YVEGPJZX */
