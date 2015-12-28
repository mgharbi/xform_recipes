#include <Halide.h>
#include "utils/color_transform.h"

using namespace Halide;
using namespace std;

int main()
{
    Var x, y, yi, yo, c;

    ImageParam gray;
    Func clamped;
    Func colors;


    // Input parameters
    gray = ImageParam(type_of<float>(), 2, "gray");

    // Unpack RGBA
    colors(x,y) = FloatToRGBA(gray(x,y), gray(x,y), gray(x,y));


    colors.parallel(y,32).vectorize(x, 8);
    colors.compile_to_file("hl_gray2rgb", {gray});



    return 0;
}
