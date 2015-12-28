/* -----------------------------------------------------------------
 * File:    hl_laplacian_pyramid_level.cpp
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

using namespace Halide;

int main() {

    Var x("x"), y("y"), c("c");
    Var yi("yi"), yo("yo");

    ImageParam input;
    ImageParam ds2;
    Func clamped;
    Func clamped_ds2;
    Func input_float;
    Func ds2_float;
    Func laplacian;

    // Input parameters
    input       = ImageParam(type_of<uint32_t>(), 2);
    ds2         = ImageParam(type_of<uint32_t>(), 2);
    clamped     = BoundaryConditions::repeat_edge(input);
    clamped_ds2 = BoundaryConditions::repeat_edge(ds2);

    input_float = Func("input_float");
    std::vector<Expr> rgba = RGBAToFloat(clamped(x,y));
    input_float(x,y,c) = select(c==0, rgba[0], c==1, rgba[1], c==2, rgba[2], rgba[3]);

    ds2_float = Func("input_float");
    std::vector<Expr> rgba2 = RGBAToFloat(clamped_ds2(x,y));
    ds2_float(x,y,c) = select(c==0, rgba2[0], c==1, rgba2[1], c==2, rgba2[2], rgba2[3]);

    laplacian = Func("laplacian");
    laplacian(x,y,c) = input_float(x,y,c) - upsample(ds2_float)(x, y,c);

    laplacian.compute_root().parallel(y,16).vectorize(x,8);
    laplacian.compile_to_file("hl_laplacian_pyramid_level", {input, ds2});


    return 0;
}
