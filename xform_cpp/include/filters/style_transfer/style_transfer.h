#ifndef STYLE_TRANSFER_WRAPPER_H_NXOCJ6DN
#define STYLE_TRANSFER_WRAPPER_H_NXOCJ6DN

#include <cstdio>
#include <vector>
#include <iostream>
#include <sys/time.h>
#include "utils/static_image.h"

#include "const.h"

using namespace std;

int style_transfer(Image<uint32_t> &input, Image<uint32_t> &model, int levels, Image<uint32_t> &output, int n_iterations);

#endif /* end of include guard: STYLE_TRANSFER_WRAPPER_H_NXOCJ6DN */

