#include <Halide.h>
using namespace Halide;

Func yuv2rgb(Func yuv_){
  Var x("x"), y("y"), c("c");
  Func rgb("rgb");

  // JPEG YCbCr
  Expr r = yuv_(x, y, 0)                              + 1.13983f * (yuv_(x, y, 2));
  Expr g = yuv_(x, y, 0) - 0.39465f * (yuv_(x, y, 1)) - 0.58060f * (yuv_(x, y, 2));
  Expr b = yuv_(x, y, 0) + 2.03211f * (yuv_(x, y, 1));

  rgb(x,y,c) = select(c == 0, r, c == 1, g,  b);
  return rgb;
}

Func rgb2yuv(Func rgb_){
  Var x("x"), y("y"), c("c");
  Func yuv_("yuv_");

  // JPEG YCbCr
  Expr yy = + 0.299f   * rgb_(x, y, 0) + 0.587f   * rgb_(x, y, 1) + 0.114f   * rgb_(x, y, 2);
  Expr u  = - 0.14713f * rgb_(x, y, 0) - 0.28886f * rgb_(x, y, 1) + 0.436f   * rgb_(x, y, 2);
  Expr v  = + 0.615f   * rgb_(x, y, 0) - 0.51499f * rgb_(x, y, 1) - 0.10001f * rgb_(x, y, 2);

  yuv_(x,y,c) = select(c == 0, yy, c == 1, u,  v);

  return yuv_;
}

// Convert packed RGB to float
static Func RGBAToFloat(Func f) {
    Var x, y, c;
    Func img;

    Expr a = cast<float>((f(x,y) >> 24) & 0xff) / 255.0f;
    Expr b = cast<float>((f(x,y) >> 16) & 0xff) / 255.0f;
    Expr g = cast<float>((f(x,y) >> 8 ) & 0xff) / 255.0f;
    Expr r = cast<float>((f(x,y) >> 0 ) & 0xff) / 255.0f;

    img(x,y,c) = select(c==0, r, c==1, g, c==2, b, a);

    return img;
}

// Convert packed RGB to float
static Func RGBAToFloat(ImageParam f) {
    Var x, y, c;
    Func img;

    Expr a = cast<float>((f(x,y) >> 24) & 0xff) / 255.0f;
    Expr b = cast<float>((f(x,y) >> 16) & 0xff) / 255.0f;
    Expr g = cast<float>((f(x,y) >> 8 ) & 0xff) / 255.0f;
    Expr r = cast<float>((f(x,y) >> 0 ) & 0xff) / 255.0f;

    img(x,y,c) = select(c==0, r, c==1, g, c==2, b, a);

    return img;
}

// Convert packed RGB to float
static std::vector<Expr> RGBAToFloat(Expr f) {
    Expr a = cast<float>((f >> 24) & 0xff) / 255.0f;
    Expr b = cast<float>((f >> 16) & 0xff) / 255.0f;
    Expr g = cast<float>((f >> 8 ) & 0xff) / 255.0f;
    Expr r = cast<float>((f >> 0 ) & 0xff) / 255.0f;
    return {r, g, b, a};
}

// Convert float to packed RGB
static Expr FloatToRGBA(Expr fr, Expr fg, Expr fb, Expr fa=1.0f) {
    Expr a = cast<uint32_t>(clamp(fa*255, 0.0f, 255.0f)) << 24;
    Expr b = cast<uint32_t>(clamp(fb*255, 0.0f, 255.0f)) << 16;
    Expr g = cast<uint32_t>(clamp(fg*255, 0.0f, 255.0f)) << 8;
    Expr r = cast<uint32_t>(clamp(fr*255, 0.0f, 255.0f)) << 0;
    return (r | g | b | a);
}
