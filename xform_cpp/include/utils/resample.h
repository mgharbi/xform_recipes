#ifndef RESAMPLE_H_ZL2GQKLO
#define RESAMPLE_H_ZL2GQKLO

#include <Halide.h>

using std::vector;
using namespace Halide;

// Downsample with a 1 3 3 1 filter
Func downsample(Func f) {
    Func downx, downy;
    Var x("x"), y("y");

    downx(x, y, _) = (f(2*x-1, y, _) + 3.0f * (f(2*x, y, _) + f(2*x+1, y, _)) + f(2*x+2, y, _)) / 8.0f;
    downy(x, y, _) = (downx(x, 2*y-1, _) + 3.0f * (downx(x, 2*y, _) + downx(x, 2*y+1, _)) + downx(x, 2*y+2, _)) / 8.0f;

    return downy;
}
// Upsample using bilinear interpolation
Func upsample(Func f) {
    Func upx, upy;
    Var x("x"), y("y");

    upx(x, y, _) = 0.25f * f((x/2) - 1 + 2*(x % 2), y, _) + 0.75f * f(x/2, y, _);
    upy(x, y, _) = 0.25f * upx(x, (y/2) - 1 + 2*(y % 2), _) + 0.75f * upx(x, y/2, _);

    return upy;
}

Func downsample_n(Func f, const int J){
    vector<Func> gdPyramid(J);
    Var x("x"), y("y"), yi("yi"), yo("yo");

    gdPyramid[0](x, y, _) = f(x, y, _);
    for (int j = 1; j < J; j++) {
        gdPyramid[j](x, y, _) = downsample(gdPyramid[j-1])(x, y, _);
    }
    for(int i = 1; i < J; i++){
        gdPyramid[i].compute_root();
        gdPyramid[i].parallel(y, 4).vectorize(x, 8);
    }
    return gdPyramid[J-1];
}
Func upsample_n(Func f, const int J){
    if(J == 1) {
        return f;
    }

    vector<Func> guPyramid(J);
    Var x("x"), y("y"), yi("yi"), yo("yo");

    guPyramid[J-1](x, y, _) = f(x, y, _);
    for (int j = J-1; j > 0; j--) {
        guPyramid[j-1](x, y, _) = upsample(guPyramid[j])(x, y, _);
    }
    for(int i = 0; i < J-1; i++){
        guPyramid[i].compute_root();
        guPyramid[i].parallel(y,32).vectorize(x, 8);
    }
    return guPyramid[0];
}

Func gaussian_blur(Func f, const int j){
    Var x("x"), y("y");
    Func ds;
    ds(x, y, _) = downsample_n(f, j)(x, y, _);
    Func us;
    us(x, y, _) = upsample_n(ds, j)(x, y, _);
    return us;
}

Func gaussian_stack(Func f, const int j){
    Var x("x"), y("y");
    Func ds;
    ds(x, y, _) = downsample_n(f, j)(x, y, _);
    Func us;
    us(x, y, _) = upsample_n(ds, j)(x, y, _);
    return us;
}


#endif /* end of include guard: RESAMPLE_H_ZL2GQKLO */

