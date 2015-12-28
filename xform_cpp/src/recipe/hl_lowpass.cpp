/* -----------------------------------------------------------------
 * File:    hl_lowpass.cpp
 * Author:  Michael Gharbi <gharbi@mit.edu>
 * Created: 2015-07-24
 * -----------------------------------------------------------------
 * 
 * 
 * 
 * ---------------------------------------------------------------*/

#include <Halide.h>
#include "utils/color_transform.h"
#include "global_parameters.h"
#include "utils/resize.h"

using namespace Halide;

int main(void) {

    Var x("x"), y("y"), c("c");

    // Input parameters
    ImageParam input(type_of<uint32_t>(), 2, "input");
    int wSize         = XFORM_WSIZE;
    float scaleFactor = static_cast<float>(wSize);

    Func input_float = RGBAToFloat(BoundaryConditions::repeat_edge(input));

    Func downsampled_x("downsampled_x");
    Func downsampled  ("downsampled");
    Func output       ("lowpass");

    downsampled_x(x, y, c) = resize_x(input_float,   1.0f/scaleFactor)(x, y, c);
    downsampled  (x, y, c) = resize_y(downsampled_x, 1.0f/scaleFactor)(x, y, c);
    output       (x, y)    = FloatToRGBA(downsampled(x,y,0), downsampled(x,y,1), downsampled(x,y,2));

    // - SCHEDULE --------------------------------------------------------------

    int tile_width   = 32;
    int vector_width = 32;

    downsampled_x
        .compute_root();
    downsampled_x
        .specialize(output.output_buffer().width() < 32 || output.output_buffer().height() < 32)
        .vectorize(x, 4)
        .parallel(y, 4)
        .parallel(c);
    downsampled_x
        .vectorize(x, vector_width)
        .parallel(y, tile_width);

    output
        .specialize(output.output_buffer().width() < 32 || output.output_buffer().height() < 32)
        .vectorize(x,4)
        .parallel(y,4);
    output
        .vectorize(x, vector_width)
        .parallel(y, tile_width);

    // -------------------------------------------------------------------------

    output.compile_to_file("hl_lowpass", { input });

    return 0;
}
