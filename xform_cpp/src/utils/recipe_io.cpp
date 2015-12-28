#include "utils/recipe_io.h"
#include "utils/image_io.h"
#include <png.h>
#define _assert(condition, ...) if (!(condition)) {fprintf(stderr, __VA_ARGS__); exit(-1);}

#include "print_helper.h"

using std::min;
using std::max;

void save_recipe(const Image<uint32_t> &lp_res, const Image<float> &hp_coefs, std::vector<float> qTable, unsigned char **data, unsigned long *size) {
    // LP
    unsigned char* lp_data;
    unsigned long lp_sz;
    png_compress(lp_res, &lp_data, &lp_sz);

    // HP
    unsigned char* hp_data;
    unsigned long hp_sz;
    save_highpass(hp_coefs, &hp_data, &hp_sz);

    int qsize = qTable.size();

    // Aggregatted data
    *size = 3*sizeof(int) + lp_sz + hp_sz + sizeof(float)*qsize;
    *data = new unsigned char[*size];

    int lp = lp_sz;
    int hp = hp_sz;

    // Copy byte sizes
    int idx = 0;
    memcpy((*data)+idx, &lp, sizeof(int));
    idx += sizeof(int);
    memcpy((*data)+idx, &hp, sizeof(int));
    idx += sizeof(int);
    memcpy((*data)+idx, &qsize, sizeof(int));
    idx += sizeof(int);

    // Copy data
    memcpy((*data)+idx, lp_data, lp_sz);
    idx += lp_sz;
    memcpy((*data)+idx, hp_data, hp_sz);
    idx += hp_sz;
    memcpy((*data)+idx,&qTable[0], sizeof(float)*qsize);
}


void save_highpass(const Image<float> &input, unsigned char** data,  unsigned long *size) {
    png_structp png_ptr;
    png_infop info_ptr;

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    _assert(png_ptr, "[write_png_file] png_create_write_struct failed\n");
    info_ptr = png_create_info_struct(png_ptr);
    _assert(info_ptr, "[write_png_file] png_create_info_struct failed\n");

    int bit_depth = 8;

    int h = input.height();
    int w = input.width();
    int c = input.channels();

    png_set_IHDR(png_ptr, info_ptr, w, h*c,
                 bit_depth, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    std::vector<uint8_t*> row_pointers(h*c);
    for (int z = 0; z < c; ++z) {
        for (int y = 0; y < h; ++y) {
            row_pointers[y + z*h] = new uint8_t[png_get_rowbytes(png_ptr, info_ptr)];
            uint8_t *dstPtr = (uint8_t *)(row_pointers[y + z*h]);
            for (int x = 0; x < w; x++) {
                *dstPtr++ = static_cast<uint8_t>(input(x,y,z)); // assumes clamping has been done at quantization
            }
        }
    }

    std::vector<uint8_t> temp_data;
    png_set_rows(png_ptr, info_ptr, &row_pointers[0]);
    png_set_write_fn(png_ptr, &temp_data, write_png_to_memory, NULL);
    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
    png_destroy_write_struct(&png_ptr, &info_ptr);

    *size = temp_data.size();
    *data = new unsigned char[*size];
    memcpy(*data,&temp_data[0], sizeof(uint8_t)*(*size));

    for (int y = 0; y < h*c; y++) {
        delete[] row_pointers[y];
    }
}
