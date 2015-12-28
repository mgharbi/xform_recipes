/* -----------------------------------------------------------------
 * File:    hl_collapse_pyramid.cpp
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

    ImageParam lp;
    ImageParam lvl0;
    ImageParam lvl1;
    ImageParam lvl2;
    Func clamped_lp;
    Func float_lp;
    Func clamped_lvl0;
    Func clamped_lvl1;
    Func clamped_lvl2;

    Func downsampled;
    Func downsampled_x;
    Func output;
    Func final;

    Func gp[3];

    // Input parameters
    lp = ImageParam (type_of<uint32_t>(), 2, "lp");
    lvl0 = ImageParam (type_of<float>(), 3, "lvl0");
    lvl1 = ImageParam (type_of<float>(), 3, "lvl1");
    lvl2 = ImageParam (type_of<float>(), 3, "lvl2");

    // Boundary conditions
    clamped_lp = Func("clamped_lp");
    clamped_lp = BoundaryConditions::repeat_edge(lp);
    clamped_lvl0 = Func("clamped_lvl0");
    clamped_lvl0 = BoundaryConditions::repeat_edge(lvl0);
    clamped_lvl1 = Func("clamped_lvl1");
    clamped_lvl1 = BoundaryConditions::repeat_edge(lvl1);
    clamped_lvl2 = Func("clamped_lvl2");
    clamped_lvl2 = BoundaryConditions::repeat_edge(lvl2);

    float_lp = Func("float_lp");
    std::vector<Expr> rgba = RGBAToFloat(clamped_lp(x,y));
    float_lp(x,y,c) = select(c==0, rgba[0], c==1, rgba[1], c==2, rgba[2], rgba[3]);

    gp[0](x,y,c) = upsample(float_lp)(x,y,c) + clamped_lvl0(x,y,c);
    gp[1](x,y,c) = upsample(gp[0])(x,y,c) + clamped_lvl1(x,y,c);
    gp[2](x,y,c) = clamp(upsample(gp[1])(x,y,c) + clamped_lvl2(x,y,c),0.0f,1.0f);

    output = Func("output");
    // output = final;
    output(x,y) = FloatToRGBA(gp[2](x,y,0), gp[2](x,y,1), gp[2](x,y,2));

    // output.compute_root().parallel(y,16).vectorize(x,8);
    for (int i = 0; i < 2; ++i) {
        gp[i].compute_root().parallel(y,16).vectorize(x,8);
    }
    output.compute_root().parallel(y,16).vectorize(x,8);
    output.compile_to_file("hl_collapse_pyramid", { lp, lvl0,lvl1,lvl2 });

    return 0;
}
