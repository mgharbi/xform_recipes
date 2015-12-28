#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mghimproc.h"

void resize(double* in, int i_height, int i_width,
            double* weights, int w_height, int w_width,
            long* indices, int idx_height, int idx_width,
            int o_height,
            int o_width,
            int dim,
            double* out, int numel)
{
    memset(out,0,sizeof(double)*numel);
    int index  = 0;
    double val = 0;
    if( dim == 0 ){
        for( int i = 0 ; i<o_height; ++i )
            for( int j = 0 ; j<o_width; ++j )
        {
            index = i*o_width + j;
            val   = 0;
            for(int s_id = 0; s_id < w_width; ++s_id){
                val += in[ indices[i*w_width+s_id]*i_width + j]*weights[i*w_width+s_id];
            }
            out[index] = val;
        }
    }else{
        for( int i = 0 ; i<o_height; ++i )
            for(int j = 0 ; j<o_width; ++j)
        {
            index = i*o_width + j;
            val   = 0;
            for( int s_id = 0; s_id < w_width; ++s_id ){
                val += in[indices[j*w_width+s_id] +i*i_width]*weights[j*w_width+s_id];
            }
            out[index] = val;
        }
    }
}
