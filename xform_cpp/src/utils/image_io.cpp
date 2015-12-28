#include "utils/image_io.h"
#include <jpeglib.h>
#include <vector>
#define _assert(condition, ...) if (!(condition)) {fprintf(stderr, __VA_ARGS__); exit(-1);}

Image<uint32_t> jpeg_decompress(unsigned char * data, int size, int* reader_position) {
    // Open JPEG 
    jpeg_decompress_struct cinfo;
    jpeg_error_mgr error_mgr;
	cinfo.err = jpeg_std_error(&error_mgr);
    jpeg_create_decompress(&cinfo);
	jpeg_mem_src(&cinfo, data, size);

	jpeg_read_header(&cinfo, TRUE);
	jpeg_start_decompress(&cinfo);

    // Create image structure
    int width     = cinfo.image_width;
    int height    = cinfo.image_height;
    int channels  = cinfo.num_components;
    Image<uint32_t> output(width,height);

    // Read RGB jpeg
    uint8_t r,g,b;
    uint32_t *ptr = output.data();
    uint8_t *scanline = new uint8_t[width*channels];
    while(cinfo.output_scanline < cinfo.image_height) {
		jpeg_read_scanlines(&cinfo, &scanline, 1);
        uint8_t *current = scanline;
        for (int x = 0; x < output.width(); x++) {
            r = *current++;
            g = *current++;
            b = *current++;
            *ptr = r | (g << 8) | (b << 16);
            ptr++;
        }
	}
    delete[] scanline;
    scanline = nullptr;
    ptr = nullptr;

    if(reader_position) {
        *reader_position = size-cinfo.src->bytes_in_buffer;
    }

    jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

    return output;
}


void jpeg_compress(const Image<uint32_t> &input, int quality, unsigned char** data,  unsigned long *size) {
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_mem_dest(&cinfo, data, size);
    
    cinfo.image_width      = input.width();
    cinfo.image_height     = input.height();
    cinfo.input_components = 3;
    cinfo.in_color_space   = JCS_RGB;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, true);
    jpeg_start_compress(&cinfo, true);
 
    uint8_t r,g,b;
    uint32_t *ptr = input.data();
    uint8_t *scanline = new uint8_t[cinfo.image_width*cinfo.input_components];
    while (cinfo.next_scanline < cinfo.image_height) {
        uint8_t *current = scanline;
        for (int x = 0; x < input.width(); x++) {
            r = ((*ptr) >>  0) & 0xff;
            g = ((*ptr) >>  8) & 0xff;
            b = ((*ptr) >> 16) & 0xff;
            current[0] = r;
            current[1] = g;
            current[2] = b;
            current += 3;
            ptr++;
        }
        jpeg_write_scanlines(&cinfo, &scanline, 1);
    }
    delete[] scanline;
    scanline = nullptr;
    ptr = nullptr;
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
}

Image<uint32_t> jpeg_load(const char *path) {
    FILE * f = fopen(path, "r");
    if(!f){
        throw "could not open .jpg file";
    }

    // Open JPEG 
    jpeg_decompress_struct cinfo;
    jpeg_error_mgr error_mgr;
	cinfo.err = jpeg_std_error(&error_mgr);
    jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, f);

	jpeg_read_header(&cinfo, TRUE);
	jpeg_start_decompress(&cinfo);

    // Create image structure
    int width     = cinfo.image_width;
    int height    = cinfo.image_height;
    int channels  = cinfo.num_components;
    Image<uint32_t> output(width,height);

    // Read RGB jpeg
    uint8_t r,g,b;
    uint32_t *ptr = output.data();
    uint8_t *scanline = new uint8_t[width*channels];
    while(cinfo.output_scanline < cinfo.image_height) {
		jpeg_read_scanlines(&cinfo, &scanline, 1);
        uint8_t *current = scanline;
        for (int x = 0; x < output.width(); x++) {
            r = *current++;
            g = *current++;
            b = *current++;
            *ptr = r | (g << 8) | (b << 16);
            ptr++;
        }
	}
    delete[] scanline;
    scanline = nullptr;
    ptr = nullptr;

    jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

    return output;
}

