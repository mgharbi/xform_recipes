#include "filters/style_transfer/style_transfer.h"
#include "filters/style_transfer/hl_rgb2gray.h"
#include "filters/style_transfer/hl_gray2rgb.h"
#include "filters/style_transfer/hl_style_transfer.h"
#include "perf_measure.h"

#include "utils/histogram/histogram.h"

int style_transfer(Image<uint32_t> &input, Image<uint32_t> &model, int levels, Image<uint32_t> &output, int n_iterations)
{
    int source_precision = 256;
    int target_precision = 256*256;

    // Convert Input and example to Gray
    auto start = get_time();
    Image<float> gray_input(input.width(),input.height());
    hl_rgb2gray(input, gray_input);
    Image<float> gray_model(model.width(), model.height());
    hl_rgb2gray(model, gray_model);
    printf("- rgb2gray : %ldms\n", get_duration(start,get_time()));

    // Get example's gradient magnitude histogram
    start = get_time();
    Histogram GM = histogram_of_gradients(gray_model, target_precision);
    printf("- histogram (halide) : %ldms\n", get_duration(start,get_time()));


    start = get_time();
    // Apply Gray transfer function
    Histogram h_input = histogram(gray_input,source_precision);
    Histogram h_model = histogram(gray_model,target_precision);
    printf("input: %f %f\n", h_input.mini, h_input.maxi);
    printf("target: %f %f\n", h_model.mini, h_model.maxi);
    TransferFunction f(h_input, h_model);
    Image<float> current = transfer(gray_input,f);
    printf("- transfer gray (halide) : %ldms\n", get_duration(start,get_time()));

    for (int i = 0; i < n_iterations; ++i) {
        start = get_time();
        // Gradient transfer
        Histogram GI = histogram_of_gradients(current, source_precision);

        f = TransferFunction(GI, GM);
        Image<float> h_f(f.size);
        memcpy(h_f.data(),&f.values[0], f.size*sizeof(float));

        Image<float> temp(current.width(), current.height());
        hl_style_transfer(levels, current, h_f, f.source_mini, f.source_maxi, f.target_mini, f.target_maxi, temp);
        current = temp;

        // Value transfer
        h_input = histogram(current, source_precision);
        f       = TransferFunction(h_input, h_model);
        temp    = transfer(current,f);
        current = temp;

        printf("- iteration %d : %ldms\n", i,  get_duration(start,get_time()));
    }

    hl_gray2rgb(current,output);

    return 0;
}
