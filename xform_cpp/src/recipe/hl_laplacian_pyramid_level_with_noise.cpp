/* -----------------------------------------------------------------
 * File:    hl_laplacian_pyramid_level_with_noise.cpp
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
#include "utils/resample.h"
#include "utils/resample_with_schedule.h"

using namespace Halide;

int main()
{
    Var x("x"), y("y"), c("c");
    Var yi("yi"), yo("yo");
    Var xi("yi"), xo("yo");

    ImageParam input(type_of<uint32_t>(), 2);
    ImageParam ds2(type_of<uint32_t>(), 2);
    ImageParam noise(type_of<float>(), 2, "noise");
    Func clamped  = RGBAToFloat(BoundaryConditions::repeat_edge(input));
    Func clamped_ds2  = RGBAToFloat(BoundaryConditions::repeat_edge(ds2));
    Func clamped_noise  = BoundaryConditions::repeat_edge(noise);

    Func laplacian;
    Func upsampled_x;
    Func upsampled;
    upsample_sched(clamped_ds2, upsampled_x, upsampled,x,y);
    laplacian = Func("laplacian");
    laplacian(x,y,c) = clamped(x,y,c) - upsampled(x,y,c) + clamped_noise(x,y);

    // -------------------------------------------------------------------------
    
    int tile_width   = 32;
    int vector_width = 16;
    // upsampled_x.compute_at(laplacian,y).parallel(y,tile_width).vectorize(x,vector_width);
    upsampled_x.compute_at(laplacian,y).split(x,xo,xi,64).parallel(xo).vectorize(xi, 8);
    laplacian.parallel(y,tile_width).vectorize(x,vector_width);

    // -------------------------------------------------------------------------
    
    laplacian.compile_to_file("hl_laplacian_pyramid_level_with_noise", {input, ds2,noise});
}
