#include <Halide.h>
#include "utils/color_transform.h"

using namespace Halide;

int main() {
    Var x("x"), y("y"), xi("xi"), yi("yi"), xo("xo"), yo("yo"), t("t");
    RDom r, ri, ri_x,ri_y, ri_xy, ro;

    ImageParam input = ImageParam(type_of<float>(), 2, "input");
    Param<int> nbins;
    Param<int> skip;
    int tile_sz = 32;

    Func histogram("histogram"), histogram_tiled("histogram_tiled");
    Func limits("limits"),    limits_tiled("limits_tiled");

    Expr tile_x = cast<int>(Halide::min(tile_sz,input.width()));
    Expr tile_y = cast<int>(Halide::min(tile_sz,input.height()));
    Expr num_tiles_x = cast<int>(Halide::ceil(cast<float>(input.width())/tile_x));
    Expr num_tiles_y = cast<int>(Halide::ceil(cast<float>(input.height())/tile_y));

    Expr last_x = (num_tiles_x*tile_x-input.width());
    Expr last_y = (num_tiles_y*tile_y-input.height());

    ri    = RDom(0, tile_x/skip, 0, tile_y/skip);
    ri_x  = RDom(0, select(last_x > 0, last_x/skip, tile_x/skip), 0, tile_y/skip);
    ri_y  = RDom(0, tile_x/skip, 0, select(last_y > 0, last_y/skip, tile_y/skip));
    ri_xy = RDom(0, select(last_x > 0, last_x/skip, tile_x/skip), 0, select(last_y > 0, last_y, tile_y/skip));
    ro    = RDom(0, num_tiles_x, 0, num_tiles_y);

    Func clamped;
    clamped(xi, yi, xo, yo) = input(clamp(cast<int>(xo*tile_x + xi*skip),0,input.width()-1),
                                    clamp(cast<int>(yo*tile_y + yi*skip),0, input.height()-1));

    // compute the limit inside each tile xo, yo
    limits_tiled(xo, yo) = { clamped(0,0,xo,yo), clamped(0,0,xo,yo) };
    limits_tiled(xo, yo) = {
        Halide::min(clamped(skip*ri.x, skip*ri.y, xo, yo), limits_tiled(xo, yo)[0]),
        Halide::max(clamped(skip*ri.x, skip*ri.y, xo, yo), limits_tiled(xo, yo)[1])
    };
    // Valid boundary conditions
    limits_tiled(num_tiles_x-1, yo) = {
        Halide::min(clamped(skip*ri_x.x, skip*ri_x.y, num_tiles_x-1, yo), limits_tiled(num_tiles_x-1, yo)[0]),
        Halide::max(clamped(skip*ri_x.x, skip*ri_x.y, num_tiles_x-1, yo), limits_tiled(num_tiles_x-1, yo)[1])
    };
    limits_tiled(xo, num_tiles_y-1) = {
        Halide::min(clamped(skip*ri_y.x, skip*ri_y.y, xo, num_tiles_y-1), limits_tiled(xo, num_tiles_y-1)[0]),
        Halide::max(clamped(skip*ri_y.x, skip*ri_y.y, xo, num_tiles_y-1), limits_tiled(xo, num_tiles_y-1)[1])
    };
    limits_tiled(num_tiles_x-1, num_tiles_y-1) = {
        Halide::min(clamped(skip*ri_xy.x, skip*ri_xy.y, num_tiles_x-1, num_tiles_y-1), limits_tiled(num_tiles_x-1, num_tiles_y-1)[0]),
        Halide::max(clamped(skip*ri_xy.x, skip*ri_xy.y, num_tiles_x-1, num_tiles_y-1), limits_tiled(num_tiles_x-1, num_tiles_y-1)[1])
    };

    // compute the min and max across tiles
    limits() = { limits_tiled(0,0)[0], limits_tiled(0,0)[1] };
    limits() = {
        Halide::min(limits()[0], limits_tiled(ro.x, ro.y)[0]),
        Halide::max(limits()[1], limits_tiled(ro.x, ro.y)[1])
    };

    Expr min_val = limits()[0];
    Expr max_val = limits()[1];
    Expr binsize = (max_val-min_val)/(nbins);
    Expr bin     = cast<int>((clamped(skip*ri.x, skip*ri.y, xo, yo)-min_val) / binsize);
    Expr bin_x     = cast<int>((clamped(skip*ri_x.x, skip*ri_x.y, num_tiles_x-1, yo)-min_val) / binsize);
    Expr bin_y     = cast<int>((clamped(skip*ri_y.x, skip*ri_y.y, xo, num_tiles_y-1)-min_val) / binsize);
    Expr bin_xy     = cast<int>((clamped(skip*ri_xy.x, skip*ri_xy.y, num_tiles_x-1, num_tiles_y-1)-min_val) / binsize);

    // compute histogram inside each tile
    histogram_tiled(xo, yo, t) = 0.0f;
    histogram_tiled(xo, yo, clamp(bin,0,nbins-1)) += 1.0f;

    // Override boundaries
    histogram_tiled(num_tiles_x-1, yo, t) = 0.0f;
    histogram_tiled(num_tiles_x-1, yo, clamp(bin_x,0,nbins-1)) += 1.0f;
    histogram_tiled(xo, num_tiles_y-1, t) = 0.0f;
    histogram_tiled(xo, num_tiles_y-1, clamp(bin_y,0,nbins-1)) += 1.0f;
    histogram_tiled(num_tiles_x-1, num_tiles_y-1, t) = 0.0f;
    histogram_tiled(num_tiles_x-1, num_tiles_y-1, clamp(bin_xy,0,nbins-1)) += 1.0f;

    // sum the histograms across all tiles
    histogram(t) = 0.0f;
    histogram(t)+= histogram_tiled(ro.x, ro.y, max(t-2,0));
    histogram(0) = min_val;
    histogram(1) = max_val;

    // - SCHEDULE --------------------------------------------------------------
    
    limits_tiled   .compute_root();
    histogram_tiled.compute_root();
    limits         .compute_root();
    histogram      .compute_root();

    limits_tiled
        .reorder(xo, yo)
        .fuse(xo, yo, yo)
        .parallel(yo);
    limits_tiled
        .update(0)
        .reorder(ri.x, ri.y, xo, yo)
        .fuse(xo, yo, yo)
        .parallel(yo);
    limits
        .update(0)
        .reorder(ro.x, ro.y);
    histogram_tiled
        .reorder(t, xo, yo)
        .fuse(xo, yo, yo)
        .parallel(yo);
    histogram_tiled
        .update(0)
        .reorder(ri.x, ri.y, xo, yo)
        .fuse(xo, yo, yo)
        .parallel(yo);
    histogram
        .update(0)
        .reorder(t, ro.x, ro.y);

    // -------------------------------------------------------------------------

    histogram.compile_to_file("hl_histogram", { input, nbins, skip });

    return 0;
}