void jpeg_save(const Image<uint32_t> &input, int quality, const char * path) {
    FILE * f = fopen(path, "w");
    if(!f){
        throw "could not open .jpg file for writing";
    }
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, f);
    
    cinfo.image_width      = input.width();
    cinfo.image_height     = input.height();
    cinfo.input_components = 3;
    cinfo.in_color_space   = JCS_RGB;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, true);
    jpeg_start_compress(&cinfo, true);
 
    uint8_t r,g,b;
    uint32_t *ptr = input.data();
    uint8_t *scanline = new uint8_t[cinfo.image_width*cinfo.input_components];
    while (cinfo.next_scanline < cinfo.image_height) {
        uint8_t *current = scanline;
        for (int x = 0; x < input.width(); x++) {
            r = ((*ptr) >>  0) & 0xff;
            g = ((*ptr) >>  8) & 0xff;
            b = ((*ptr) >> 16) & 0xff;
            current[0] = r;
            current[1] = g;
            current[2] = b;
            current += 3;
            ptr++;
        }
        jpeg_write_scanlines(&cinfo, &scanline, 1);
    }
    delete[] scanline;
    scanline = nullptr;
    ptr = nullptr;
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
}

void write_png_to_memory(png_structp  png_ptr, png_bytep data, png_size_t length) {
    std::vector<uint8_t> *p = (std::vector<uint8_t>*)png_get_io_ptr(png_ptr);
    p->insert(p->end(), data, data + length);
}

void read_png_from_memory(png_structp  png_ptr, png_bytep data, png_size_t length) {
    std::vector<uint8_t> *p = (std::vector<uint8_t>*)png_get_io_ptr(png_ptr);
    // p->insert(p->end(), data, data + length);
}


