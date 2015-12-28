#ifndef RECIPE_IO_H_XJM0PZBT
#define RECIPE_IO_H_XJM0PZBT

#include "utils/static_image.h"
#include <vector>

void save_highpass(const Image<float> &input, unsigned char** data,  unsigned long *size);
void save_recipe(const Image<uint32_t> &lp_res, const Image<float> &hp_coefs, std::vector<float> qTable, unsigned char **data, unsigned long *size);


#endif /* end of include guard: RECIPE_IO_H_XJM0PZBT */

