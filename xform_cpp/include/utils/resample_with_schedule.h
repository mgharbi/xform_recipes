#ifndef RESAMPLE_WITH_SCHEDULE_H_K0GSWYXS
#define RESAMPLE_WITH_SCHEDULE_H_K0GSWYXS

#include <Halide.h>
#include <vector>

using namespace Halide;
using std::vector;

// Downsample with a 1 3 3 1 filter
void downsample_sched(Func f, Func &downx, Func &downy, Var &x, Var &y) {

    downx(x, y, _) = (f(2*x-1, y, _) + 3.0f * (f(2*x, y, _) + f(2*x+1, y, _)) + f(2*x+2, y, _)) / 8.0f;
    downy(x, y, _) = (downx(x, 2*y-1, _) + 3.0f * (downx(x, 2*y, _) + downx(x, 2*y+1, _)) + downx(x, 2*y+2, _)) / 8.0f;
}

// Upsample using bilinear interpolation
void upsample_sched(Func f, Func &upx, Func &upy, Var &x, Var &y) {

    upx(x, y, _) = 0.25f * f((x/2) - 1 + 2*(x % 2), y, _) + 0.75f * f(x/2, y, _);
    upy(x, y, _) = 0.25f * upx(x, (y/2) - 1 + 2*(y % 2), _) + 0.75f * upx(x, y/2, _);

}

void upsample_n_sched(Func f, const int J, vector<Func> &upx, vector<Func>& upy, Var &x, Var &y){

    upx = vector<Func>(J);
    upy = vector<Func>(J);

    upsample_sched(f, upx[J-1],upy[J-1],x,y);
    for (int j = J-2; j >= 0; j--) {
        upsample_sched(upy[j+1], upx[j],upy[j],x,y);
    }
}

#endif /* end of include guard: RESAMPLE_WITH_SCHEDULE_H_K0GSWYXS */
