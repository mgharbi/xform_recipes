#include <vector>

#include "gtest/gtest.h"

#include "utils/histogram/histogram.h"
#include "utils/image_io.h"

using namespace std;

class HistogramTest : public testing::Test {
protected:
    HistogramTest() {
        I1_float = Image<float>(w,h);
        I2_float = Image<float>(w,h,c);
        I3_int = Image<uint32_t>(w,h);
        for (int x = 0; x < w; ++x)
        for (int y = 0; y < h; ++y)
        {
            if(x < w/2){
                I1_float(x,y)   = values[0];
                I2_float(x,y,0) = values[0];
                I2_float(x,y,1) = values[1];
                I2_float(x,y,2) = values[2];

                I3_int(x,y) = (values_int[2] << 16) | (values_int[1] << 8) | values_int[0];
            }else {
                I1_float(x,y)   = 2*values[0];
                I2_float(x,y,0) = 2*values[0];
                I2_float(x,y,1) = 2*values[1];
                I2_float(x,y,2) = 2*values[2];

                I3_int(x,y) = ((2*values_int[2]) << 16) | ((2*values_int[1]) << 8) | 2*values_int[0];
            }
        }
    }

    const int nbins = 256;
    const int w = 20;
    const int h = 10;
    const int c = 3;
    const float values[3] = {0.11f, 0.21f, 0.31f};
    const uint8_t values_int[3] = {120,1,42};
    Image<float> I1_float;
    Image<float> I2_float;
    Image<uint32_t> I3_int;
};

TEST_F(HistogramTest, initFromRawPtr){
    vector<float> raw_hist = {1.0f, 3.0f, 4.0f, 1.0f,2.0f};
    Histogram h0 = Histogram(raw_hist.data(), 3);
    EXPECT_EQ(3, h0.nbins);
    EXPECT_EQ(1.0f, h0.mini);
    EXPECT_EQ(3.0f, h0.maxi);
    EXPECT_EQ(7.0f, h0.sum);
    EXPECT_EQ(4.0f, h0.count[0]);
    EXPECT_EQ(1.0f, h0.count[1]);
    EXPECT_EQ(2.0f, h0.count[2]);
    EXPECT_EQ(4.0f/7.0f, h0.cdf[0]);
    EXPECT_EQ(5.0f/7.0f, h0.cdf[1]);
    EXPECT_EQ(7.0f/7.0f, h0.cdf[2]);
}

TEST_F(HistogramTest, initFromImage){
    Image<float> raw_hist(5);
    raw_hist(0) = 1.0f;
    raw_hist(1) = 3.0f;
    raw_hist(2) = 4.0f;
    raw_hist(3) = 1.0f;
    raw_hist(4) = 2.0f;
    Histogram h0 = Histogram(raw_hist.data(), 3);
    EXPECT_EQ(3, h0.nbins);
    EXPECT_EQ(1.0f, h0.mini);
    EXPECT_EQ(3.0f, h0.maxi);
    EXPECT_EQ(7.0f, h0.sum);
    EXPECT_EQ(4.0f, h0.count[0]);
    EXPECT_EQ(1.0f, h0.count[1]);
    EXPECT_EQ(2.0f, h0.count[2]);
    EXPECT_EQ(4.0f/7.0f, h0.cdf[0]);
    EXPECT_EQ(5.0f/7.0f, h0.cdf[1]);
    EXPECT_EQ(7.0f/7.0f, h0.cdf[2]);
}


