#ifndef PYRECIPE_H_BVJRMPOZ
#define PYRECIPE_H_BVJRMPOZ

typedef struct XformParams {
    // filter-related parameters
    int levels;
    int iterations;
    float alpha;
    char* filter_type;

    // jpeg output parameters
    int jpeg_quality;

    // proxy input parameters
    int width;
    int height;
    int upsampling_factor;

} XformParams;

void naive_processing(
    char *im_data, int im_datasize,
    char *extra_data, int extra_datasize,
    XformParams params,
    char **output, int *output_datasize
);

void recipe_processing(
    char *im_data, int im_datasize,
    char *extra_data, int extra_datasize,
    char *noise_data, int noise_datasize,
    XformParams params,
    char **output, int *output_datasize
);

void fit_recipe(
    char *im_data, int im_datasize,
    char *im_target_data, int im_target_datasize,
    char **output, int *output_datasize
);

void input_preprocessing(
    char *im_data, int im_datasize,
    char *noise_data, int noise_datasize,
    XformParams params,
    char **output, int *output_datasize
);


#endif /* end of include guard: PYRECIPE_H_BVJRMPOZ */

