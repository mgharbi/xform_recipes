# -------------------------------------------------------------------
# File:    input_transfer.py
# Author:  Michael Gharbi <gharbi@mit.edu>
# Created: 2015-11-20
# -------------------------------------------------------------------
# 
# 
# 
# ------------------------------------------------------------------#


import os
import sys
import numpy as np

from PIL import Image

from scipy.interpolate import interp1d
import argparse

from settings import DATA_DIR, OUTPUT_DIR
from mghimproc import buildLaplacianPyramid, reconstructFromLaplacianPyramid, buildGaussianPyramid
from mghimproc import color
from mghimproc import imresize, save_and_get_img

noisemap = np.random.randn(6000,6000,3)
def add_noise(I, sigma = 1.0):
    """ Add Gaussian random noise to input I """

    sz = I.shape
    I = I.astype(np.float32)
    if len(sz) > 2:
        I = I+sigma*noisemap[:sz[0], :sz[1], :sz[2]]
    else:
        I = I+sigma*noisemap[:sz[0], :sz[1],0]
    return I

def get_mean_and_variance(h,bins):
    """ Extract statistics for histogram """

    center = .5*(bins[1:]+bins[:-1])
    n = np.sum(h)*1.0
    mu = np.sum(center*h) /n
    var = np.sum(np.square(center)*h) /n - np.square(mu)
    return mu,var


def transfer(Id,hist_ref,rng_ref, deg_res = 256, ref_res = 256*2, hist_resample = 10, transfer_color = True, output_dir = None):
    """ Transfer histogram hist_ref whose range of values is rng_ref 
        to image Id. The resolution of the reference histogram is
        ref_res and the resolution of the target's histogram is def_res.
        Specifiy a resampling factor before inverting the histogram.
    """

    nlevels = hist_ref[1]

    nit = 1
    if transfer_color:
        nit = 1

    for iteration in range(nit):
        print "iteration %d" % iteration

        LId = buildLaplacianPyramid(Id, nLevels = nlevels + 1)
        LO = []
        # Remap Laplacian
        for il in range(nlevels):
            ld   = LId[il]
            ld = add_noise(ld,sigma = 3.0)

            lo = np.zeros(ld.shape)
            for c in range(ld.shape[2]):
                ldc = ld[:,:,c]
                h,bins    = extract_histogram(hist_ref,rng_ref,il,c)

                f,m,M     = get_transfer_function(ldc,h,bins,deg_res,hist_resample*ref_res)
                lo[:,:,c] = apply_transfer_function(ldc, f,m, M)


            if len(lo.shape) == 2:
                lo.shape += (1,)
            LO.append(lo)
        LO.append(LId[-1])
        Id = reconstructFromLaplacianPyramid(LO)
        Id = np.squeeze(Id)

        # Remap colors
        if transfer_color:
            il = nlevels
            for c in range(Id.shape[2]):
                h,bins    = extract_histogram(hist_ref,rng_ref,il,c)
                f,m,M     = get_transfer_function(Id[:,:,c],h,bins,deg_res,hist_resample*ref_res)
                Id[:,:,c] = apply_transfer_function(Id[:,:,c], f,m, M)


    # Clamp values
    Id[Id<0]   = 0
    Id[Id>255] = 255
    return Id


def get_histograms(I, nlevels = 3, resolution = 256):
    """ Compute rescaling histograms """

    if len(I.shape) > 2:
        nchan = I.shape[2]
    else:
        nchan = 1

    nHistPerChannel = nlevels + 1
    LI            = buildLaplacianPyramid(I,nLevels = nlevels + 1)
    histograms    = np.zeros((nchan*nHistPerChannel*((resolution-1)) + 3,), dtype=np.int32)

    histograms[0] = resolution
    histograms[1] = nlevels
    histograms[2] = nchan
    idx           = 3

    rng = np.zeros(nchan*nHistPerChannel*2, dtype=np.float32)

    for il in range(nlevels):
        l    = LI[il]
        for c in range(nchan):
            h,bins = get_histogram(l[:,:,c], resolution)
            n = len(h)
            rng[2*c+2*nchan*il]   = bins[0]
            rng[2*c+2*nchan*il+1] = bins[-1]
            histograms[idx:idx+n] = h
            idx += n

    # histogram of RGB values
    il = nlevels
    for c in range(nchan):
        h,bins = get_histogram(I[:,:,c], resolution)
        n = len(h)
        rng[2*c+2*nchan*il]   = bins[0]
        rng[2*c+2*nchan*il+1] = bins[-1]
        histograms[idx:idx+n] = h
        idx += n

    return histograms,rng


def extract_histogram(histograms, rng, lvl, chan):
    """ Extract an histogram from the packed data """

    resolution = histograms[0]
    nlevels    = histograms[2]
    nchan      = histograms[2]
    n          = resolution -1

    nHistPerChannel = nlevels + 1
    idx = 3 + lvl*(nchan*((resolution-1))) + chan*((resolution-1))

    mini = rng[2*chan+2*lvl*nchan]
    maxi = rng[2*chan+2*lvl*nchan+1]
    h    = histograms[idx:idx+n]
    bins = np.linspace(mini,maxi,endpoint = True, num = resolution)

    return (h,bins)


