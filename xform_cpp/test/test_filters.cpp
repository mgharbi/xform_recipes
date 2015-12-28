#include <boost/filesystem.hpp> 
#include "gtest/gtest.h"

#include "test/setup.h"
#include "perf_measure.h"
#include "utils/image_io.h"
#include "utils/recipe_io.h"

#include "filters/local_laplacian/hl_local_laplacian.h"
#include "filters/style_transfer/style_transfer.h"
#include "filters/colorization/colorize.h"

namespace fs = boost::filesystem;
using namespace std;

class FilterTest : public testing::Test {
protected:
    FilterTest() {
    }
};

TEST_F(FilterTest, LocalLaplacian){
    Image<uint32_t> unprocessed = jpeg_load((TestParams::fixture_path/"0001.jpg").c_str());
    Image<uint32_t> processed(unprocessed.width(), unprocessed.height(), unprocessed.channels());
    int levels  = 10;
    float alpha = 2.0f;
    float beta  = 1.0f;
    hl_local_laplacian(levels, alpha/(levels-1), beta, unprocessed, processed);
    jpeg_save(processed,100,(TestParams::output_path/"filter_local_laplacian.jpg").c_str());
}

TEST_F(FilterTest, StyleTransfer){
    Image<uint32_t> unprocessed = jpeg_load((TestParams::fixture_path/"0001.jpg").c_str());
    Image<uint32_t> target = jpeg_load((TestParams::fixture_path/"style_target.jpg").c_str());
    Image<uint32_t> processed(unprocessed.width(), unprocessed.height(), unprocessed.channels());
    int levels = 10;
    int iterations = 3;
    style_transfer(unprocessed,target,levels,processed,iterations);
    jpeg_save(processed,100,(TestParams::output_path/"filter_style_transfer.jpg").c_str());
}

TEST_F(FilterTest, Colorization){
    Image<uint32_t> unprocessed = jpeg_load((TestParams::fixture_path/"colorization.jpg").c_str());
    Image<uint32_t> scribbles = jpeg_load((TestParams::fixture_path/"scribbles.jpg").c_str());
    Image<uint32_t> processed(unprocessed.width(), unprocessed.height(), unprocessed.channels());
    colorize(unprocessed,scribbles,processed);
    jpeg_save(processed,100,(TestParams::output_path/"filter_colorization.jpg").c_str());
}
