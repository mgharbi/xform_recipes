#ifndef IMAGE_IO_H_QKFDBCIE
#define IMAGE_IO_H_QKFDBCIE

#include "utils/static_image.h"
#include <png.h>

Image<uint32_t> jpeg_decompress(unsigned char *data, int size, int* reader_position = nullptr);
void jpeg_compress(const Image<uint32_t> &input, int quality, unsigned char** data,  unsigned long *size);

Image<uint32_t> jpeg_load(const char * path);
void jpeg_save(const Image<uint32_t> &input, int quality, const char * path);

void write_png_to_memory(png_structp  png_ptr, png_bytep data, png_size_t length);

void png_compress(const Image<uint32_t> &input, unsigned char** data,  unsigned long *size);
void png_save(const Image<uint32_t> &input, const char* path);
void png_save(const Image<float> &input, const char* path);

Image<uint32_t> png_decompress(unsigned char *data, int size, int* reader_position = nullptr);

#endif /* end of include guard: IMAGE_IO_H_QKFDBCIE */
