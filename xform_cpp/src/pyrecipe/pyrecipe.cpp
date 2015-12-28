#include <cstdio>
#include <sstream>
#include <vector>
#include <memory>
#include <cmath>

#include "utils/image_io.h"
#include "utils/recipe_io.h"
#include "perf_measure.h"

#include "print_helper.h"
#include "recipe/Recipe.h"

#include "recipe/client_preprocessing.h"
#include "recipe/server_preprocessing.h"

#include "filters/colorization/colorize.h"
#include "filters/local_laplacian/hl_local_laplacian.h"
#include "filters/style_transfer/style_transfer.h"

#include "pyrecipe.h"

using namespace std;

void naive_processing(
    char *im_data, int im_datasize,
    char *extra_data, int extra_datasize,
    XformParams params,
    char **output, int *output_datasize
) {
    // Load JPEG input
    auto start = get_time();
    Image<uint32_t> input = jpeg_decompress((unsigned char*) im_data,im_datasize);
    Image<uint32_t> out(input.width(), input.height());
    PRINT("- load/allocate : %ldms\n", get_duration(start,get_time()));

    // Process image
    start = get_time();
    if(strcmp(params.filter_type, "local_laplacian") == 0) {
        float beta = 1.0f;
        hl_local_laplacian(params.levels, params.alpha/(params.levels-1), beta, input, out);
    } else if(strcmp(params.filter_type, "style_transfer") == 0) {
        Image<uint32_t> target = jpeg_decompress((unsigned char*) extra_data,extra_datasize);
        style_transfer(input, target, params.levels, out, params.iterations);
    } else if(strcmp(params.filter_type, "colorization") == 0) {
        Image<uint32_t> scribbles = jpeg_decompress((unsigned char*) extra_data,extra_datasize);
        colorize(input, scribbles, out);
    }
    PRINT("- filter : %ldms\n", get_duration(start,get_time()));

    // Save JPEG output
    start = get_time();
    unsigned char* jpeg_data = nullptr;
    unsigned long jpeg_size = 0;
    jpeg_compress(out,params.jpeg_quality,&jpeg_data, &jpeg_size);

    *output = (char *)jpeg_data;
    *output_datasize = jpeg_size;
    PRINT("- save jpeg :\t %ldms\n", get_duration(start,get_time()));
}


void input_preprocessing(
    char *im_data, int im_datasize,
    char *noise_data, int noise_datasize,
    XformParams params,
    char **output, int *output_datasize
)
{
    // Load JPEG input
    auto start = get_time();
    Image<uint32_t> input;

    if(params.upsampling_factor == 1) {
        input = jpeg_decompress((unsigned char*) im_data,im_datasize);
        PRINT("- load input : %ldms\n", get_duration(start,get_time()));
    } else {
        int buffer_position;
        Image<uint32_t> raw_input = jpeg_decompress((unsigned char*) im_data,im_datasize, &buffer_position);
        uint8_t* hdata = (uint8_t*) im_data + buffer_position;

        // FIXME: Dirty Hack because jpeg decoder doesnt always go to the end of image
        while(buffer_position < im_datasize && ( *(hdata-1)!= 0xd9 || *(hdata-2) != (uint8_t)0xff)) {
            PRINT("- current byte %x\n", ((uint8_t*)im_data)[buffer_position]);
            buffer_position++;
            hdata++;
        }

        int hdatasize = im_datasize-buffer_position;

        // Upsample and preprocess input
        start = get_time();
        input = server_preprocessing(raw_input, (uint8_t*)hdata, hdatasize, noise_data, noise_datasize, params.upsampling_factor, params.width, params.height);
        PRINT("- server preprocess input (%d) : %ldms\n", params.upsampling_factor, get_duration(start,get_time()));
    }

    // Save JPEG output
    start = get_time();
    unsigned char* jpeg_data = nullptr;
    unsigned long jpeg_size = 0;
    jpeg_compress(input,params.jpeg_quality,&jpeg_data, &jpeg_size);

    *output = (char *)jpeg_data;
    *output_datasize = jpeg_size;
    PRINT("- save jpeg :\t %ldms\n", get_duration(start,get_time()));
}