TEST_F(HistogramTest, countImage1D){
    Image<float> vals(5,1);
    vals(0) = 1.0f;
    vals(1) = 2.1f;
    vals(2) = 1.3f;
    vals(3) = 1.8f;
    vals(4) = 3.0f;
    
    Histogram h0 = histogram(vals, 4,1);
    EXPECT_EQ(4, h0.nbins);
    EXPECT_EQ(1.0f, h0.mini);
    EXPECT_EQ(3.0f, h0.maxi);
    EXPECT_EQ(5.0f, h0.sum);
    EXPECT_EQ(2.0f, h0.count[0]);
    EXPECT_EQ(1.0f, h0.count[1]);
    EXPECT_EQ(1.0f, h0.count[2]);
    EXPECT_EQ(1.0f, h0.count[3]);
    EXPECT_EQ(2.0f/5.0f, h0.cdf[0]);
    EXPECT_EQ(3.0f/5.0f, h0.cdf[1]);
    EXPECT_EQ(4.0f/5.0f, h0.cdf[2]);
    EXPECT_EQ(5.0f/5.0f, h0.cdf[3]);

    vals = Image<float>(6,1);
    vals(0) = 1.1f;
    vals(1) = 2.1f;
    vals(2) = 1.3f;
    vals(3) = 1.8f;
    vals(4) = 3.0f;
    vals(5) = 2.9f;
    
    h0 = histogram(vals, 4,1);
    EXPECT_EQ(4, h0.nbins);
    EXPECT_EQ(1.1f, h0.mini);
    EXPECT_EQ(3.0f, h0.maxi);
    EXPECT_EQ(6.0f, h0.sum);
    EXPECT_EQ(2.0f, h0.count[0]);
    EXPECT_EQ(1.0f, h0.count[1]);
    EXPECT_EQ(1.0f, h0.count[2]);
    EXPECT_EQ(2.0f, h0.count[3]);
    EXPECT_EQ(2.0f/6.0f, h0.cdf[0]);
    EXPECT_EQ(3.0f/6.0f, h0.cdf[1]);
    EXPECT_EQ(4.0f/6.0f, h0.cdf[2]);
    EXPECT_EQ(6.0f/6.0f, h0.cdf[3]);
}

// TEST_F(HistogramTest, skip2){
//     Image<float> vals(5,1);
//     vals(0) = 1.0f;
//     vals(1) = 2.1f;
//     vals(2) = 1.3f;
//     vals(3) = 1.8f;
//     vals(4) = 3.0f;
//     
//     Histogram h0 = histogram(vals, 4,2);
//     EXPECT_EQ(4, h0.nbins);
//     EXPECT_EQ(1.0f, h0.mini);
//     EXPECT_EQ(3.0f, h0.maxi);
//     EXPECT_EQ(3.0f, h0.sum);
    // EXPECT_EQ(2.0f, h0.count[0]);
    // EXPECT_EQ(1.0f, h0.count[1]);
    // EXPECT_EQ(1.0f, h0.count[2]);
    // EXPECT_EQ(1.0f, h0.count[3]);
    // EXPECT_EQ(2.0f/5.0f, h0.cdf[0]);
    // EXPECT_EQ(3.0f/5.0f, h0.cdf[1]);
    // EXPECT_EQ(4.0f/5.0f, h0.cdf[2]);
    // EXPECT_EQ(5.0f/5.0f, h0.cdf[3]);
// }

TEST_F(HistogramTest, countImage2D){
    // image is correctly filled
    EXPECT_EQ(values[0], I1_float(0,0));
    EXPECT_EQ(2*values[0], I1_float(w-1,0));

    Histogram h1 = histogram(I1_float, 2,1);
    EXPECT_EQ(2, h1.nbins);
    EXPECT_EQ(values[0]  , h1.mini); 
    EXPECT_EQ(2*values[0]  , h1.maxi); 
    EXPECT_EQ(h*w, h1.sum);
    EXPECT_EQ(h*w/2, h1.count[0]);
    EXPECT_EQ(h*w/2, h1.count[1]);
    EXPECT_EQ(1.0f/2.0f, h1.cdf[0]);
    EXPECT_EQ(1.0f, h1.cdf[1]);

}

TEST_F(HistogramTest, countImage3D){
    vector<Histogram> h2 = histograms(I2_float, nbins,1);

    // histogram has correct span?
    EXPECT_EQ(nbins, h2[0].nbins);
    EXPECT_EQ(nbins, h2[1].nbins);
    EXPECT_EQ(nbins, h2[2].nbins);
    EXPECT_EQ(values[0]  , h2[0].mini); 
    EXPECT_EQ(values[1]  , h2[1].mini); 
    EXPECT_EQ(values[2]  , h2[2].mini); 
    EXPECT_EQ(2*values[0], h2[0].maxi); 
    EXPECT_EQ(2*values[1], h2[1].maxi); 
    EXPECT_EQ(2*values[2], h2[2].maxi); 
    EXPECT_EQ(h*w, h2[0].sum);
    EXPECT_EQ(h*w, h2[1].sum);
    EXPECT_EQ(h*w, h2[2].sum);
}