void png_compress(const Image<uint32_t> &input, unsigned char** data,  unsigned long *size) {
    png_structp png_ptr;
    png_infop info_ptr;

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    _assert(png_ptr, "[write_png_file] png_create_write_struct failed\n");
    info_ptr = png_create_info_struct(png_ptr);
    _assert(info_ptr, "[write_png_file] png_create_info_struct failed\n");

    int bit_depth = 8;

    int h = input.height();
    int w = input.width();

    png_set_IHDR(png_ptr, info_ptr, w, h,
                 bit_depth, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    uint32_t *ptr = (uint32_t*)input.data();
    std::vector<uint8_t*> row_pointers(h);
    for (int y = 0; y < h; ++y) {
        row_pointers[y] = new uint8_t[png_get_rowbytes(png_ptr, info_ptr)];
        uint8_t *dstPtr = (uint8_t *)(row_pointers[y]);
        for (int x = 0; x < w; x++) {
            *dstPtr++ = ((*ptr) >>  0) & 0xff; // R
            *dstPtr++ = ((*ptr) >>  8) & 0xff; // G
            *dstPtr++ = ((*ptr) >> 16) & 0xff; // B
            ptr++;
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

    for (int y = 0; y < h; y++) {
        delete[] row_pointers[y];
    }
}

void png_save(const Image<uint32_t> &input, const char* path) {
    FILE * f = fopen(path, "w");
    if(!f){
        throw "could not open .png file for writing";
    }
    png_structp png_ptr;
    png_infop info_ptr;

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    _assert(png_ptr, "[write_png_file] png_create_write_struct failed\n");
    info_ptr = png_create_info_struct(png_ptr);
    _assert(info_ptr, "[write_png_file] png_create_info_struct failed\n");

    png_init_io(png_ptr, f);

    int bit_depth = 8;

    int h = input.height();
    int w = input.width();

    png_set_IHDR(png_ptr, info_ptr, w, h,
                 bit_depth, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_write_info(png_ptr, info_ptr);

    uint32_t *ptr = (uint32_t*)input.data();
    uint8_t* row_pointer = new uint8_t[png_get_rowbytes(png_ptr, info_ptr)];
    for (int y = 0; y < h; ++y) {
        uint8_t *dstPtr = row_pointer;
        for (int x = 0; x < w; x++) {
            *dstPtr++ = ((*ptr) >>  0) & 0xff; // R
            *dstPtr++ = ((*ptr) >>  8) & 0xff; // G
            *dstPtr++ = ((*ptr) >> 16) & 0xff; // B
            ptr++;
        }
        png_write_row(png_ptr, row_pointer);
    }
    delete row_pointer;

    png_write_end(png_ptr, NULL);
    png_destroy_write_struct(&png_ptr, &info_ptr);

}

void png_save(const Image<float> &input, const char* path) {
//     FILE * f = fopen(path, "w");
//     if(!f){
//         throw "could not open .png file for writing";
//     }
//     png_structp png_ptr;
//     png_infop info_ptr;
//
//     png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
//     _assert(png_ptr, "[write_png_file] png_create_write_struct failed\n");
//     info_ptr = png_create_info_struct(png_ptr);
//     _assert(info_ptr, "[write_png_file] png_create_info_struct failed\n");
//
//     png_init_io(png_ptr, f);
//
//     int bit_depth = 8;
//
//     int h = input.height();
//     int w = input.width();
//
//     png_set_IHDR(png_ptr, info_ptr, w, h,
//                  bit_depth, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
//                  PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
//
//     png_write_info(png_ptr, info_ptr);
//
//     uint8_t* row_pointer = new uint8_t[png_get_rowbytes(png_ptr, info_ptr)];
//     for (int y = 0; y < h; ++y) {
//         uint8_t *dstPtr = row_pointer;
//         for (int x = 0; x < w; x++) {
//             *dstPtr++ = input(x,y,0); // R
//             *dstPtr++ = input(x,y,1); // R
//             *dstPtr++ = input(x,y,2); // R
//         }
//         png_write_row(png_ptr, row_pointer);
//     }
//     delete row_pointer;
//
//     png_write_end(png_ptr, NULL);
//     png_destroy_write_struct(&png_ptr, &info_ptr);
//
// }
//
// Image<uint32_t> png_decompress(unsigned char *data, int size, int* reader_position) {
//     png_structp png_ptr;
//     png_infop info_ptr;
//
//     png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
//     _assert(png_ptr, "[read_png_file] png_create_read_struct failed\n");
//     info_ptr = png_create_info_struct(png_ptr);
//     _assert(info_ptr, "[write_png_file] png_create_info_struct failed\n");
//
//     png_set_read_fn(png_ptr, (png_voidp) &data, write_png_to_memory);
//
//     png_read_info(png_ptr, info_ptr);
//
//     png_uint_32 width  = 0;
//     png_uint_32 height = 0;
//     int bitDepth       = 0;
//     int colorType      = -1;
//     png_uint_32 retval = png_get_IHDR(png_ptr, info_ptr,
//             &width,
//             &height,
//             &bitDepth,
//             &colorType,
//             NULL, NULL, NULL);
//
//     if(retval != 1){
//         // error
//     }
//
//     Image<uint32_t> output(width,height);
//
//     const png_uint_32 bytesPerRow = png_get_rowbytes(png_ptr, info_ptr);
//     uint8_t* rowData = new uint8_t[bytesPerRow];
//
//     uint32_t *ptr = (uint32_t*)output.data();
//     std::vector<uint8_t*> row_pointers(height);
//     for (unsigned int y = 0; y < height; ++y) {
//         row_pointers[y] = new uint8_t[png_get_rowbytes(png_ptr, info_ptr)];
//         uint8_t *dstPtr = (uint8_t *)(row_pointers[y]);
//         for (unsigned int x = 0; x < width; x++) {
//             *dstPtr++ = ((*ptr) >>  0) & 0xff; // R
//             *dstPtr++ = ((*ptr) >>  8) & 0xff; // G
//             *dstPtr++ = ((*ptr) >> 16) & 0xff; // B
//             ptr++;
//         }
//     }
//
//     png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
//
//     return output;
}
