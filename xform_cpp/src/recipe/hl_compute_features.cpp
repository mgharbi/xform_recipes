/* -----------------------------------------------------------------
 * File:    hl_compute_features.cpp
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
#include "utils/resample_with_schedule.h"

#include "global_parameters.h"

using namespace Halide;
using namespace std;

int main(void) {
    Var x("x"), y("y"), c("c");

    // Input parameters
    ImageParam unprocessed = ImageParam(type_of<uint32_t>(), 2, "input");
    const int wSize         = XFORM_WSIZE;
    const float scaleFactor = static_cast<float>(wSize);
    const int nPyrLevels    = XFORM_PYR_LEVELS;

    Func clamped                     ( "clamped");
    Func unprocessed_yuv             ( "unprocessed yuv");
    Func downsampled                 ( "downsampled");
    Func downsampled_x               ( "downsampled_x");
    Func upsampled                   ( "upsampled");
    Func upsampled_x                 ( "upsampled_x");
    Func hp                          ( "hp");
    Func hp_yuv                      ( "hp_yuv");
    vector<Func> gdPyramid_x         ( nPyrLevels);
    vector<Func> gdPyramid           ( nPyrLevels);
    vector<vector<Func> > gaussian_x ( nPyrLevels);
    vector<vector<Func> > gaussian   ( nPyrLevels);
    Func laplacian_features          ( "laplacian features");
    Func output                      ( "output");

    // Convert to YUV float
    clamped  = RGBAToFloat(BoundaryConditions::repeat_edge(unprocessed));
    unprocessed_yuv(x, y, c) = rgb2yuv(clamped)(x, y, c);

    // Downsample
    downsampled_x(x, y, c) = resize_x(clamped, 1.0f/scaleFactor)(x, y, c);
    downsampled(x, y, c)   = resize_y(downsampled_x, 1.0f/scaleFactor)(x, y, c);

    // Upsample
    upsampled_x(x, y, c) = resize_x(downsampled, scaleFactor)(x, y, c);
    upsampled(x, y, c)   = resize_y(upsampled_x, scaleFactor)(x, y, c);

    // High pass features
    hp(x, y, c)     = clamped(x, y, c) - upsampled(x, y, c);
    hp_yuv(x, y, c) = rgb2yuv(hp)(x, y, c);

    gdPyramid[0](x, y) = unprocessed_yuv(x, y, 0);
    for (int j = 1; j < nPyrLevels; j++) {
        downsample_sched(gdPyramid[j-1], gdPyramid_x[j], gdPyramid[j],x,y);
    }
    // gaussian[0](x, y) = gdPyramid[0](x, y);
    gaussian[0].push_back(gdPyramid[0]);
    for(int i = 1; i < nPyrLevels; i++){
        // gaussian[i](x, y) = upsample_n(gdPyramid[i], i+1)(x, y);
        upsample_n_sched(gdPyramid[i],i,gaussian_x[i],gaussian[i],x,y);
    }
    laplacian_features = Func("laplacian_features");
    laplacian_features(x,y,c) = 0.0f;
    for (int i = 0; i < nPyrLevels-1; ++i) {
        laplacian_features(x,y,i) = (gaussian[i][0])(x, y) - (gaussian[i+1][0])(x, y);
    }

    // output feature map
    output(x, y, c) = select(
        c < 3 , hp_yuv(x, y, c),
        (c >= 4) && (c < 4+nPyrLevels-1), laplacian_features(x,y,c-4),
        1.0f
    );

    // - SCHEDULE --------------------------------------------------------------
    
    int tile_width   = 32;
    int vector_width = 16;

    downsampled_x
        .compute_at(downsampled, y)
        .vectorize(x, vector_width)
        .parallel(y, tile_width);

    downsampled
        .compute_root()
        .vectorize(x, vector_width)
        .parallel(y, tile_width)
        .parallel(c);

    upsampled_x
        .store_at(output, y)
        .compute_at(output, y)
        .vectorize(x, vector_width);

    for (int i = 1; i<nPyrLevels; i++) {
        gdPyramid_x[i]
            .compute_at(gdPyramid[i],y)
            .vectorize(x, vector_width)
            .parallel(y, tile_width);
        gdPyramid[i]
            .compute_root()
            .vectorize(x, vector_width)
            .parallel(y, tile_width);

        for (size_t j = 0; j < gaussian[i].size(); ++j) {
            gaussian_x[i][j]
                .compute_at(gaussian[i][j],y)
                .vectorize(x, vector_width)
                .parallel(y, tile_width);
            gaussian[i][j]
                .compute_root()
                .vectorize(x, vector_width)
                .parallel(y, tile_width);
        }
    }

    output
        .parallel(y,tile_width)
        .vectorize(x,vector_width)
        .parallel(c);

    // -------------------------------------------------------------------------
    
    output.compile_to_file("hl_compute_features", { unprocessed });

    return 0;
};
