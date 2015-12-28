#include <Halide.h>
#include "utils/color_transform.h"

using namespace Halide;
using namespace std;

int main()
{
    Var x, y, yi, yo, c;

    ImageParam yuv;
    Func input;
    Func clamped;
    Func rgb;

    // Input parameters
    yuv = ImageParam(Float(32), 3);
    input(x,y,c) = yuv(x,y,c);

    // convert final result back to packed RGB
    clamped(x, y, c) = clamp(yuv2rgb(input)(x, y, c),0.0f,1.0f);
    rgb(x,y) = FloatToRGBA(clamped(x,y,0), clamped(x,y,1), clamped(x,y,2));

    rgb.split(y, yo, yi, 32).parallel(yo).vectorize(x, 8);
    rgb.compile_to_file("hl_yuv2rgb", {yuv});

    return 0;
}
