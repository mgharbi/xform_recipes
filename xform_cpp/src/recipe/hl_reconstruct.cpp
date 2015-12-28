/* -----------------------------------------------------------------
 * File:    hl_reconstuct.cpp
 * Author:  Michael Gharbi <gharbi@mit.edu>
 * Created: 2015-07-24
 * -----------------------------------------------------------------
 * 
 * 
 * 
 * ---------------------------------------------------------------*/

#include <Halide.h>
#include "utils/resize.h"
#include "utils/color_transform.h"
#include "utils/resample.h"
#include "global_parameters.h"

using namespace Halide;

using std::vector;
using std::pair;
using std::make_pair;

int main(void) {

    Var x("x"), y("y"), c("c");

    int nLumaBands    = XFORM_LUMA_BANDS;
    int nPyrLevels    = XFORM_PYR_LEVELS;
    int wSize         = XFORM_WSIZE;
    int step          = wSize/2;
    float scaleFactor = wSize;
    float kernel_weight = 1.0f/scaleFactor;

    int n_lumFeats   = 4+nPyrLevels-1+nLumaBands-1;
    int n_chromFeats = 4;

    ImageParam i_pyramid_features (type_of<float>(), 3);
    ImageParam i_lowpass (type_of<uint32_t>(), 2);
    ImageParam i_hp      (type_of<float>(),    3);
    ImageParam i_hp_coeff(type_of<float>(),    3);

    // Bounds of input image
    Func pyramid_features  = BoundaryConditions::repeat_edge(i_pyramid_features);
    Func lowpass  = RGBAToFloat(BoundaryConditions::repeat_edge(i_lowpass));
    Func hp       = BoundaryConditions::repeat_edge(i_hp);
    Func hp_coeff = BoundaryConditions::repeat_edge(i_hp_coeff);

    vector<Func> collapsed_laplacian(nPyrLevels);

    // Patch coordinates
    Expr patch_x = x / step;
    Expr patch_y = y / step;
    Expr dx = x % step;
    Expr dy = y % step;

    // Linear interpolation kernel
    Func K1("kernel_1D");
    Func K ("kernel_2D");
    K1(x)   = cast<float>(step-x)*kernel_weight;
    K (x,y) = K1(x)*K1(y);

    // Reduction domain for the laplacian, and bilinear interpolation weight
    // Aggregate contributions from laplacian decomposition
    for (int i=0; i<nPyrLevels-1; i++) {
        collapsed_laplacian[i](x,y) = (
                hp_coeff(patch_x-1, patch_y-1, 4+i)*K(dx,  dy  ) +
                hp_coeff(patch_x-1, patch_y  , 4+i)*K(dx,  1-dy) +
                hp_coeff(patch_x  , patch_y-1, 4+i)*K(1-dx,dy  ) +
                hp_coeff(patch_x  , patch_y  , 4+i)*K(1-dx,1-dy)
        ) * pyramid_features(x,y,i);
    }

    collapsed_laplacian[nPyrLevels-1](x,y) = collapsed_laplacian[0](x,y);
    for (int i=1; i<nPyrLevels-1; i++) {
        collapsed_laplacian[nPyrLevels-1](x,y) += collapsed_laplacian[i](x,y);
    }

    // Lumin curve features
    Func lumin_hp("lumin_hp");
    Func maxi   ("maxi");
    Func mini   ("mini");
    Func range  ("range");
    lumin_hp(x, y) = hp(x, y, 0);
    if(XFORM_ADAPTIVE_LUMA_RANGE) {
        RDom r2(0, wSize, 0, wSize, "r2");
        maxi    (x, y) = maximum(lumin_hp(step*x + r2.x, step*y + r2.y));
        mini    (x, y) = minimum(lumin_hp(step*x + r2.x, step*y + r2.y));
    }else {
        maxi    (x, y) = 1.0f;
        mini    (x, y) = 0.0f;
    }
    range   (x, y) = maxi(x, y) - mini(x, y);

    vector<Func> thresh             (nLumaBands-1);
    vector<Func> collapsed_tonecurve(nLumaBands);

    for (int i=0; i<nLumaBands-1; i++) {
        thresh[i](x, y) = cast<float>(i+1) * range(x, y) / static_cast<float>(nLumaBands) + mini(x, y);
    }

    // Aggregate contributions from the tone curve
    {
        for (int i=0; i<nLumaBands-1; i++) {
            collapsed_tonecurve[i](x,y) = (
                    hp_coeff(patch_x-1, patch_y-1, 4+nPyrLevels-1+i)*K(dx,  dy  ) * max(0.0f, lumin_hp(x,y)-thresh[i](patch_x-1, patch_y-1)) +
                    hp_coeff(patch_x-1, patch_y  , 4+nPyrLevels-1+i)*K(dx,  1-dy) * max(0.0f, lumin_hp(x,y)-thresh[i](patch_x-1, patch_y)  ) +
                    hp_coeff(patch_x  , patch_y-1, 4+nPyrLevels-1+i)*K(1-dx,dy  ) * max(0.0f, lumin_hp(x,y)-thresh[i](patch_x  , patch_y-1)) +
                    hp_coeff(patch_x  , patch_y  , 4+nPyrLevels-1+i)*K(1-dx,1-dy) * max(0.0f, lumin_hp(x,y)-thresh[i](patch_x  , patch_y)));
        }

        collapsed_tonecurve[nLumaBands-1](x,y) = collapsed_tonecurve[0](x,y);
        for (int i=1; i<nLumaBands-1; i++) {
            collapsed_tonecurve[nLumaBands-1](x,y) += collapsed_tonecurve[i](x,y);
        }
    }

    // Affine contributions
    Func lumin_affine("lumin_affine");
    Func uv_affine   ("uv_affine");
    {
        lumin_affine(x,y) =
            (
             hp_coeff(patch_x-1, patch_y-1, 0)*K(dx,  dy  ) +
             hp_coeff(patch_x-1, patch_y  , 0)*K(dx,  1-dy) +
             hp_coeff(patch_x  , patch_y-1, 0)*K(1-dx,dy  ) +
             hp_coeff(patch_x  , patch_y  , 0)*K(1-dx,1-dy))* hp(x,y,0) +
            (
             hp_coeff(patch_x-1, patch_y-1, 1)*K(dx,  dy  ) +
             hp_coeff(patch_x-1, patch_y  , 1)*K(dx,  1-dy) +
             hp_coeff(patch_x  , patch_y-1, 1)*K(1-dx,dy  ) +
             hp_coeff(patch_x  , patch_y  , 1)*K(1-dx,1-dy))* hp(x,y,1) +
            (
             hp_coeff(patch_x-1, patch_y-1, 2)*K(dx,  dy  ) +
             hp_coeff(patch_x-1, patch_y  , 2)*K(dx,  1-dy) +
             hp_coeff(patch_x  , patch_y-1, 2)*K(1-dx,dy  ) +
             hp_coeff(patch_x  , patch_y  , 2)*K(1-dx,1-dy))* hp(x,y,2) +
            (
             hp_coeff(patch_x-1, patch_y-1, 3)*K(dx,  dy  ) +
             hp_coeff(patch_x-1, patch_y  , 3)*K(dx,  1-dy) +
             hp_coeff(patch_x  , patch_y-1, 3)*K(1-dx,dy  ) +
             hp_coeff(patch_x  , patch_y  , 3)*K(1-dx,1-dy));

        uv_affine(x,y,c) =
            (
             hp_coeff(patch_x-1, patch_y-1, n_lumFeats + c*n_chromFeats + 0)*K(dx,  dy  ) +
             hp_coeff(patch_x-1, patch_y  , n_lumFeats + c*n_chromFeats + 0)*K(dx,  1-dy) +
             hp_coeff(patch_x  , patch_y-1, n_lumFeats + c*n_chromFeats + 0)*K(1-dx,dy  ) +
             hp_coeff(patch_x  , patch_y  , n_lumFeats + c*n_chromFeats + 0)*K(1-dx,1-dy))* hp(x,y,0) +
            (
             hp_coeff(patch_x-1, patch_y-1, n_lumFeats + c*n_chromFeats + 1)*K(dx,  dy  ) +
             hp_coeff(patch_x-1, patch_y  , n_lumFeats + c*n_chromFeats + 1)*K(dx,  1-dy) +
             hp_coeff(patch_x  , patch_y-1, n_lumFeats + c*n_chromFeats + 1)*K(1-dx,dy  ) +
             hp_coeff(patch_x  , patch_y  , n_lumFeats + c*n_chromFeats + 1)*K(1-dx,1-dy))* hp(x,y,1) +
            (
             hp_coeff(patch_x-1, patch_y-1, n_lumFeats + c*n_chromFeats + 2)*K(dx,  dy  ) +
             hp_coeff(patch_x-1, patch_y  , n_lumFeats + c*n_chromFeats + 2)*K(dx,  1-dy) +
             hp_coeff(patch_x  , patch_y-1, n_lumFeats + c*n_chromFeats + 2)*K(1-dx,dy  ) +
             hp_coeff(patch_x  , patch_y  , n_lumFeats + c*n_chromFeats + 2)*K(1-dx,1-dy))* hp(x,y,2) +
            (
             hp_coeff(patch_x-1, patch_y-1, n_lumFeats + c*n_chromFeats + 3)*K(dx,  dy  ) +
             hp_coeff(patch_x-1, patch_y  , n_lumFeats + c*n_chromFeats + 3)*K(dx,  1-dy) +
             hp_coeff(patch_x  , patch_y-1, n_lumFeats + c*n_chromFeats + 3)*K(1-dx,dy  ) +
             hp_coeff(patch_x  , patch_y  , n_lumFeats + c*n_chromFeats + 3)*K(1-dx,1-dy));
    }

    Func lumin_full ("lumin_full");
    Func yuv_out    ("yuv_out");
    Func rgb_out    ("rgb_out");
    Func dc_x       ("new_dc_x");
    Func dc         ("new_dc");
    Func color      ("color");
    Func output     ("output");

    // Aggregate luminance channel
    lumin_full(x, y) = lumin_affine(x,y) 
        + collapsed_laplacian[nPyrLevels-1](x,y) 
        + collapsed_tonecurve[nLumaBands-1](x,y);

    // Assemble YUV
    yuv_out(x,y,c) = select(c==0, lumin_full(x,y), uv_affine(x,y,c-1));

    // YUV2RGB
    rgb_out(x, y, c) = yuv2rgb(yuv_out)(x, y, c);

    // DC component
    dc_x (x, y, c) = resize_x(lowpass, scaleFactor)(x, y, c);
    dc   (x, y, c) = resize_y(dc_x,    scaleFactor)(x, y, c);

    color(x, y, c) = dc(x, y, c) + rgb_out(x, y, c);

    output(x, y)   = FloatToRGBA(color(x,y,0), color(x,y,1), color(x,y,2));

    // - Schedule --------------------------------------------------------------

    int parallel_task_size = 32;
    int vector_width       = 16;

    K.compute_root()
     .vectorize(x, vector_width)
     .parallel(y, parallel_task_size);

    maxi   .compute_at(yuv_out, y);
    mini   .compute_at(yuv_out, y);
    dc_x   .compute_at(output, y);
    yuv_out.compute_root();

    yuv_out 
        .vectorize(x, vector_width)
        .parallel(y, parallel_task_size)
        .parallel(c);
    dc_x    
        .vectorize(x, vector_width)
        .parallel(y, parallel_task_size)
        .parallel(c);
    output  
        .vectorize(x, vector_width)
        .parallel(y, parallel_task_size);

    // -------------------------------------------------------------------------

    output.compile_to_file("hl_reconstruct", { i_pyramid_features, i_hp, i_hp_coeff, i_lowpass });

    return 0;
}
