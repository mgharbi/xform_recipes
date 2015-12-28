/* -----------------------------------------------------------------
 * File:    Recipe.cpp
 * Author:  Michael Gharbi <gharbi@mit.edu>
 * Created: 2015-07-24
 * -----------------------------------------------------------------
 * 
 * 
 * 
 * ---------------------------------------------------------------*/


#include <cmath>
#include <algorithm>

#ifndef __ANDROID__
    #include <armadillo>
    #include "utils/image_io.h"
#endif

// HALIDE
#include "utils/static_image.h"

#include "recipe/hl_reconstruct.h"
#include "recipe/hl_lowpass.h"
#include "recipe/hl_highpass.h"
#include "recipe/hl_compute_features.h"
#include "recipe/hl_precompute_pyramid.h"

#include "print_helper.h"
#include "perf_measure.h"


#include "recipe/Recipe.h"

namespace xform {

Recipe::Recipe( Image<uint32_t>& unprocessed ) {
    m_model_width  = static_cast<int>(ceil(2.0f*unprocessed.width()/XFORM_WSIZE));
    m_model_height = static_cast<int>(ceil(2.0f*unprocessed.height()/XFORM_WSIZE));
    m_lp_width     = unprocessed.width()/XFORM_WSIZE;
    m_lp_height    = unprocessed.height()/XFORM_WSIZE;
    m_unprocessed_channels = 3;
    m_processed_channels   = 3;
}

#ifndef __ANDROID__
Recipe::Recipe( Image<uint32_t>& unprocessed, Image<uint32_t>& processed ):
    Recipe(unprocessed)
{
    fit(unprocessed, processed);
}
#endif

#ifndef __ANDROID__
void Recipe::fit( Image<uint32_t>& unprocessed, Image<uint32_t>& processed )
{
    PRINT("Fit recipe\n");
    m_lp_residual = std::make_shared<Image<uint32_t> >(m_lp_width,m_lp_height,m_processed_channels);
    m_hp_coefs    = std::make_shared<Image<float> >(m_model_width,m_model_height,nCoefMaps());
    m_qtable      = std::vector<float>(2*nCoefMaps());

    int width  = unprocessed.width();
    int height = unprocessed.height();

    Image<float> processed_hp(width, height, 3);

    // Compute and store lowpass of the processed image
    auto start = get_time();
    hl_lowpass(processed, *m_lp_residual);
    PRINT("  - lowpass:\t %ldms\n", get_duration(start,get_time()));

    // Compute highpass of the output
    start = get_time();
    hl_highpass(processed, *m_lp_residual, processed_hp);
    PRINT("  - highpass:\t %ldms\n", get_duration(start,get_time()));

    // Features
    start = get_time();
    Image<float> features(width, height, nFeatureChannels());
    hl_compute_features(unprocessed, features);
    PRINT("  - features:\t %ldms\n", get_duration(start,get_time()));

    start = get_time();
    regression(features, processed_hp);
    PRINT("  - regression:\t %ldms\n", get_duration(start,get_time()));

    quantize();
}
#endif


void Recipe::precompute_features( const Image<uint32_t>& unprocessed)
{
    m_lp_unprocessed = Image<uint32_t>(m_lp_width, m_lp_height);
    m_hp_unprocessed = Image<float>(unprocessed.width(), unprocessed.height(), 3);

    auto start = get_time();
    hl_lowpass(unprocessed, m_lp_unprocessed);
    PRINT("  - lowpass : %ldms\n", get_duration(start,get_time()));

    start = get_time();
    hl_highpass(unprocessed, m_lp_unprocessed, m_hp_unprocessed);
    PRINT("  - highpass : %ldms\n", get_duration(start,get_time()));

    start = get_time();
    m_pyramid_unprocessed = Image<float>(unprocessed.width(), unprocessed.height(), nPyrLevels()-1);
    hl_precompute_pyramid(unprocessed,m_pyramid_unprocessed);
    PRINT("  - precompute pyramid : %ldms\n", get_duration(start,get_time()));
}


void Recipe::reconstruct_with_features( Image<uint32_t> &output )
{
    dequantize();

    output = Image<uint32_t>(m_pyramid_unprocessed.width(),
            m_pyramid_unprocessed.height(), m_processed_channels);

    auto start = get_time();
    hl_reconstruct(
            m_pyramid_unprocessed,
            m_hp_unprocessed,
            *m_hp_coefs,
            *m_lp_residual,
            output
    );
    PRINT("  - reconstruct precomputed: %ldms\n", get_duration(start,get_time()));
}


void Recipe::reconstruct_image( const Image<uint32_t>& unprocessed, Image<uint32_t> &output )
{
    precompute_features(unprocessed);
    reconstruct_with_features(output);
}


#ifndef __ANDROID__
void Recipe::regression( Image<float>& features, Image<float>& target) 
{
    const int h_mdl           = m_model_height;
    const int w_mdl           = m_model_width;
    const int h               = target.height();
    const int w               = target.width();
    const int n_targets       = target.channels();
    const int n_lumCoefs      = nLumCoefs();
    const int n_chromCoefs    = nChromCoefs();
    const int n_features      = features.channels();

    float* pDataFeat   = features.data();
    float* pDataTarget = target.data();
    float* pLuminCoef  = m_hp_coefs->data();
    float* pChromCoef  = pLuminCoef + h_mdl*w_mdl*n_lumCoefs;

    int wSize = XFORM_WSIZE;
    int step  = XFORM_WSIZE/2;

    #pragma omp parallel for
    for (int patch_idx = 0; patch_idx < h_mdl*w_mdl; ++patch_idx)
    {
        // temporary storage for the current patch
        float* X = new float[wSize*wSize*n_lumCoefs]();
        float* Y = new float[wSize*wSize*n_targets]();

        // Patch indices (in the recipe)
        int patch_x = patch_idx % w_mdl;
        int patch_y = patch_idx / w_mdl;

        // Pixel indices (in the image)
        int x_min   = patch_x*step;
        int x_max   = min(x_min+wSize, w);
        int y_min   = patch_y*step;
        int y_max   = min(y_min+wSize, h);
        int n_samples = (y_max-y_min)*(x_max-x_min);

        // Copy patch features
        int idx = 0;
        for (int f = 0; f < n_features; ++f)
        for (int y = y_min; y < y_max; ++y)
        for (int x = x_min; x < x_max; ++x)
        {
            X[idx] = pDataFeat[x+y*w+f*h*w];
            ++idx;
        }

        // Make features for the non-linear luminance curve
        int first_curve = idx;
        auto minmax     = std::minmax_element(X,X + n_samples);
        if(!XFORM_ADAPTIVE_LUMA_RANGE) {
            *minmax.first = 0.0f;
            *minmax.second = 1.0f;
        }
        float step_sz   = (*minmax.second-*minmax.first)/XFORM_LUMA_BANDS;
        float mini      = *minmax.first;
        for (int s = 0; s < XFORM_LUMA_BANDS-1; ++s){
            float thresh = mini+(s+1)*step_sz;
            for (idx = 0; idx < n_samples; ++idx)
            {
                X[first_curve + n_samples*s + idx] = std::max(X[idx]-thresh, 0.0f);
            }
        }

        // Copy target variables
        idx = 0;
        for (int f = 0; f < n_targets; ++f)
        for (int y = y_min; y < y_max; ++y)
        for (int x = x_min; x < x_max; ++x)
        {
            Y[idx] = pDataTarget[x+y*w+f*h*w];
            ++idx;
        }

        // Solve least-square regression
        arma::Mat<float> matX(X, n_samples,n_lumCoefs, false);
        arma::Mat<float> matY(Y, n_samples,n_targets, false);

        // For the luminance
        arma::Mat<float> result_lumin;
        arma::Mat<float> Xview = matX.cols(0,n_lumCoefs-1);
        arma::Mat<float> Yview = matY.cols(0,0);
        arma::Mat<float> regularizer = arma::eye<arma::Mat<float>>(n_lumCoefs, n_lumCoefs);
        regularizer(3,3) = 0.0f; //do not regularize affine offset
        arma::Mat<float> lhs = arma::trans(Xview) * Xview + XFORM_EPSILON*regularizer;
        arma::Mat<float> rhs = arma::trans(Xview) * Yview;
        arma::solve(result_lumin,lhs,rhs);

        // For the chrominance
        arma::Mat<float> result_chrom;
        Xview = matX.cols(0,n_chromCoefs-1);
        Yview = matY.cols(1,2);
        regularizer = arma::eye<arma::Mat<float>>(n_chromCoefs, n_chromCoefs);
        regularizer(3,3) = 0.0f; //do not regularize affine offset
        lhs = arma::trans(Xview) * Xview + XFORM_EPSILON*regularizer;
        rhs = arma::trans(Xview) * Yview;
        arma::solve(result_chrom,lhs,rhs);

        // Fill-in the luminance regression coefficients
        float* coefs = result_lumin.memptr();
        idx = 0;
        for (int f = 0; f < n_lumCoefs; ++f)
        {
            pLuminCoef[patch_x+patch_y*w_mdl+f*h_mdl*w_mdl] = coefs[idx];
            ++idx;
        }

        // Fill in the chrominance regression coefficients
        coefs = result_chrom.memptr();
        idx = 0;
        for (int c = 0; c < 2; ++c)
        for (int f = 0; f < n_chromCoefs; ++f)
        {
            pChromCoef[patch_x + patch_y*w_mdl + f*h_mdl*w_mdl + c*n_chromCoefs*h_mdl*w_mdl] = coefs[idx];
            ++idx;
        }
        coefs = nullptr;

        delete[] X; X = nullptr;
        delete[] Y; Y = nullptr;
    }
    pDataTarget = nullptr;
    pDataFeat   = nullptr;
}
#endif 


void Recipe::quantize() 
{
    const int h_mdl   = m_model_height;
    const int w_mdl   = m_model_width;
    const int n_feats = nCoefMaps();

    // Get values range per regression channel
    float* pCoef = m_hp_coefs->data();

    // Compute min and max for each channel
    for (int f = 0; f < n_feats; ++f){
        m_qtable[2*f]   = pCoef[f*h_mdl*w_mdl];
        m_qtable[2*f+1] = pCoef[f*h_mdl*w_mdl];
        for (int y = 0; y < h_mdl; ++y)
        for (int x = 0; x < w_mdl; ++x)
        {
            float val = pCoef[x + y*w_mdl + f*h_mdl*w_mdl];
            // min
            m_qtable[2*f] = min(m_qtable[2*f],val);
            // max
            m_qtable[2*f+1] = max(m_qtable[2*f+1],val);
        }
    }

    // Quantize
    for (int f = 0; f < n_feats; ++f)
    {
        float min = (m_qtable)[2*f];
        float max = (m_qtable)[2*f+1];
        float inv_rng = 1.0f/(max-min);
        for (int y = 0; y < h_mdl; ++y)
        for (int x = 0; x < w_mdl; ++x)
        {
            float *val = pCoef + (x+y*w_mdl+f*h_mdl*w_mdl);
            *val -= min;
            *val *= inv_rng;
            *val = floor(255.0f*(*val)+0.5f);
        }
    }
}


void Recipe::dequantize() 
{
    const int h_mdl        = m_model_height;
    const int w_mdl        = m_model_width;
    const int n_feats = nCoefMaps();

    float* pCoef  = m_hp_coefs->data();

    // Dequantize
    for (int f = 0; f < n_feats; ++f)
    {
        float min = m_qtable[2*f];
        float max = m_qtable[2*f+1];
        float rng = (max-min)/255.0f;
        for (int y = 0; y < h_mdl; ++y)
        for (int x = 0; x < w_mdl; ++x) {
            float *val = pCoef + (x+y*w_mdl+f*h_mdl*w_mdl);
            *val *= rng;
            *val += min;
        }
    }
}

} //namespace xform
