/* -----------------------------------------------------------------
 * File:    hl_precompute_pyramid.cpp
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

using std::vector;
using std::pair;
using std::make_pair;

int main(void) {

    Var x("x"), y("y"), c("c");

    // Input parameters
    ImageParam i_input   (type_of<uint32_t>(), 2);
    int nPyrLevels    = XFORM_PYR_LEVELS;

    // Bounds of input image
    Func clamped  = RGBAToFloat(BoundaryConditions::repeat_edge(i_input));

    vector<Func> gdPyramid_x  (nPyrLevels);
    vector<Func> gdPyramid    (nPyrLevels);
    vector<vector<Func> > gaussian_x   (nPyrLevels);
    vector<vector<Func> > gaussian     (nPyrLevels);
    Func laplacian_features ("laplacian features");

    // Laplacian features
    gdPyramid[0](x, y) = rgb2yuv(clamped)(x, y, 0);
    for (int j=1; j<nPyrLevels; j++) {
        downsample_sched(gdPyramid[j-1], gdPyramid_x[j], gdPyramid[j],x,y);
    }
    gaussian[0].push_back(gdPyramid[0]);
    for(int i=1; i<nPyrLevels; i++){
        upsample_n_sched(gdPyramid[i],i,gaussian_x[i],gaussian[i],x,y);
    }
    laplacian_features(x,y,c) = 0.0f;
    for(int i=0; i<nPyrLevels-1; i++){
        laplacian_features(x,y,i) = (gaussian[i][0])(x, y) - (gaussian[i+1][0])(x, y);
    }

    // - Schedule --------------------------------------------------------------

    int tile_width   = 32;
    int vector_width = 16;

    for (int i=1; i<nPyrLevels; i++) {
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

    laplacian_features
        .parallel(y,tile_width)
        .vectorize(x,vector_width);

    // -------------------------------------------------------------------------

    laplacian_features.compile_to_file("hl_precompute_pyramid", { i_input});

    return 0;
}

