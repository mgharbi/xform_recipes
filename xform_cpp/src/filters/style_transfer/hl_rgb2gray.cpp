#include <Halide.h>
#include "utils/color_transform.h"

using namespace Halide;
using namespace std;

int main(){
    Var x, y, yi, yo, c;

    ImageParam colors;
    Func colors_float;
    Func gray;

    // Input parameters
    colors = ImageParam(type_of<uint32_t>(), 2, "colors");

    // Unpack RGBA
    std::vector<Expr> rgba = RGBAToFloat(colors(x,y));
    colors_float(x,y,c) = select(c==0, rgba[0], c==1, rgba[1], c==2, rgba[2], rgba[3]);

    gray(x, y) = rgb2yuv(colors_float)(x, y, 0);


    gray.parallel(y,32).vectorize(x, 8);
    gray.compile_to_file("hl_rgb2gray", {colors});


    return 0;
}
