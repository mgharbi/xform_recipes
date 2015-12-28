/* -----------------------------------------------------------------
 * File:    histogram.cpp
 * Author:  Michael Gharbi <gharbi@mit.edu>
 * Created: 2015-07-27
 * -----------------------------------------------------------------
 * 
 * 
 * 
 * ---------------------------------------------------------------*/


#include "utils/histogram/histogram.h"
#include "utils/histogram/hl_transfer_histogram.h"
#include "utils/histogram/hl_transfer_histogram_color.h"
#include "utils/histogram/hl_transfer_histogram_color_float.h"
#include "utils/histogram/hl_histogram.h"
#include "utils/histogram/hl_histogram_color.h"
#include "utils/histogram/hl_histogram_color_float.h"
#include "utils/histogram/hl_histogram_of_gradients.h"
#include "utils/Histogram/hl_histogram_of_laplacian_coefficients.h"

#include "print_helper.h"


Histogram::Histogram(const Image<float> &halide_hist) : nbins(halide_hist.width()-2) {
    // 2 floats for mini and max at beginning
    count = vector<float>(nbins);
    memcpy(&count[0], halide_hist.data() + 2, nbins*sizeof(float));

    mini = halide_hist(0);
    maxi = halide_hist(1);

    init();
}


Histogram::Histogram(const float* src, int nbins) : nbins(nbins) {
    // 2 floats for mini and max at beginning
    count = vector<float>(nbins);
    memcpy(&count[0], src + 2, nbins*sizeof(float));

    mini = src[0];
    maxi = src[1];

    init();
}


void Histogram::init() {

    cdf = vector<float>(nbins);
    cdf[0] = count[0];
    sum = count[0];
    for (int i = 1; i < nbins; ++i) {
        cdf[i] = cdf[i-1] + count[i];
        sum += count[i];
    }
    for (int i = 0; i < nbins; ++i) {
        cdf[i] /= sum;
    }
}


TransferFunction::TransferFunction(const Histogram &source, const Histogram &target) {
    size = source.nbins+1;
    values = vector<float>(size);
    values[source.nbins] = 1.0f;

    const vector<float> &cdf_s = source.cdf;
    const vector<float> &cdf_t = target.cdf;
    int t_current = 0;
    for (int i = 0; i < source.nbins; ++i) {
        // TODO: clamping here?
        while(cdf_t[t_current] < cdf_s[i]) {
            ++t_current;
        }
        // values[i] = i *1.0f / source.nbins;
        values[i] = static_cast<float>(t_current) / target.nbins;
    }

    source_mini = source.mini;
    source_maxi = source.maxi;

    target_mini = target.mini;
    target_maxi = target.maxi;
}


Image<float> transfer(Image<float> &image, TransferFunction &f) {
    Image<float> h_f(f.size);
    memcpy(h_f.data(),&f.values[0], f.size*sizeof(float));

    Image<float> output(image.width(), image.height());
    hl_transfer_histogram(image, h_f, f.source_mini, f.source_maxi, f.target_mini, f.target_maxi, output);

    return output;
}


Image<float> transfer(Image<float> &image, vector<TransferFunction> &fs) {
    Image<float> h_f(fs[0].size+4,3);
    float *pData = h_f.data();

    int idx = 0;
    for (size_t i = 0; i < fs.size(); ++i) {
        pData[idx++] = fs[i].source_mini;
        pData[idx++] = fs[i].source_maxi;
        pData[idx++] = fs[i].target_mini;
        pData[idx++] = fs[i].target_maxi;
        memcpy(pData+idx,&fs[i].values[0], fs[i].size*sizeof(float));
        idx += fs[i].size;
    }
    Image<float> output(image.width(), image.height(),3);
    hl_transfer_histogram_color_float(image, h_f, output);
    return output;
}


Image<uint32_t> transfer(Image<uint32_t> &image, vector<TransferFunction> &fs) {
    Image<float> h_f(fs[0].size+4,3);
    float *pData = h_f.data();

    int idx = 0;
    for (size_t i = 0; i < fs.size(); ++i) {
        pData[idx++] = fs[i].source_mini;
        pData[idx++] = fs[i].source_maxi;
        pData[idx++] = fs[i].target_mini;
        pData[idx++] = fs[i].target_maxi;
        memcpy(pData+idx,&fs[i].values[0], fs[i].size*sizeof(float));
        idx += fs[i].size;
    }
    Image<uint32_t> output(image.width(), image.height(),3);
    hl_transfer_histogram_color(image, h_f, output);
    return output;
}


// Compute histogram from a single channel float image
Histogram histogram(Image<float> &input, int nbins, int skip) {
    Image<float> h_hist(nbins+2); // 2 floats for mini and max at beginning
    hl_histogram(input, nbins,skip, h_hist);
    Histogram h = Histogram(h_hist);
    return h;
}


// Compute 3 histograms from a packed RGB image
vector<Histogram> histograms(Image<uint32_t> &input, int nbins, int skip) {
    Image<float> h_hist(nbins+2,3); // 2 floats for mini and max at beginning
    hl_histogram_color(input, nbins, skip, h_hist);
    vector<Histogram> ret;

    Histogram h = Histogram(h_hist.data(),nbins);
    ret.insert(ret.end(),h);

    h = Histogram(h_hist.data()+h_hist.width(),nbins);
    ret.insert(ret.end(),h);

    h = Histogram(h_hist.data()+2*h_hist.width(),nbins);
    ret.insert(ret.end(),h);

    return ret;
}


vector<Histogram> histograms(Image<float> &input, int nbins, int skip) {
    Image<float> h_hist(nbins+2,3); // 2 floats for mini and max at beginning
    hl_histogram_color_float(input, nbins, skip, h_hist);
    vector<Histogram> ret;

    Histogram h = Histogram(h_hist.data(),nbins);
    ret.insert(ret.end(),h);

    h = Histogram(h_hist.data()+h_hist.width(),nbins);
    ret.insert(ret.end(),h);

    h = Histogram(h_hist.data()+2*h_hist.width(),nbins);
    ret.insert(ret.end(),h);

    return ret;
}


vector<Histogram> histograms_of_laplacian(Image<uint32_t> &input, Image<uint32_t> &ds2, int nbins, int skip) {
    Image<float> h_hist(nbins+2,3); // 2 floats for mini and max at beginning
    hl_histogram_of_laplacian_coefficients(input, ds2, nbins, h_hist);
    vector<Histogram> ret;

    Histogram h = Histogram(h_hist.data(),nbins);
    ret.insert(ret.end(),h);

    h = Histogram(h_hist.data()+h_hist.width(),nbins);
    ret.insert(ret.end(),h);

    h = Histogram(h_hist.data()+2*h_hist.width(),nbins);
    ret.insert(ret.end(),h);

    return ret;
}


Histogram histogram_of_gradients(Image<float> &input, int nbins, int skip) {
    Image<float> h_hist(nbins+2);
    hl_histogram_of_gradients(input, nbins, h_hist);
    return Histogram(h_hist);
}