void recipe_processing(
    char *im_data, int im_datasize,
    char *extra_data, int extra_datasize,
    char *noise_data, int noise_datasize,
    XformParams params,
    char **output, int *output_datasize
) {
    // Load JPEG input
    auto start = get_time();
    Image<uint32_t> input;

    if(params.upsampling_factor == 1) {
        input = jpeg_decompress((unsigned char*) im_data,im_datasize);
        PRINT("- load input : %ldms\n", get_duration(start,get_time()));
    } else {
        int buffer_position;
        Image<uint32_t> raw_input = jpeg_decompress((unsigned char*) im_data,im_datasize, &buffer_position);
        uint8_t* hdata = (uint8_t*) im_data + buffer_position;

        // FIXME: Dirty Hack because jpeg decoder doesnt always go to the end of image
        while(buffer_position < im_datasize && ( *(hdata-1)!= 0xd9 || *(hdata-2) != (uint8_t)0xff)) {
            PRINT("- current byte %x\n", ((uint8_t*)im_data)[buffer_position]);
            buffer_position++;
            hdata++;
        }

        int hdatasize = im_datasize-buffer_position;

        // Upsample and preprocess input
        start = get_time();
        input = server_preprocessing(raw_input, (uint8_t*)hdata, hdatasize, noise_data, noise_datasize, params.upsampling_factor, params.width, params.height);
        PRINT("- server preprocess input (%d) : %ldms\n", params.upsampling_factor, get_duration(start,get_time()));
    }

    Image<uint32_t> out(input.width(), input.height());

    // Process image
    start = get_time();
    if(strcmp(params.filter_type, "local_laplacian") == 0) {
        float beta = 1.0f;
        hl_local_laplacian(params.levels, params.alpha/(params.levels-1), beta, input, out);
    } else if(strcmp(params.filter_type, "style_transfer") == 0) {
        Image<uint32_t> target = jpeg_decompress((unsigned char*) extra_data,extra_datasize);
        style_transfer(input, target, params.levels, out, params.iterations);
    } else if(strcmp(params.filter_type, "colorization") == 0) {
        Image<uint32_t> scribbles = jpeg_decompress((unsigned char*) extra_data,extra_datasize);
        colorize(input, scribbles, out);
    }
    PRINT("- filter : %ldms\n", get_duration(start,get_time()));

    // Fit recipe
    start = get_time();
    xform::Recipe recipe(input, out);

    // hp, lp, qtable
    std::shared_ptr<Image<uint32_t> >lp_res = recipe.lowpass_residual();
    std::shared_ptr<Image<float> > hp_coefs  = recipe.highpass_coefficients();

    std::vector<float> qTable = recipe.qtable();
    PRINT("- recipe : %ldms\n", get_duration(start,get_time()));

    // Save Recipe output
    start = get_time();
    unsigned char* recipe_data = nullptr;
    unsigned long recipe_size = 0;
    save_recipe(*lp_res, *hp_coefs, qTable, &recipe_data, &recipe_size);
    *output = (char *)recipe_data;
    *output_datasize = recipe_size;
    PRINT("- save:\t %ldms\n", get_duration(start,get_time()));
}


void fit_recipe(
    char *im_data, int im_datasize,
    char *im_target_data, int im_target_datasize,
    char **output, int *output_datasize
)
{

    // Decompress JPEG input/output pair from binary string
    Image<uint32_t> input = jpeg_decompress((unsigned char*) im_data,im_datasize);
    Image<uint32_t> out   = jpeg_decompress((unsigned char*) im_target_data,im_target_datasize);

    // Fit recipe
    xform::Recipe recipe(input, out);

    // hp, lp, qtable
    std::shared_ptr<Image<uint32_t> > lp_res = recipe.lowpass_residual();
    std::shared_ptr<Image<float> > hp_coefs  = recipe.highpass_coefficients();
    std::vector<float> qTable = recipe.qtable();

    // Save Recipe output to binary string
    unsigned char* recipe_data = nullptr;
    unsigned long recipe_size = 0;
    save_recipe(*lp_res, *hp_coefs, qTable, &recipe_data, &recipe_size);
    *output = (char *)recipe_data;
    *output_datasize = recipe_size;
}
