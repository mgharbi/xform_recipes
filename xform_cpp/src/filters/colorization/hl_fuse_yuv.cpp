#include <Halide.h>
#include "utils/color_transform.h"

using namespace Halide;
using namespace std;

int main()
{
    Var x, y, yi, yo, c;

    ImageParam gray;
    ImageParam colors;
    Func clamped;
    Func clamped_colors;
    Func yuv;
    Func yuv_colors;
    Func final;
    Func gray_float;
    Func colors_float;

    // Input parameters
    gray   = ImageParam(type_of<uint32_t>(), 2);
    colors = ImageParam(type_of<uint32_t>(), 2);

    clamped        = BoundaryConditions::mirror_interior(gray);
    clamped_colors = BoundaryConditions::mirror_interior(colors);

    // Unpack RGBA
    std::vector<Expr> rgba = RGBAToFloat(clamped(x,y));
    gray_float(x,y,c) = select(c==0, rgba[0], c==1, rgba[1], c==2, rgba[2], rgba[3]);
    std::vector<Expr> rgba2 = RGBAToFloat(clamped_colors(x,y));
    colors_float(x,y,c) = select(c==0, rgba2[0], c==1, rgba2[1], c==2, rgba2[2], rgba2[3]);

    yuv(x, y, c)        = rgb2yuv(gray_float)(x, y, c);
    yuv_colors(x, y, c) = rgb2yuv(colors_float)(x, y, c);
    final(x,y,c) = select( c == 0 , yuv(x,y,0), yuv_colors(x,y,c));


    final.split(y, yo, yi, 32).parallel(c).vectorize(x, 8);
    final.compile_to_file("hl_fuse_yuv", {gray, colors});


    return 0;
}
