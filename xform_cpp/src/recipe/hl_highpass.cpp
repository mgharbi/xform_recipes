/* -----------------------------------------------------------------
 * File:    hl_highpass.cpp
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
#include "global_parameters.h"

using namespace Halide;

int main(void) {

    Var x("x"), y("y"), c("c");

    // Input parameters
    ImageParam input   (type_of<uint32_t>(), 2, "input");
    ImageParam lp_input(type_of<uint32_t>(), 2, "lp_input");
    int wSize         = XFORM_WSIZE;
    float scaleFactor = wSize;

    Func input_float = RGBAToFloat(BoundaryConditions::repeat_edge(input));
    Func lp_float    = RGBAToFloat(BoundaryConditions::repeat_edge(lp_input));

    Func upsampled  ("upsampled");
    Func upsampled_x("upsampled_x");
    Func hp         ("hp");
    Func output     ("highpass");

    upsampled_x(x, y, c) = resize_x(lp_float,    scaleFactor)(x, y, c);
    upsampled  (x, y, c) = resize_y(upsampled_x, scaleFactor)(x, y, c);
    hp         (x, y, c) = input_float(x, y, c) - upsampled(x, y, c);
    output     (x, y, c) = rgb2yuv(hp)(x, y, c);

    // - SCHEDULE --------------------------------------------------------------

    int tile_width   = 32;
    int vector_width = 16;

    upsampled_x
        .compute_root()
        .vectorize(x, vector_width)
        .parallel(c);
    output
        .parallel(y, tile_width)
        .vectorize(x, vector_width)
        .parallel(c);

    // -------------------------------------------------------------------------

    output.compile_to_file("hl_highpass", { input, lp_input });

    return 0;
}
