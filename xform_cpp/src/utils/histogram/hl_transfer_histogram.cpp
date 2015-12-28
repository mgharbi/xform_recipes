#include <Halide.h>
#include "utils/color_transform.h"

using namespace Halide;
using namespace std;

int main()
{
    Var x, y, yi, yo, c;

    // Input parameters
    ImageParam input(type_of<float>(), 2);;
    ImageParam f(type_of<float>(), 1);
    Param<float> source_mini;
    Param<float> source_maxi;
    Param<float> target_mini;
    Param<float> target_maxi;

    Func f_clamped = BoundaryConditions::repeat_edge(f);
    Func output;

    Expr irange    = 1.0f/(source_maxi - source_mini);
    Expr nbins     = f.width()-1;
    Expr float_idx = nbins*(input(x,y)-source_mini)*irange;
    Expr idx       = cast<int>(float_idx);
    Expr dx        = float_idx-idx;

    Expr new_range = target_maxi - target_mini;
    output(x,y) = new_range*( (1-dx)*f_clamped(idx) + dx*f_clamped(idx+1) ) + target_mini;

    // - SCHEDULE --------------------------------------------------------------
    
    int parallel_task_size = 32;
    int vector_width       = 8;
    output
        .compute_root()
        .vectorize(x, vector_width)
        .parallel(y, parallel_task_size);
    output.compile_to_file("hl_transfer_histogram", {input, f, source_mini, source_maxi, target_mini, target_maxi});

    // -------------------------------------------------------------------------

    return 0;
}
