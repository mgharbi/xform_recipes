#include <Halide.h>
#include "utils/color_transform.h"

using namespace Halide;
using namespace std;

int main() {
    Var x, y, yi, yo, c;

    // Input parameters
    ImageParam input(type_of<float>(), 3);
    ImageParam f(type_of<float>(), 2);

    Func float_idx;

    Func output;

    Func f_clamped = BoundaryConditions::repeat_edge(f);

    Expr source_mini = f_clamped(0,c);
    Expr source_maxi = f_clamped(1,c);
    Expr target_mini = f_clamped(2,c);
    Expr target_maxi = f_clamped(3,c);
    Expr irange      = 1.0f/(source_maxi - source_mini);
    Expr nbins       = f.width()-1-4;
    float_idx(x,y,c) = nbins*(input(x,y,c)-source_mini)*irange;
    Expr idx         = cast<int>(float_idx(x,y,c));
    Expr dx          = float_idx(x,y,c)-idx;

    Expr new_range = target_maxi - target_mini;
    output(x,y,c) = new_range*( (1-dx)*f_clamped(idx,c) + dx*f_clamped(idx+1,c) ) + target_mini;

    // - SCHEDULE --------------------------------------------------------------

    int parallel_task_size = 32;
    int vector_width       = 8;
    output
        .vectorize(x, vector_width)
        .parallel(y, parallel_task_size)
        .parallel(c);
    output.compile_to_file("hl_transfer_histogram_color_float", {input, f});

    // -------------------------------------------------------------------------

    return 0;
}
