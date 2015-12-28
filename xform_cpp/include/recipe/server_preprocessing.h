#ifndef SERVER_PREPROCESSING_H_IQXLMQNO
#define SERVER_PREPROCESSING_H_IQXLMQNO

#include "utils/static_image.h"

Image<uint32_t> server_preprocessing(
        Image<uint32_t> &raw_input,
        const uint8_t* hdata, unsigned long hdatasize,
        const char* noise_data, int noise_datasize,
        int upsampling_factor, 
        int width,
        int height
);


#endif /* end of include guard: SERVER_PREPROCESSING_H_IQXLMQNO */

