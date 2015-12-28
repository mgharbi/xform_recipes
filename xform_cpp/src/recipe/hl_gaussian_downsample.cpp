/* -----------------------------------------------------------------
 * File:    hl_gaussian_downsample.cpp
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
#include "utils/resample_with_schedule.h"
#include "global_parameters.h"

using namespace Halide;


int main()
{
    Var x("x"), y("y"), c("c");
    Var yi("yi"), yo("yo");

    ImageParam input;
    Func input_float;
    Func downsampled;
    Func downsampled_x;
    Func output;
    Func final;


    // Input parameters
    input = ImageParam (type_of<uint32_t>(), 2, "input");

    // Boundary conditions

    input_float = RGBAToFloat(BoundaryConditions::repeat_edge(input));

    downsampled = Func("downsampled");
    downsampled_x = Func("downsampled_x");

    downsample_sched(input_float, downsampled, downsampled_x,x,y);

    output = Func("output");
    output(x,y) = FloatToRGBA(downsampled(x,y,0), downsampled(x,y,1), downsampled(x,y,2));


    // -------------------------------------------------------------------------

    int tile_width   = 32;
    int vector_width = 32;
    downsampled_x.compute_at(downsampled,y);
    downsampled_x.specialize(output.output_buffer().width() < 32 || output.output_buffer().height() < 32).vectorize(x, 4).parallel(y, 4);
    downsampled_x.vectorize(x, vector_width).parallel(y, tile_width);

    output.specialize(output.output_buffer().width() < 32 || output.output_buffer().height() < 32).vectorize(x,4).parallel(y,4);
    output.vectorize(x, vector_width).parallel(y, tile_width);

    // -------------------------------------------------------------------------
    output.compile_to_file("hl_gaussian_downsample", { input });


    return 0;
};
