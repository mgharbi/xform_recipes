%module pyrecipe

%{
#define SWIG_FILE_WITH_INIT
#include "pyrecipe.h"
%}

%include "numpy.i"
%include "typemaps.i"
%include "cstring.i"

%init %{
import_array();
%}

%apply (char *STRING, int LENGTH) { 
    (char *im_data, int im_datasize), 
    (char *im_target_data, int im_target_datasize), 
    (char *ref_im_data, int ref_im_datasize), 
    (char *extra_data, int extra_datasize),
    (char *recipe_data, int recipe_datasize),
    (char *noise_data, int noise_datasize)
};

%cstring_output_allocate_size(char **output, int *output_datasize, delete *$1);

%include "pyrecipe.h"
