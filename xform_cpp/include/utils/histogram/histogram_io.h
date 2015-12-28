#ifndef HISTOGRAM_IO_H_6GLAUN25
#define HISTOGRAM_IO_H_6GLAUN25

#include <vector>
#include "utils/histogram/histogram.h"
using std::vector;

void compress_histogram(vector<vector<Histogram> > histograms, unsigned char **data, unsigned long* datasize);
vector<vector<Histogram> > decompress_histogram(const unsigned char* data, unsigned long datasize);


#endif /* end of include guard: HISTOGRAM_IO_H_6GLAUN25 */

