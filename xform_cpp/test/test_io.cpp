#include <boost/filesystem.hpp> 
#include "gtest/gtest.h"

#include "test/setup.h"
#include "perf_measure.h"
#include "utils/image_io.h"
#include "utils/recipe_io.h"

namespace fs = boost::filesystem;
using namespace std;

class IOTest : public testing::Test {
protected:
    IOTest() {
    }
};

TEST_F(IOTest, JpegLoad){
    Image<uint32_t> unprocessed = jpeg_load((TestParams::fixture_path/"io_test.jpg").c_str());
    uint32_t pixel = unprocessed(0,0);
    uint8_t r = pixel & 0xff;
    uint8_t g = (pixel >> 8) & 0xff;
    uint8_t b = (pixel >> 16)& 0xff;

    ASSERT_EQ(10,unprocessed.width());
    ASSERT_EQ(10,unprocessed.height());
    ASSERT_EQ(40,r);
    ASSERT_EQ(70,g);
    ASSERT_EQ(120,b);
}


TEST_F(IOTest, JpegSave){
    Image<uint32_t> unprocessed = jpeg_load((TestParams::fixture_path/"io_test.jpg").c_str());
    jpeg_save(unprocessed,100,(TestParams::output_path/"io_test.jpg").c_str());
    Image<uint32_t> saved= jpeg_load((TestParams::output_path/"io_test.jpg").c_str());

    uint32_t pixel = saved(0,0);
    uint8_t r = pixel & 0xff;
    uint8_t g = (pixel >> 8) & 0xff;
    uint8_t b = (pixel >> 16)& 0xff;

    ASSERT_EQ(10,saved.width());
    ASSERT_EQ(10,saved.height());
    ASSERT_EQ(40,r);
    ASSERT_EQ(70,g);
    ASSERT_EQ(120,b);
}

TEST_F(IOTest, JpegCompressToMemory){
    Image<uint32_t> unprocessed = jpeg_load((TestParams::fixture_path/"io_test.jpg").c_str());
    unsigned char* data = nullptr;
    unsigned long size;
    jpeg_compress(unprocessed,100,&data, &size);
    Image<uint32_t> out = jpeg_decompress(data, size);
    delete data;
    data = nullptr;

    uint32_t pixel = out(0,0);
    uint8_t r = pixel & 0xff;
    uint8_t g = (pixel >> 8) & 0xff;
    uint8_t b = (pixel >> 16)& 0xff;

    ASSERT_EQ(10,out.width());
    ASSERT_EQ(10,out.height());
    ASSERT_EQ(40,r);
    ASSERT_EQ(70,g);
    ASSERT_EQ(120,b);
}

// TEST_F(IOTest, PNGCompressToMemory){
//     Image<uint32_t> unprocessed = jpeg_load((TestParams::fixture_path/"io_test.jpg").c_str());
//     unsigned char* data = nullptr;
//     unsigned long size;
//     png_compress(unprocessed,&data, &size);
//     delete data;
//     data = nullptr;
//
//
//     Image<uint32_t> out = png_decompress(data, size);
//
//     uint32_t pixel = out(0,0);
//     uint8_t r = pixel & 0xff;
//     uint8_t g = (pixel >> 8) & 0xff;
//     uint8_t b = (pixel >> 16)& 0xff;
//
//     ASSERT_EQ(10,out.width());
//     ASSERT_EQ(10,out.height());
//     ASSERT_EQ(40,r);
//     ASSERT_EQ(70,g);
//     ASSERT_EQ(120,b);
// }
