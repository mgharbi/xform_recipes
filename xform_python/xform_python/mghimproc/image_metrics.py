# ----------------------------------------------------------------------------
# File:    image_metrics.py
# Author:  Michael Gharbi <gharbi@mit.edu>
# Created: 2015-03-09
# ----------------------------------------------------------------------------
#
#
#
# ---------------------------------------------------------------------------#


import numpy as np
from math import log10
from numpy.ma.core import exp
from skimage import color
import scipy.ndimage
from scipy.constants.constants import pi
from scipy.ndimage.filters import correlate, gaussian_filter
from PIL import Image

def computeMSE(I1,I2):
    I1 = np.squeeze(np.float32(I1))
    I2 = np.squeeze(np.float32(I2))
    err = np.square(I1-I2)
    if len(err.shape) > 2:
        err = np.mean(err,axis = 2)
    return err


def computePSNR(I1,I2):
    err = computeMSE(I1,I2)
    smooth_err = gaussian_filter(err,64)

    max_idx = np.argmax(smooth_err)
    max_idx = np.unravel_index(max_idx, smooth_err.shape)
    max_idx = (max_idx[1], max_idx[0])

    err = np.mean(err)
    if err>0:
        psnr = -10*log10(err/255**2)
    else:
        psnr = float("Inf")
    return (psnr,max_idx)

def compute_ssim(img_mat_1, img_mat_2):
    #Variables for Gaussian kernel definition
    img_mat_1 = np.squeeze(img_mat_1)
    img_mat_2 = np.squeeze(img_mat_2)
    gaussian_kernel_sigma=1.5
    gaussian_kernel_width=11
    if len(img_mat_1.shape) == 3:
        gaussian_kernel=np.zeros((gaussian_kernel_width,gaussian_kernel_width,1))
    else:
        gaussian_kernel=np.zeros((gaussian_kernel_width,gaussian_kernel_width))

    #Fill Gaussian kernel
    for i in range(gaussian_kernel_width):
        for j in range(gaussian_kernel_width):
            gaussian_kernel[i,j]=\
            (1/(2*pi*(gaussian_kernel_sigma**2)))*\
            exp(-(((i-5)**2)+((j-5)**2))/(2*(gaussian_kernel_sigma**2)))

    #Convert image matrices to double precision (like in the Matlab version)
    img_mat_1=img_mat_1.astype(np.float)
    img_mat_2=img_mat_2.astype(np.float)

    #Squares of input matrices
    img_mat_1_sq=img_mat_1**2
    img_mat_2_sq=img_mat_2**2
    img_mat_12=img_mat_1*img_mat_2

    #Means obtained by Gaussian filtering of inputs
    img_mat_mu_1=scipy.ndimage.filters.convolve(img_mat_1,gaussian_kernel)
    img_mat_mu_2=scipy.ndimage.filters.convolve(img_mat_2,gaussian_kernel)

    #Squares of means
    img_mat_mu_1_sq=img_mat_mu_1**2
    img_mat_mu_2_sq=img_mat_mu_2**2
    img_mat_mu_12=img_mat_mu_1*img_mat_mu_2

    #Variances obtained by Gaussian filtering of inputs' squares
    img_mat_sigma_1_sq=scipy.ndimage.filters.convolve(img_mat_1_sq,gaussian_kernel)
    img_mat_sigma_2_sq=scipy.ndimage.filters.convolve(img_mat_2_sq,gaussian_kernel)

    #Covariance
    img_mat_sigma_12=scipy.ndimage.filters.convolve(img_mat_12,gaussian_kernel)

    #Centered squares of variances
    img_mat_sigma_1_sq=img_mat_sigma_1_sq-img_mat_mu_1_sq
    img_mat_sigma_2_sq=img_mat_sigma_2_sq-img_mat_mu_2_sq
    img_mat_sigma_12=img_mat_sigma_12-img_mat_mu_12;

    #c1/c2 constants
    #First use: manual fitting
    c_1=6.5025
    c_2=58.5225

    #Second use: change k1,k2 & c1,c2 depend on L (width of color map)
    l=255
    k_1=0.01
    c_1=(k_1*l)**2
    k_2=0.03
    c_2=(k_2*l)**2

    #Numerator of SSIM
    num_ssim=(2*img_mat_mu_12+c_1)*(2*img_mat_sigma_12+c_2)
    #Denominator of SSIM
    den_ssim=(img_mat_mu_1_sq+img_mat_mu_2_sq+c_1)*\
    (img_mat_sigma_1_sq+img_mat_sigma_2_sq+c_2)
    #SSIM
    ssim_map=num_ssim/den_ssim
    index=np.average(ssim_map)

    return index

