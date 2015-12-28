#include <Halide.h>
#include "halide/color_transform.h"
#include "halide/resample.h"

using namespace Halide;

#define EPSILON 0.01f

Var x("x"), y("y"), c("c");


int main(void) {

    // Number of pyramid levels
    const int J = 8;

    // number of intensity levels
    Param<int> levels("levels");

    // Parameters controlling the filter
    Param<float> alpha("alpha"), beta("beta");

    // Input image
    ImageParam input(type_of<uint32_t>(), 2, "input");

    // loop variables
    Var k;

    // -------------------------------------------------------------------------
    

    // Make the remapping function as a lookup table.
    Func remap;
    Expr fx = cast<float>(x) / 256.0f;
    remap(x) = alpha*fx*exp(-fx*fx/2.0f);

    // Convert to floating point
    Func input_float("input_float");
    std::vector<Expr> rgba = RGBAToFloat(input(x,y));
    input_float(x,y,c) = select(c==0, rgba[0], c==1, rgba[1], c==2, rgba[2], rgba[3]);

    // Set a boundary condition
    Func clamped("clamped");
    clamped(x, y, c) = input_float(clamp(x, 0, input.width()-1), clamp(y, 0, input.height()-1), c);

    // Get the luminance channel
    Func gray("gray");
    gray(x, y) = 0.299f * clamped(x, y, 0) + 0.587f * clamped(x, y, 1) + 0.114f * clamped(x, y, 2);

    Func gPyramid[J];
    Func lPyramid[J];
    Func inGPyramid[J];
    Func outLPyramid[J];
    Func outGPyramid[J];
    for (int j=0; j<J; j++) {
        gPyramid[j]    = Func("gPyramid_"   + std::to_string(j));
        lPyramid[j]    = Func("lPyramid_"   + std::to_string(j));
        inGPyramid[j]  = Func("inGPyramid_" + std::to_string(j));
        outGPyramid[j] = Func("outGPyramid_"+ std::to_string(j));
        outLPyramid[j] = Func("outLPyramid" + std::to_string(j));
    }

    // Make the processed Gaussian pyramid.
    // Do a lookup into a lut with 256 entires per intensity level
    Expr level = k * (1.0f / (levels - 1));
    Expr idx = gray(x, y)*cast<float>(levels-1)*256.0f;
    idx = clamp(cast<int>(idx), 0, (levels-1)*256);
    gPyramid[0](x, y, k) = beta*(gray(x, y) - level) + level + remap(idx - 256*k);
    for (int j = 1; j < J; j++) {
        gPyramid[j](x, y, k) = downsample(gPyramid[j-1])(x, y, k);
    }

    // Get its laplacian pyramid
    lPyramid[J-1](x, y, k) = gPyramid[J-1](x, y, k);
    for (int j = J-2; j >= 0; j--) {
        lPyramid[j](x, y, k) = gPyramid[j](x, y, k) - upsample(gPyramid[j+1])(x, y, k);
    }

    // Make the Gaussian pyramid of the input
    inGPyramid[0](x, y) = gray(x, y);
    for (int j = 1; j < J; j++) {
        inGPyramid[j](x, y) = downsample(inGPyramid[j-1])(x, y);
    }

    // Make the laplacian pyramid of the output
    for (int j = 0; j < J; j++) {
        // Split input pyramid value into integer and floating parts
        Expr level = inGPyramid[j](x, y) * cast<float>(levels-1);
        Expr li = clamp(cast<int>(level), 0, levels-2);
        Expr lf = level - cast<float>(li);
        // Linearly interpolate between the nearest processed pyramid levels
        outLPyramid[j](x, y) = (1.0f - lf) * lPyramid[j](x, y, li) + lf * lPyramid[j](x, y, li+1);
    }

    // Make the Gaussian pyramid of the output
    outGPyramid[J-1](x, y) = outLPyramid[J-1](x, y);
    for (int j = J-2; j >= 0; j--) {
        outGPyramid[j](x, y) = upsample(outGPyramid[j+1])(x, y) + outLPyramid[j](x, y);
    }

    Func color("color");
    color(x, y, k) = outGPyramid[0](x, y) * (clamped(x, y, k)+EPSILON) / (gray(x, y)+EPSILON);

    // convert final result back to packed RGB
    Func output("local_laplacian_server");
    output(x,y) = FloatToRGBA(color(x,y,0), color(x,y,1), color(x,y,2));

    // -------------------------------------------------------------------------

    Target target = get_target_from_environment();

    // int parallel_task_size = 20;
    // int vector_width       = 4;

    remap.compute_root();

    // cpu schedule
    Var yi;
    output.parallel(y, 4).vectorize(x, 8);
    gray.compute_root().parallel(y, 4).vectorize(x, 8);
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

    // remap .compute_root().vectorize(x, vector_width);
    // output.compute_root().parallel(y, parallel_task_size).vectorize(x, vector_width);
    // gray  .compute_root().parallel(y, parallel_task_size).vectorize(x, vector_width);
    //
    // for (int j=0; j<J; j++) {
    //     gPyramid[j]   .compute_root().parallel(y, parallel_task_size).vectorize(x, vector_width).parallel(k);
    //     inGPyramid[j] .compute_root().parallel(y, parallel_task_size).vectorize(x, vector_width);
    //     outGPyramid[j].compute_root().parallel(y, parallel_task_size).vectorize(x, vector_width);
    // }

    // -------------------------------------------------------------------------

    output.compile_to_file("halide_local_laplacian_server", { levels, alpha, beta, input }, target);

    return 0;
}

