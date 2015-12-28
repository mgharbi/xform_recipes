#include <Halide.h>
#include "utils/color_transform.h"
#include "utils/resize.h"
#include "utils/resample.h"
#include "utils/resample_with_schedule.h"

using namespace Halide;

int main() {
    ImageParam input;
    ImageParam ds2;

    Func histogram, histogram_tiled;
    Func limits,    limits_tiled;
    Func upsampled;
    Func upsampled_x;

    Param<int> nbins;

    RDom r;
    RDom ri;
    RDom ro;

    Var x, y, xi, yi, xo, yo, c, t;

    int num_tiles_x = 1;
    int num_tiles_y = 16;
    int skip        = 4;    // downsampling in each dimension for faster histogram

    input = ImageParam(type_of<uint32_t>(), 2);
    ds2   = ImageParam(type_of<uint32_t>(), 2);

    Expr tile_x = input.width() / num_tiles_x;
    Expr tile_y = input.height()/ num_tiles_y;

    x = Var("x");
    y = Var("y");
    xi= Var("xi");
    yi= Var("yi");
    xo= Var("xo");
    yo= Var("yo");
    c = Var("c");
    t = Var("t");

    histogram       = Func("histogram");
    histogram_tiled = Func("histogram_tiled");
    limits          = Func("limits");
    limits_tiled    = Func("limits_tiled");
    // upsampled       = Func("upsampled");
    // upsampled_x     = Func("upsampled_x");

    ri = RDom(0, tile_x/skip, 0, tile_y/skip);
    ro = RDom(0, num_tiles_x, 0, num_tiles_y);

    // upsample_sched(RGBAToFloat(BoundaryConditions::repeat_edge(ds2)),upsampled_x, upsampled,x,y);

    // create a laplacian that is skip times subsampled version of input
    // ds2 is half the size of input, so use skip/2 as subsampling factor
    Func laplacian;
    {
        Func down_input, down_ds2;
        down_input(xi, yi, xo, yo, c) = RGBAToFloat(input)((xo*tile_x + xi*skip),   (yo*tile_y + yi*skip),   c);
        down_ds2  (xi, yi, xo, yo, c) = RGBAToFloat(ds2)  ((xo*tile_x + xi*skip)/2, (yo*tile_y + yi*skip)/2, c);
        laplacian (xi, yi, xo, yo, c) = down_input(xi, yi, xo, yo, c) - down_ds2(xi, yi, xo, yo, c);
    }

    // compute the limit inside each tile xo, yo
    limits_tiled(xo, yo, c) = { laplacian(0,0,xo,yo,c), laplacian(0,0,xo,yo,c) };
    limits_tiled(xo, yo, c) = {
        Halide::min(laplacian(ri.x, ri.y, xo, yo, c), limits_tiled(xo, yo, c)[0]),
        Halide::max(laplacian(ri.x, ri.y, xo, yo, c), limits_tiled(xo, yo, c)[1])
    };

    // compute the min and max across tiles
    limits(c) = { limits_tiled(0,0,c)[0], limits_tiled(0,0,c)[1] };
    limits(c) = {
        Halide::min(limits(c)[0], limits_tiled(ro.x, ro.y, c)[0]),
        Halide::max(limits(c)[1], limits_tiled(ro.x, ro.y, c)[1])
    };

    Expr min_val = limits(c)[0];
    Expr max_val = limits(c)[1];
    Expr binsize = (max_val-min_val)/(nbins-1);
    Expr bin     = cast<int>((laplacian(ri.x, ri.y, xo, yo, c)-min_val) / binsize);

    // compute histogram inside each tile
    histogram_tiled(xo, yo, c, t) = 0.0f;
    histogram_tiled(xo, yo, c, clamp(bin,0,nbins-1)) += 1.0f;

    // sum the historgams across all tiles
    histogram(t,c) = 0.0f;
    histogram(t,c)+= histogram_tiled(ro.x, ro.y, c, max(t-2,0));
    histogram(0,c) = min_val;
    histogram(1,c) = max_val;

    int vector_width = 32;
    int task_size    = 32;

    limits_tiled   .compute_root();
    histogram_tiled.compute_root();
    limits         .compute_root();
    histogram      .compute_root();
    //upsampled      .compute_root();

    //upsampled_x.compute_at(upsampled,y).vectorize(x,vector_width);
    //upsampled.parallel(y,task_size).vectorize(x,vector_width);

    limits_tiled.reorder(xo, yo, c).fuse(xo, yo, yo).fuse(yo, c, c).parallel(c);
    limits_tiled.update(0).reorder(ri.x, ri.y, xo, yo, c).fuse(xo, yo, yo).fuse(yo, c, c).parallel(c);
    limits      .parallel(c);
    limits      .update(0).reorder(ro.x, ro.y, c).parallel(c).unroll(ro.x).unroll(ro.y);

    histogram_tiled.reorder(t, xo, yo, c).fuse(xo, yo, yo).fuse(yo, c, c).parallel(c);
    histogram_tiled.update(0).reorder(ri.x, ri.y, xo, yo, c).fuse(xo, yo, yo).fuse(yo, c, c).parallel(c);
    histogram      .parallel(c);
    histogram      .update(0).reorder(t, ro.x, ro.y, c).parallel(c).unroll(ro.x).unroll(ro.y);

    histogram.compile_to_file("hl_histogram_of_laplacian_coefficients", {input, ds2, nbins});


    return 0;
}
