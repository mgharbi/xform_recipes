#include <Halide.h>
#include "utils/color_transform.h"

using namespace Halide;
using namespace std;

int main()
{
    Var x, y, yi, yo, c;

    ImageParam in;
    Func clamped;
    Func ifloat;
    Func out;

    // Input parameters
    in   = ImageParam(type_of<uint32_t>(), 2);

    // Boundary conditions
    clamped  = BoundaryConditions::repeat_edge(in);

    std::vector<Expr> rgba = RGBAToFloat(clamped(x,y));
    ifloat(x,y,c) = select(c==0, rgba[0], c==1, rgba[1], c==2, rgba[2], rgba[3]);

    out(x, y) = select((ifloat(x, y, 0) > 0.02f) || (ifloat(x, y, 1) > 0.02f) || (ifloat(x, y, 2) > 0.02f), 1, 0);



    out.split(y, yo, yi, 32).parallel(yo).vectorize(x, 8);
    out.compile_to_file("hl_nonzero", {in});


    return 0;
}
