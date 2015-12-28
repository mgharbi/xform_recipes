#include <Halide.h>
#include "utils/color_transform.h"

using namespace Halide;

int main()
{
    Var x("x"), y("y"), xi("xi"), yi("yi"), xo("xo"), yo("yo"), t("t"), c("c");
    RDom r, ri, ri_x,ri_y, ri_xy, ro;

    ImageParam input = ImageParam(type_of<uint32_t>(), 2);
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
    clamped(xi, yi, xo, yo,c) = RGBAToFloat(input)(clamp(cast<int>(xo*tile_x + xi*skip),0,input.width()-1),
                                    clamp(cast<int>(yo*tile_y + yi*skip),0, input.height()-1),c);

    // compute the limit inside each tile xo, yo
    limits_tiled(xo, yo,c) = { clamped(0,0,xo,yo,c), clamped(0,0,xo,yo,c) };
    limits_tiled(xo, yo,c) = {
        Halide::min(clamped(skip*ri.x, skip*ri.y, xo, yo,c), limits_tiled(xo, yo,c)[0]),
        Halide::max(clamped(skip*ri.x, skip*ri.y, xo, yo,c), limits_tiled(xo, yo,c)[1])
    };
    // Valid boundary conditions
    limits_tiled(num_tiles_x-1, yo,c) = {
        Halide::min(clamped(skip*ri_x.x, skip*ri_x.y, num_tiles_x-1, yo,c), limits_tiled(num_tiles_x-1, yo,c)[0]),
        Halide::max(clamped(skip*ri_x.x, skip*ri_x.y, num_tiles_x-1, yo,c), limits_tiled(num_tiles_x-1, yo,c)[1])
    };
    limits_tiled(xo, num_tiles_y-1,c) = {
        Halide::min(clamped(skip*ri_y.x, skip*ri_y.y, xo, num_tiles_y-1,c), limits_tiled(xo, num_tiles_y-1,c)[0]),
        Halide::max(clamped(skip*ri_y.x, skip*ri_y.y, xo, num_tiles_y-1,c), limits_tiled(xo, num_tiles_y-1,c)[1])
    };
    limits_tiled(num_tiles_x-1, num_tiles_y-1,c) = {
        Halide::min(clamped(skip*ri_xy.x, skip*ri_xy.y, num_tiles_x-1, num_tiles_y-1,c), limits_tiled(num_tiles_x-1, num_tiles_y-1,c)[0]),
        Halide::max(clamped(skip*ri_xy.x, skip*ri_xy.y, num_tiles_x-1, num_tiles_y-1,c), limits_tiled(num_tiles_x-1, num_tiles_y-1,c)[1])
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
    Expr bin     = cast<int>((clamped(skip*ri.x, skip*ri.y, xo, yo, c)-min_val) / binsize);
    Expr bin_x   = cast<int>((clamped(skip*ri_x.x, skip*ri_x.y, num_tiles_x-1, yo, c)-min_val) / binsize);
    Expr bin_y   = cast<int>((clamped(skip*ri_y.x, skip*ri_y.y, xo, num_tiles_y-1, c)-min_val) / binsize);
    Expr bin_xy  = cast<int>((clamped(skip*ri_xy.x, skip*ri_xy.y, num_tiles_x-1, num_tiles_y-1, c)-min_val) / binsize);

    // compute histogram inside each tile
    histogram_tiled(xo, yo, c, t) = 0.0f;
    histogram_tiled(xo, yo, c, clamp(bin,0,nbins-1)) += 1.0f;

    // Override boundaries
    histogram_tiled(num_tiles_x-1, yo, c, t) = 0.0f;
    histogram_tiled(num_tiles_x-1, yo, c, clamp(bin_x,0,nbins-1)) += 1.0f;
    histogram_tiled(xo, num_tiles_y-1, c, t) = 0.0f;
    histogram_tiled(xo, num_tiles_y-1, c, clamp(bin_y,0,nbins-1)) += 1.0f;
    histogram_tiled(num_tiles_x-1, num_tiles_y-1, c, t) = 0.0f;
    histogram_tiled(num_tiles_x-1, num_tiles_y-1, c, clamp(bin_xy,0,nbins-1)) += 1.0f;

    // sum the historgams across all tiles
    histogram(t,c) = 0.0f;
    histogram(t,c)+= histogram_tiled(ro.x, ro.y, c, max(t-2,0));
    histogram(0,c) = min_val;
    histogram(1,c) = max_val;
    
    // - SCHEDULE --------------------------------------------------------------

    limits_tiled   .compute_root();
    histogram_tiled.compute_root(); 
    limits         .compute_root();
    histogram      .compute_root();

    limits_tiled
        .reorder(xo, yo, c)
        .fuse(xo, yo, yo)
        .fuse(yo, c, c)
        .parallel(c);
    limits_tiled
        .update(0)
        .reorder(ri.x, ri.y, xo, yo, c)
        .fuse(xo, yo, yo)
        .fuse(yo, c, c)
        .parallel(c); 
    limits      
        .parallel(c);
    limits      
        .update(0)
        .reorder(ro.x, ro.y, c)
        .parallel(c);

    histogram_tiled
        .reorder(t, xo, yo, c)
        .fuse(xo, yo, yo)
        .fuse(yo, c, c)
        .parallel(c);
    histogram_tiled
        .update(0)
        .reorder(ri.x, ri.y, xo, yo, c)
        .fuse(xo, yo, yo)
        .fuse(yo, c, c)
        .parallel(c); 
    histogram      
        .parallel(c);
    histogram      
        .update(0)
        .reorder(ro.x, ro.y, t, c)
        .parallel(c);

    histogram.compile_to_file("hl_histogram_color", {input, nbins, skip});

    return 0;
}
