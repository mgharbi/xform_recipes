%module mghimproc

%{
#define SWIG_FILE_WITH_INIT
#include "mghimproc.h"
%}

%include "numpy.i"

%init %{
import_array();
%}

%apply (double* IN_ARRAY2, int DIM1, int DIM2){(double* in, int i_height, int i_width)};
%apply (double* IN_ARRAY3, int DIM1, int DIM2, int DIM3){(double* in, int i_height, int i_width, int i_chan)};
%apply (double* IN_ARRAY2, int DIM1, int DIM2){(double* weights, int w_height, int w_width)};
%apply (double* IN_ARRAY2, int DIM1, int DIM2){(double* weights_x, int wx_height, int wx_width)};
%apply (double* IN_ARRAY2, int DIM1, int DIM2){(double* weights_y, int wy_height, int wy_width)};
%apply (long* IN_ARRAY2, int DIM1, int DIM2){(long* indices, int idx_height, int idx_width)};
%apply (long* IN_ARRAY2, int DIM1, int DIM2){(long* idx_x, int ix_height, int ix_width)};
%apply (long* IN_ARRAY2, int DIM1, int DIM2){(long* idx_y, int iy_height, int iy_width)};

%apply (double* ARGOUT_ARRAY1, int DIM1){(double* out, int numel)}
%apply (long* ARGOUT_ARRAY1, int DIM1){(long* I, int Inumel)}
%apply (long* ARGOUT_ARRAY1, int DIM1){(long* J, int Jnumel)}

%include "mghimproc.h"
