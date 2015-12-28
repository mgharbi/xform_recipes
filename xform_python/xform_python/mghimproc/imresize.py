# ----------------------------------------------------------------------------
# File:    imresize.py
# Author:  Michael Gharbi <gharbi@mit.edu>
# Created: 2015-02-12
# ----------------------------------------------------------------------------
#
# Matlab-style image resizing with a proper anti-aliasing filter for
# downsampling.
#
# ---------------------------------------------------------------------------#


import math
import numpy as np
import scipy as sp
from scipy.sparse import csr_matrix, lil_matrix, coo_matrix
import mghimproc

def imresize(I,Oshape, method = "linear"):
    isint = (I.dtype.name == 'uint8')
    if len(I.shape)>2:
        chans = []
        for c in range(I.shape[2]):
            chans.append(_imresize(I[:,:,c],Oshape))
        O = np.dstack(chans)
    else:
        O = _imresize(I,Oshape, method = method)
    if isint:
        O = np.uint8(np.round(np.clip(O,0,255)))
    return O


def getScaleAndShape(Ishape,Oshape):
    if np.isscalar(Oshape):
        # Input is a scale factor
        scale = np.array([Oshape, Oshape])
        Oshape = np.ceil(scale*np.array(Ishape[0:2]))
    else:
        # Input is a shape
        scale = np.array(Oshape[0:2],dtype=float)/np.array(Ishape[0:2],dtype=float)
    return (scale,Oshape)

def getProcessOrder(scale):
    # Order of dimensions to process. Resize first along the dimension with
    # the smallest scale factor
    order = np.argsort(scale)
    return order

def _imresize(I,Oshape, method = "linear"):
    (scale, Oshape) = getScaleAndShape(I.shape, Oshape)

    if method == "cubic":
        kernel = cubic
        kernel_width = 4.0
    elif method == "linear":
        kernel = linear
        kernel_width = 2.0
    elif method == "nearest":
        kernel = box
        kernel_width = 1.0
    else:
        print "Error in _imresize: method unknown"
    order = getProcessOrder(scale)
    weights = 2*[None]
    indices = 2*[None]
    for k in order:
        (w,idx) = contributions(I.shape[k], Oshape[k], scale[k],  kernel, kernel_width)
        weights[k] = w
        indices[k] = idx
    O = I
    for k in order:
        O = resizeAlongDim(O,k,weights[k],indices[k])
    return O

def resizeAlongDim(O,dim,weights,indices):
    s      = list(O.shape)
    s[dim] = weights.shape[0]
    ret =  mghimproc.resize(O,weights,indices, s[0], s[1],dim, s[0]*s[1])
    ret = np.reshape(ret,s)
    return ret

def contributions(Ilength, Olength, scale, kernel,kernel_width):
    # Antialiasing for downsizing
    if scale < 1:
        h = lambda x: kernel(x,scale)
        kernel_width = kernel_width/scale
    else:
        h = kernel

    # output space coordinate
    x            = np.arange(Olength, dtype = float)
    x.shape     += (1,)
    # input space coord so that 0.5 in Out ~ 0.5 in In, and 0.5+scale in Out ~
    # 0.5 + 1 in In
    u            = x/scale + 0.5*(-1+1.0/scale)
    left         = np.floor(u-kernel_width/2)
    P            = math.ceil(kernel_width) + 2
    indices      = left + np.arange(P)
    weights      = h(u - indices)
    norm         = np.sum(weights,axis=1)
    norm.shape  += (1,)
    weights      = weights/norm
    indices      = np.minimum(np.maximum(0,indices),Ilength-1)
    indices      = np.array(indices,dtype                      = int)


    kill    = np.ma.any(weights,0)
    weights = np.compress(kill,weights,1)
    indices = np.compress(kill,indices,1)
    return (weights,indices)

# Interpolation kernel
def _cubic(x, scale = 1):
    absx  = math.fabs(x*scale)
    absx2 = absx*absx
    absx3 = absx2*absx

    f = (1.5*absx3 - 2.5*absx2 + 1)*(absx<=1)
    f += (-0.5*absx3 + 2.5*absx2 - 4*absx + 2)*((1<absx) and (absx<=2))
    f = scale*f
    return f

def _linear(x, scale = 1):
    x = scale * x

    f = (x+1)*((-1<=x) & (x < 0)) + (1-x)*((0<=x) & (x <= 1))
    f = scale*f
    return f

def _box(x, scale = 1):
    x = scale * x
    f = (-0.5 <= x) & (x <0.5)
    f = scale*f
    return f

cubic  = np.vectorize(_cubic)
linear = np.vectorize(_linear)
box    = np.vectorize(_box)