def get_histogram(array, resolution = 256, mini = None, maxi = None):
    """ Computes a 1D histogram of the given array """

    array = np.ravel(array)
    if mini is None:
        mini = np.amin(array)
    if maxi is None:
        maxi = np.amax(array)
    bins = np.linspace(mini,maxi,endpoint = True, num = resolution)
    h = np.histogram(array, bins = bins)[0]
    return (h,bins)


def get_transfer_function(source, h2,bins2, res1 = 256, res2 = 256*10):
    """ Computes a 1D histogram remapping function """

    h1,bins1 = get_histogram(source, res1)
    m1 = bins1[0]
    M1 = bins1[-1]
    m2 = bins2[0]
    M2 = bins2[-1]

    h1 = h1.astype(np.float32)
    h2 = h2.astype(np.float32)

    cdf1 = np.cumsum(h1)*1.0/sum(h1)
    cdf2 = np.cumsum(h2)*1.0/sum(h2)


    # Resample in preparation of the inversion
    if res2-1 != len(cdf2):
        old_ticks = np.linspace(0,1,endpoint = True, num = len(cdf2))
        new_ticks = np.linspace(0,1,endpoint = True, num = res2)
        cdf2_i = interp1d(old_ticks, cdf2, kind="linear")
        cdf2_i = cdf2_i(new_ticks)
    else:
        cdf2_i = cdf2

    cdf2_i[-1] = 1
    cdf1[-1] = 1

    f = np.zeros((len(cdf1)-1,))
    for i in range(len(f)):
        found = np.where(cdf2_i >= cdf1[i])
        if len(found[0]) == 0:
            print cdf1[i]
            print cdf2_i
        idx = found[0][0]
        f[i] = idx *1.0/ res2
    return f,m2,M2


def apply_transfer_function(source, f, m2, M2):
    """ Remap values """

    m = np.amin(source)
    M = np.amax(source)
    src = (source-m).astype(np.float64)
    if M != m:
        src /= (M-m)

    n = len(f)-1
    f = np.append(f, f[-1])
    idx  = (np.floor(n*src)).astype(int)
    x      = (n*src-idx);
    O      = ((1-x)*f[idx]+x*f[idx+1]);
    O *= (M2-m2)
    O += m2

    return O


def process(hist_ref,rng_ref, Id, transfer_color = True, nlevels = 3, deg_res = 256, ref_res = 256*2, hist_resample = 10, output_dir = None, I = None):

    Id = color.RGB_to_YCbCr(Id)
    Id = Id.astype(np.float32)

    O = transfer(Id,hist_ref,rng_ref, deg_res, ref_res, hist_resample, transfer_color, output_dir = output_dir)

    O[O<0]   = 0
    O[O>255] = 255
    O = color.YCbCr_to_RGB(O)
    O = O.astype(np.uint8)

    # # Debug
    # if output_dir is not None and I is not None:
    #     plot_histograms(I.astype(np.float32),Id.astype(np.float32),O, os.path.join(output_dir,"hist_colors.png"))
    #
    #     LI  = buildLaplacianPyramid(I.astype(np.float32),nLevels   = nlevels + 1)
    #     LId = buildLaplacianPyramid(Id.astype(np.float32), nLevels = nlevels + 1)
    #     LO  = buildLaplacianPyramid(O.astype(np.float32), nLevels = nlevels + 1)
    #
    #     for c in range(3):
    #         for il in range(nlevels):
    #             plot_histograms(LI[il][:,:,c],LId[il][:,:,c],LO[il][:,:,c], os.path.join(output_dir,"hist_lapl-%02d-%02d.png" % (il,c)))

    return O

def main(args):
    # nlevels       = int(np.log2(ds))
    nlevels = 3
    print "%d levels" % nlevels
    ref_res       = 256*10
    deg_res       = 256*10
    hist_resample = 1
    
    # Clean input
    I = np.array(Image.open(args.input))

    # Create downsampled and compressed image image
    Ismall = Image.fromarray(I).resize((I.shape[1]/args.downsampling,I.shape[0]/args.downsampling), Image.LANCZOS)
    Ismall = np.array(Ismall)
    basedir, basename = os.path.split(args.output)
    basename, ext = os.path.splitext(basename)
    Ismall, fsize = save_and_get_img(Ismall, os.path.join(basedir, basename+"_sent.jpg"), quality = args.quality, subsampling = 0)

    # upsample the jpeg back to original resolution
    Id = np.array(Image.fromarray(Ismall).resize((I.shape[1],I.shape[0]), Image.LANCZOS))

    # Get stats from input
    I = color.RGB_to_YCbCr(I)
    I = I.astype(np.float32)
    hist_ref,rng_ref = get_histograms(I,nlevels,ref_res)

    O = process(hist_ref,rng_ref, Id, transfer_color = True, nlevels = nlevels, deg_res = deg_res, ref_res = ref_res, hist_resample = hist_resample, I = I)

    Image.fromarray(O).save(args.output)



if __name__ == '__main__':
    parser = argparse.ArgumentParser()

    parser.add_argument("input")
    parser.add_argument("output")
    parser.add_argument("--downsampling", type=int, default = 8)
    parser.add_argument("--quality", type=int, default = 60)
    # parser.add_argument("--add-noise", dest='add_noise', action = 'store_true')
    parser.set_defaults(add_noise=False)

    args = parser.parse_args()

    main(args)
