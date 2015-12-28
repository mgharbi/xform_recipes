/* -----------------------------------------------------------------
 * File:    hl_upsample.cpp
 * Author:  Michael Gharbi <gharbi@mit.edu>
 * Created: 2015-07-24
 * -----------------------------------------------------------------
 * 
 * 
 * 
 * ---------------------------------------------------------------*/

#include <Halide.h>
#include "utils/color_transform.h"
#include "utils/resize.h"
#include "utils/resample.h"
#include "global_parameters.h"

using namespace Halide;

int main() {
    Var x("x"), y("y"), c("c");
    Var yi("yi"), yo("yo");
    Var xi("xi"), xo("xo");

    ImageParam input;
    Param<float> scaleFactor;
    Func input_float;
    Func upsampled;
    Func upsampled_x;
    Func output;
    Func final;

    // Input parameters
    input = ImageParam (type_of<uint32_t>(), 2, "input");

    // Boundary conditions
    input_float = RGBAToFloat(BoundaryConditions::repeat_edge(input));

    upsampled_x = Func("downsampled_x");
    upsampled_x(x, y, c) = resize_x(input_float, scaleFactor)(x, y, c);

    upsampled = Func("downsampled");
    upsampled(x, y, c) = resize_y(upsampled_x, scaleFactor)(x, y, c);

    final = Func("final");
    final(x, y, c) = clamp(upsampled(x, y, c), 0.0f, 1.0f);

    output = Func("output");
    output(x,y) = FloatToRGBA(final(x,y,0), final(x,y,1), final(x,y,2));


    upsampled_x.compute_at(output,y).split(x,xo,xi,64).parallel(xo).vectorize(xi, 8);
    output.compute_root().parallel(y,32).vectorize(x,8);
    output.compile_to_file("hl_upsample", { input, scaleFactor });

    return 0;
}
