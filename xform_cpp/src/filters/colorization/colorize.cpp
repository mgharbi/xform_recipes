/* -----------------------------------------------------------------
 * File:    colorize.cpp
 * Author:  Michael Gharbi <gharbi@mit.edu>
 * Created: 2015-07-27
 * -----------------------------------------------------------------
 * 
 * 
 * 
 * ---------------------------------------------------------------*/


#include <algorithm>
#include <numeric>
#include <iterator>
#include <vector>
#include <cmath>
#include <string>
#include <iostream>

#include "filters/colorization/hl_yuv2rgb.h"
#include "filters/colorization/hl_nonzero.h"
#include "filters/colorization/hl_fuse_yuv.h"
#include "filters/colorization/linear_solver.h"
#include "filters/colorization/mg.h"
#include "filters/colorization/fmg.h"

#include "filters/colorization/colorize.h"

#include "perf_measure.h"

using std::min;
using std::max;
using std::log;
using std::floor;
using std::ceil;
using std::pow;

void colorize(const Image<uint32_t> &input, const Image<uint32_t> &strokes, Image<uint32_t> &output) {
    int w     = input.width();
    int h     = input.height();
    int x,y;

    int max_d          = floor(log(min(h,w))/log(2)-2);
    float scale_factor = 1.0f/( pow(2,max_d-1) );
    int padded_w       = ceil(w*scale_factor)*pow(2,max_d-1);
    int padded_h       = ceil(h*scale_factor)*pow(2,max_d-1);

    // RGB 2 YUV and padarray
    Image<float> yuv_fused(padded_w,padded_h,3);
    hl_fuse_yuv(input, strokes, yuv_fused);


    // Extract Strokes mask
    Image<float> stroke_mask(padded_w, padded_h);
    hl_nonzero(strokes,stroke_mask);

    Image<float> result(padded_w,padded_h,3);

    int n = padded_h;
    int m = padded_w;
    int k = 1;

    Tensor3d D,G,I;
    Tensor3d Dx,Dy,iDx,iDy;
    MG smk;
    G.set(n,m,k);
    D.set(n,m,k);
    I.set(n,m,k);

    int in_itr_num  = 5;
    int out_itr_num = 1;

    Dx.set(n,m,k-1);
    Dy.set(n,m,k-1);
    iDx.set(n,m,k-1);
    iDy.set(n,m,k-1);

    // Fill in label mask and luminance channels
    for ( y = 0; y<n; y++){
        for ( x = 0; x<m; x++){
            I(y,x,0) = stroke_mask(x,y);
            G(y,x,0) = yuv_fused(x,y,0);
            I(y,x,0) = !I(y,x,0);
        }
    }

    // Write output luminance
    for ( y=0; y<n; y++){
        for ( x=0; x<m; x++){
            result(x,y,0)=G(y,x,0);
        }
    }

    smk.set(n,m,k,max_d);
    smk.setI(I) ;
    smk.setG(G);
    smk.setFlow(Dx,Dy,iDx,iDy);

    // Solve chrominance
    for (int t=1; t<3; t++){
        for ( y=0; y<n; y++){
            for ( x=0; x<m; x++){
                D(y,x,0)       = yuv_fused(x,y,t);
                smk.P()(y,x,0) = yuv_fused(x,y,t);
                D(y,x,0)      *= (!I(y,x,0));
            }
        }

        smk.Div() = D ;

        Tensor3d tP2;

        for (int itr=0; itr<out_itr_num; itr++){
            smk.setDepth(max_d);
            Field_MGN(&smk, in_itr_num, 2) ;
            smk.setDepth(ceil(max_d/2));
            Field_MGN(&smk, in_itr_num, 2) ;
            smk.setDepth(2);
            Field_MGN(&smk, in_itr_num, 2) ;
            smk.setDepth(1);
            Field_MGN(&smk, in_itr_num, 4) ;
        }

        tP2 = smk.P();

        for ( y=0; y<n; y++){
            for ( x=0; x<m; x++){
                result(x,y,t) = tP2(y,x,0);
            }
        }
    }
    
    hl_yuv2rgb(result,output);
    

}
