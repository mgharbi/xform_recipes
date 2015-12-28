#include <Halide.h>
#include "utils/color_transform.h"

using namespace Halide;
using namespace std;

int main() {
    Var x, y, yi, yo, c;

    // Input parameters
    ImageParam input(type_of<uint32_t>(), 2);
    ImageParam f(type_of<float>(), 2);

    Func f_clamped = BoundaryConditions::repeat_edge(f);

    Func output;
    Func output_float;

    Func input_float = Func("input_float");
    std::vector<Expr> rgba = RGBAToFloat(input(x,y));
    input_float(x,y,c) = select(c==0, rgba[0], c==1, rgba[1], c==2, rgba[2], rgba[3]);

    Expr source_mini = f_clamped(0,c);
    Expr source_maxi = f_clamped(1,c);
    Expr target_mini = f_clamped(2,c);
    Expr target_maxi = f_clamped(3,c);
    Expr irange = 1.0f/(source_maxi - source_mini);
    Expr nbins = f.width()-1-4;
    Expr float_idx = nbins*(input_float(x,y,c)-source_mini)*irange;
    Expr idx = cast<int>(float_idx);
    Expr dx = float_idx-idx;

    Expr new_range = target_maxi - target_mini;
    output_float(x,y,c) = clamp(new_range*( (1-dx)*f_clamped(idx,c) + dx*f_clamped(idx+1,c) ) + target_mini,0.0f,1.0f);
    output(x,y) = FloatToRGBA(output_float(x,y,0), output_float(x,y,1), output_float(x,y,2));

    // - SCHEDULE --------------------------------------------------------------

    int parallel_task_size = 32;
    int vector_width       = 8;
    output
        .vectorize(x, vector_width)
        .parallel(y, parallel_task_size);
    output.compile_to_file("hl_transfer_histogram_color", {input, f});

    // -------------------------------------------------------------------------

    return 0;
}
