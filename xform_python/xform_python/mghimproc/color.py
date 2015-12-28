import numpy as np
from . import float2uint8

def RGB_to_YCbCr(im):
    im = im.astype(np.float32)
    YCbCr = np.zeros(im.shape)
    YCbCr[:,:,0] = 0.299*im[:,:,0] + 0.587*im[:,:,1] + 0.114*im[:,:,2]
    YCbCr[:,:,1] = 128 - 0.168736*im[:,:,0] - 0.331264*im[:,:,1] + 0.5*im[:,:,2]
    YCbCr[:,:,2] = 128 + 0.5*im[:,:,0] - 0.418688*im[:,:,1] - 0.081312*im[:,:,2]

    YCbCr = float2uint8(YCbCr)
    return YCbCr

def YCbCr_to_RGB(im):
    im = im.astype(np.float32)
    RGB = np.zeros(im.shape)
    RGB[:,:,0] = im[:,:,0] + 1.402*(im[:,:,2]-128)
    RGB[:,:,1] = im[:,:,0] - 0.34414*(im[:,:,1]-128) - 0.71414*(im[:,:,2]-128)
    RGB[:,:,2] = im[:,:,0] + 1.772*(im[:,:,1]-128)

    RGB = float2uint8(RGB)

    return RGB
