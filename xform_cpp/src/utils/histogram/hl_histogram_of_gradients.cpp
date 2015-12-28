#include <Halide.h>
#include "utils/color_transform.h"

using namespace Halide;
using namespace std;

int main() {

    Var x, y, yi, yo, c;

    ImageParam input;
    Param<int> nbins;
    Func clamped;
    Expr dx,dy;
    Func norm;
    Func histogram;
    Func mini;
    Func maxi;

    RDom rx, ry, r;

    // Input parameters
    input = ImageParam(type_of<float>(), 2);
    clamped = BoundaryConditions::repeat_edge(input);
    dx = clamped(x,y) - clamped(x-1,y);
    dy = clamped(x,y) - clamped(x,y-1);
    norm(x,y) = sqrt(dx*dx + dy*dy);

    histogram = Func("histogram");
    histogram(x) = 0.0f;

    rx = RDom(0,input.width());
    ry = RDom(0,input.height());
    r  = RDom(0,input.width(),0,input.height());

    mini() = minimum(norm(r.x, r.y));
    maxi() = maximum(norm(r.x, r.y));

    Expr binsize = (maxi()-mini())/(nbins-1);

    Expr bin = cast<int>( (norm(r.x,r.y)-mini()) / binsize );
    histogram(0) = mini();
    histogram(1) = maxi();
    histogram(2+clamp(bin,0,nbins-1)) += 1;

    mini.compute_root();
    maxi.compute_root();
    histogram.compile_to_file("hl_histogram_of_gradients", {input, nbins});

    return 0;
}
