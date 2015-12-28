#include <Halide.h>
#include "utils/color_transform.h"
#include "utils/resample.h"
#include "filters/style_transfer/const.h"
using namespace Halide;

Var x("x"), y("y"), c("c");

int main() {

    // Number of pyramid levels
    const int J = 8;

    // Input image
    ImageParam input(type_of<float>(), 2);
    ImageParam lut  (type_of<float>(),    1);

    // number of intensity levels
    Param<int> levels;

    // Parameters controlling the filter
    Param<float> source_mini;
    Param<float> source_maxi;
    Param<float> target_mini;
    Param<float> target_maxi;
    Expr range = source_maxi-source_mini;
    Expr irange = 1.0f/(range);
    Expr nbins = lut.width()-1;

    Expr width  = input.width();
    Expr height = input.height();

    // loop variables
    Var k;
    
    // -------------------------------------------------------------------------

    // Set a boundary condition
    Func clamped("clamped");
    clamped(x, y) = input(clamp(x, 0, input.width()-1), clamp(y, 0, input.height()-1));

    Func gPyramid[J];
    Func lPyramid[J];
    Func inGPyramid[J];
    Func outLPyramid[J];
    Func outGPyramid[J];

    // 2 - Do a lookup into a lut with nbins entries per intensity level
    Expr level = k * (1.0f / (levels - 1));

    // Processed input
    float eps                = 1e-3;
    Expr sign                = (level-clamped(x,y))/(abs(level-clamped(x,y))+eps);
    Expr gradient_mag        = abs(level-clamped(x,y));
    Expr float_idx           = nbins*(gradient_mag-source_mini)*irange;
    Expr idx                 = clamp(cast<int>(floor(float_idx)),0,nbins-1);
    Expr dx                  = (float_idx-idx) / (1.0f*nbins);
    Expr new_range = target_maxi-target_mini;
    // gPyramid[0](x, y, k)  = level-sign*(((1-dx)*lut(idx) + dx*lut(idx+1) ) );
    gPyramid[0](x, y, k)  = level-sign*( target_mini + new_range*((1-dx)*lut(idx) + dx*lut(idx+1) ) );
    for (int j               = 1; j < J; j++) {
        gPyramid[j](x, y, k) = downsample(gPyramid[j-1])(x, y, k);
    }

    // Get its laplacian pyramid
    lPyramid[J-1](x, y, k) = gPyramid[J-1](x, y, k);
    for (int j = J-2; j >= 0; j--) {
        lPyramid[j](x, y, k) = gPyramid[j](x, y, k) - upsample(gPyramid[j+1])(x, y, k);
    }

    // 1 - Make the Gaussian pyramid of the input
    inGPyramid[0](x, y) = clamped(x, y);
    for (int j = 1; j < J; j++) {
        inGPyramid[j](x, y) = downsample(inGPyramid[j-1])(x, y);
    }

    // 4 - Make the laplacian pyramid of the output
    for (int j = 0; j < J; j++) {
        // Split input pyramid value into integer and floating parts
        Expr level = inGPyramid[j](x, y) * cast<float>(levels-1); // range level
        Expr li = clamp(cast<int>(level), 0, levels-2);
        Expr lf = level - cast<float>(li);

        // Linearly interpolate between the nearest processed pyramid levels
        outLPyramid[j](x, y) = (1.0f - lf) * lPyramid[j](x, y, li) + lf * lPyramid[j](x, y, li+1);
    }

    // 5 - Make the Gaussian pyramid of the output
    outGPyramid[J-1](x, y) = outLPyramid[J-1](x, y);
    for (int j = J-2; j >= 0; j--) {
        outGPyramid[j](x, y) = upsample(outGPyramid[j+1])(x, y) + outLPyramid[j](x, y);
    }

    Func output;
    output(x, y) = outGPyramid[0](x, y);

    // -------------------------------------------------------------------------

    Target target = get_target_from_environment();

    // cpu schedule
    Var yi;
    output.parallel(y, 4).vectorize(x, 8);
    for (int j = 0; j < 4; j++) {
        if (j > 0) inGPyramid[j].compute_root().parallel(y, 4).vectorize(x, 8);
        if (j > 0) gPyramid[j].compute_root().parallel(y, 4).vectorize(x, 8);
        outGPyramid[j].compute_root().parallel(y, 4).vectorize(x, 8);
    }
    for (int j = 4; j < J; j++) {
        inGPyramid[j].compute_root().parallel(y);
        gPyramid[j].compute_root().parallel(k);
        outGPyramid[j].compute_root().parallel(y);
    }

    output.compile_to_file("hl_style_transfer", { levels, input, lut, source_mini, source_maxi, target_mini, target_maxi}, target);

    return 0;
}

