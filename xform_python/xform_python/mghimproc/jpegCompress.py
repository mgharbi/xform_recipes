import numpy as np
import tempfile,os,sys,shutil
from scipy.misc import imread, imsave
from settings import *
from PIL import Image, ImageFilter

from imsave import save_and_get_img

class DifferenceImage(object):
    def __init__(self,I1,I2):
        R = np.squeeze(np.float32(I1)) - np.squeeze(np.float32(I2))
        self.min = np.amin(R)
        self.max = np.amax(R)
        R -= self.min
        rng = (self.max-self.min)
        if rng != 0:
            R /= rng
        R *= 255
        R = np.uint8(np.clip(np.round(R),0,255))
        self.im = R
        self.nbytes = 0
        self.file_size = 0

    def getDifference(self):
        out = (self.max-self.min)*np.float32(self.im)/255+self.min
        return out

    def compress(self, quality = 50,separate_channels = False, ext =".jpg"):
        if separate_channels:
            im = self.im
            thresh = 5
            im[np.abs(im) < thresh] = 0
            sz = im.shape
            if len(sz) == 3 and sz[2]>1:
                im2 = np.zeros((sz[0],sz[1]*sz[2]))
                for c in range(sz[2]):
                    im2[:,c*sz[1]:(c+1)*sz[1]] = im[:,:,c]
                im = im2

            im, self.file_size = compressImage(im,quality, ext = ext)

            if len(sz) == 3 and sz[2]>1:
                im2 = np.zeros((sz[0],sz[1],sz[2]))
                for c in range(sz[2]):
                    im2[:,:,c] =  im[:,c*sz[1]:(c+1)*sz[1]]
                im = im2
            self.im = im
        else:
            self.im, self.file_size = compressImage(self.im,quality, ext = ext)
        
def _compressImage(I,quality=50, ext = ".jpg"):
    d = tempfile.mkdtemp()
    fname = os.path.join(d,"compressed_image"+ext)
    im, filesize = save_and_get_img(np.uint8(np.squeeze(I)),fname,quality)
    shutil.rmtree(d)
    return im,filesize

def compressImage(I,quality=50,separate_channels = False, ext = ".jpg"):
    newI = np.zeros(I.shape)
    if separate_channels:
        file_size = 0
        for c in range(I.shape[2]):
            ii = I[:,:,c]
            ii,sz = _compressImage(ii,quality, ext = ext)
            newI[:,:,c] = ii
            file_size += sz
    else:
        newI, file_size = _compressImage(I,quality, ext = ext)
    return newI, file_size

