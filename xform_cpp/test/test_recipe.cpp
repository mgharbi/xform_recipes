#include <boost/filesystem.hpp> 
#include "gtest/gtest.h"

#include "test/setup.h"
#include "perf_measure.h"
#include "utils/image_io.h"
#include "utils/recipe_io.h"

#include "recipe/Recipe.h"
#include "recipe/client_preprocessing.h"
#include "recipe/server_preprocessing.h"
#include "filters/local_laplacian/hl_local_laplacian.h"

namespace fs = boost::filesystem;
using namespace std;

class RecipeTest : public testing::Test {
protected:
    RecipeTest() {
        unprocessed_path = TestParams::fixture_path / "0001.jpg";
        processed_path   = TestParams::fixture_path / "0001-processed.jpg";
    }

    fs::path unprocessed_path;
    fs::path processed_path;
    fs::path output_path;
};


TEST_F(RecipeTest, fitHighQuality){
    Image<uint32_t> unprocessed = jpeg_load(unprocessed_path.c_str());

    int levels  = 10;
    float alpha = 2.0f;
    float beta  = 1.0f;
    Image<uint32_t> processed(unprocessed.width(),
        unprocessed.height(), unprocessed.channels());
    hl_local_laplacian(levels, alpha/(levels-1), beta, unprocessed, processed);
    
    // Fit recipe
    xform::Recipe recipe(unprocessed, processed);

    // hp, lp, qtable
    std::shared_ptr<Image<uint32_t> > lp_res = recipe.lowpass_residual();
    std::shared_ptr<Image<float> > hp_coefs  = recipe.highpass_coefficients();
    std::vector<float> qTable = recipe.qtable();

    // Reconstruct recipe
    Image<uint32_t> reconstructed;
    recipe.reconstruct_image(unprocessed, reconstructed);

    jpeg_save(reconstructed,100,(TestParams::output_path/"recipe_fitHighQuality.jpg").c_str());
}


TEST_F(RecipeTest, fitDegraded){
    // Load high quality and degrade inputs
    Image<uint32_t> unprocessed = jpeg_load(unprocessed_path.c_str());
    Image<uint32_t> unprocessed_lowres = 
        jpeg_load((TestParams::fixture_path / "0001-degraded4x.jpg").c_str());
    
    // Process ground-truth
    int levels  = 10;
    float alpha = 2.0f;
    float beta  = 1.0f;
    Image<uint32_t> processed(unprocessed.width(),
        unprocessed.height(), unprocessed.channels());
    hl_local_laplacian(levels, alpha/(levels-1), beta, unprocessed, processed);

    // Compute statistics of high-quality input
    uint8_t *hdata = nullptr;
    unsigned long hdatasize = 0;
    client_preprocessing(unprocessed, &hdata, &hdatasize);

    // Generate some noise data
    Image<float> noise(100,100);
    char * noisedata = reinterpret_cast<char*>(noise.data());
    memset(noisedata,0,sizeof(float)*100*100);
    unsigned long noisedatasize = 100*100;

    int upsampling_factor = 4;
    int width             = unprocessed.width();
    int height            = unprocessed.height();

    // Assemble proxy
    Image<uint32_t> proxy_unprocessed = server_preprocessing(
        unprocessed_lowres,
        hdata, hdatasize,
        noisedata,noisedatasize,
        upsampling_factor,
        width, height
    );

    // Process proxy
    Image<uint32_t> proxy_processed(proxy_unprocessed.width(),
        proxy_unprocessed.height(), proxy_unprocessed.channels());
    hl_local_laplacian(levels, 
        alpha/(levels-1), beta, proxy_unprocessed, proxy_processed);

    // Fit recipe on proxy pair
    xform::Recipe recipe(proxy_unprocessed, proxy_processed);

    // hp, lp, qtable
    std::shared_ptr<Image<uint32_t> > lp_res = recipe.lowpass_residual();
    std::shared_ptr<Image<float> > hp_coefs  = recipe.highpass_coefficients();
    std::vector<float> qTable = recipe.qtable();

    // Reconstruct recipe
    Image<uint32_t> reconstructed;
    recipe.reconstruct_image(unprocessed, reconstructed);

    // Debug output
    jpeg_save(reconstructed, 100, (TestParams::output_path     / "recipe_fitDegraded_reconstructed.jpg").c_str());
    jpeg_save(proxy_unprocessed, 100, (TestParams::output_path / "recipe_fitDegraded_proxy_input.jpg").c_str());
    jpeg_save(proxy_processed, 100, (TestParams::output_path   / "recipe_fitDegraded_proxy_output.jpg").c_str());
    jpeg_save(processed, 100, (TestParams::output_path         / "recipe_fitDegraded_ground_truth.jpg").c_str());
}