TEST_F(HistogramTest, countImage2Dint){
    vector<Histogram> h2 = histograms(I3_int, nbins, 1);

    // histogram has correct span?
    EXPECT_EQ(nbins, h2[0].nbins);
    EXPECT_EQ(nbins, h2[1].nbins);
    EXPECT_EQ(nbins, h2[2].nbins);
    EXPECT_EQ(values_int[0]  , static_cast<uint8_t>(h2[0].mini*255));
    EXPECT_EQ(values_int[1]  , static_cast<uint8_t>(h2[1].mini*255));
    EXPECT_EQ(values_int[2]  , static_cast<uint8_t>(h2[2].mini*255));
    EXPECT_EQ(2*values_int[0], static_cast<uint8_t>(h2[0].maxi*255));
    EXPECT_EQ(2*values_int[1], static_cast<uint8_t>(h2[1].maxi*255));
    EXPECT_EQ(2*values_int[2], static_cast<uint8_t>(h2[2].maxi*255));
    EXPECT_EQ(h*w, h2[0].sum);
    EXPECT_EQ(h*w, h2[1].sum);
    EXPECT_EQ(h*w, h2[2].sum);
}

TEST_F(HistogramTest, transferFunction){
    vector<float> hsource(6);
    hsource[0] = 0.0f;
    hsource[1] = 1.0f;
    hsource[2] = 1.0f;
    hsource[3] = 3.0f;
    hsource[4] = 2.0f;
    hsource[5] = 5.0f;
    Histogram h0(hsource.data(), hsource.size()-2);

    vector<float> htarget(6);
    htarget[0] = 0.0f;
    htarget[1] = 1.0f;
    htarget[2] = 3.0f;
    htarget[3] = 1.0f;
    htarget[4] = 3.0f;
    htarget[5] = 2.0f;
    Histogram h1(htarget.data(), htarget.size()-2);

    TransferFunction tf(h0,h1);

    EXPECT_EQ(0.0f, h0.mini);
    EXPECT_EQ(1.0f, h0.maxi);
    EXPECT_EQ(11.0f, h0.sum);
    EXPECT_EQ(1.0f, h0.count[0]);
    EXPECT_EQ(3.0f, h0.count[1]);
    EXPECT_EQ(2.0f, h0.count[2]);
    EXPECT_EQ(5.0f, h0.count[3]);
    EXPECT_EQ(1.0f/11.0f, h0.cdf[0]);
    EXPECT_EQ(4.0f/11.0f, h0.cdf[1]);
    EXPECT_EQ(6.0f/11.0f, h0.cdf[2]);
    EXPECT_EQ(11.0f/11.0f, h0.cdf[3]);

    EXPECT_EQ(0.0f, h1.mini);
    EXPECT_EQ(1.0f, h1.maxi);
    EXPECT_EQ(9.0f, h1.sum);
    EXPECT_EQ(3.0f, h1.count[0]);
    EXPECT_EQ(1.0f, h1.count[1]);
    EXPECT_EQ(3.0f, h1.count[2]);
    EXPECT_EQ(2.0f, h1.count[3]);
    EXPECT_EQ(3.0f/9.0f, h1.cdf[0]);
    EXPECT_EQ(4.0f/9.0f, h1.cdf[1]);
    EXPECT_EQ(7.0f/9.0f, h1.cdf[2]);
    EXPECT_EQ(9.0f/9.0f, h1.cdf[3]);

    EXPECT_EQ(0.0f, tf.source_mini);
    EXPECT_EQ(1.0f, tf.source_maxi);
    EXPECT_EQ(0.0f, tf.target_mini);
    EXPECT_EQ(1.0f, tf.target_maxi);
    EXPECT_EQ(5, tf.size);
    EXPECT_EQ(0.0f, tf.values[0]);
    EXPECT_EQ(1.0f/4.0f, tf.values[1]);
    EXPECT_EQ(2.0f/4.0f, tf.values[2]);
    EXPECT_EQ(3.0f/4.0f, tf.values[3]);
    EXPECT_EQ(1.0f, tf.values[4]);
}

TEST_F(HistogramTest, compression){
}
