#ifndef RESIZE_H_PNWQC79K
#define RESIZE_H_PNWQC79K



#include <Halide.h>
using namespace Halide;

Expr kernel_box(Expr x) {
    Expr xx = abs(x);
    return select(xx <= 0.5f, 1.0f, 0.0f);
}


Expr kernel_linear(Expr x) {
    Expr xx = abs(x);
    return select(xx < 1.0f, 1.0f - xx, 0.0f);
}


Expr kernel_cubic(Expr x) {
    Expr xx = abs(x);
    Expr xx2 = xx * xx;
    Expr xx3 = xx2 * xx;
    float a = -0.5f;

    return select(xx < 1.0f, (a + 2.0f) * xx3 - (a + 3.0f) * xx2 + 1,
                  select (xx < 2.0f, a * xx3 - 5 * a * xx2 + 8 * a * xx - 4.0f * a,
                          0.0f));
}


Expr sinc(Expr x) {
    return sin(float(M_PI) * x) / x;
}


Expr kernel_lanczos(Expr x) {
    Expr value = sinc(x) * sinc(x/3);
    value = select(x == 0.0f, 1.0f, value); // Take care of singularity at zero
    value = select(x > 3 || x < -3, 0.0f, value); // Clamp to zero out of bounds
    return value;
}


struct KernelInfo {
    const char *name;
    float size;
    Expr (*kernel)(Expr);
};


enum InterpolationType {
    BOX, LINEAR, CUBIC, LANCZOS
};

static KernelInfo kernelInfo[] = {
    { "box", 0.5f, kernel_box },
    { "linear", 1.0f, kernel_linear },
    { "cubic", 2.0f, kernel_cubic },
    { "lanczos", 3.0f, kernel_lanczos }
};

InterpolationType interpolationType = LINEAR;

Func resize_x(Func f, Expr scaleFactor){
  Var x("x"), y("y"), c("c"), k("k");
  Expr kernelScaling = min(scaleFactor, 1.0f);
  Expr kernelSize = kernelInfo[interpolationType].size / kernelScaling;
  Expr sourcex = (x + 0.5f) / scaleFactor;
  Func kernelx("kernelx");
  Expr beginx = cast<int>(sourcex - kernelSize + 0.5f);
  RDom domx(0, cast<int>(2.0f * kernelSize) + 1, "domx");
  {
        const KernelInfo &info = kernelInfo[interpolationType];
        Func kx;
        kx(x, k) = info.kernel((k + beginx - sourcex) * kernelScaling);
        kernelx(x, k) = kx(x, k) / sum(kx(x, domx));
  }
  Func resized_x("resized_x");

  kernelx.compute_root();

  resized_x(x, y, _) = sum(kernelx(x, domx) * cast<float>(f(domx + beginx, y, _)));
  return resized_x;
}


Func resize_y(Func f, Expr scaleFactor){
    Var x("x"), y("y"), k("k"), c("c");

    Expr kernelScaling = min(scaleFactor, 1.0f);
    Expr kernelSize = kernelInfo[interpolationType].size / kernelScaling;
    Expr sourcey = (y + 0.5f) / scaleFactor;
    Func kernely("kernely");
    Expr beginy = cast<int>(sourcey - kernelSize + 0.5f);
    RDom domy(0, cast<int>(2.0f * kernelSize) + 1, "domy");
    {
        const KernelInfo &info = kernelInfo[interpolationType];
        Func ky;
        ky(y, k) = info.kernel((k + beginy - sourcey) * kernelScaling);
        kernely(y, k) = ky(y, k) / sum(ky(y, domy));
    }
    Func resized_y("resized_y");
    resized_y(x, y, _) = sum(kernely(y, domy) * f(x, domy + beginy, _));
    kernely.compute_root();
    return resized_y;
}

// Func resize(Func f, Expr scaleFactor) {
// }
//
#endif /* end of include guard: RESIZE_H_PNWQC79K */
