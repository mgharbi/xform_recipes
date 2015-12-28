import numpy as np
import tempfile,os,sys,shutil
from scipy.misc import imread, imsave
from settings import *
from PIL import Image, ImageFilter

class DifferenceImage(object):
    def __init__(self,I1,I2,threshold=-1):
        # self.centeredDiff(I1,I2,threshold)
        self.other(I1,I2)

    def other(self,I1,I2):
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

    def centeredDiff(self,I1,I2,threshold):
        R = np.float32(I1) - np.float32(I2)
        if threshold>0:
            R[np.abs(R)<threshold] = 0
        self.min = np.amin(R)
        self.max = np.amax(R)
        self.rng = max(abs(self.min),abs(self.max))
        rng = self.rng
        R += self.rng
        if rng != 0:
            R /= 2*rng
        R *= 255
        R = np.uint8(np.clip(np.round(R),0,255))
        self.im = R
        self.nbytes = 0
        self.file_size = 0

    def getDifference(self):
        out = (self.max-self.min)*np.float32(self.im)/255+self.min
        # out = (2*self.rng)*(np.float32(self.im)/255)-self.rng
        return out

    def jpegCompress(self, quality = 50,separate_channels = False):
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

            im, self.file_size = jpegCompressImage(im,quality)

            if len(sz) == 3 and sz[2]>1:
                im2 = np.zeros((sz[0],sz[1],sz[2]))
                for c in range(sz[2]):
                    im2[:,:,c] =  im[:,c*sz[1]:(c+1)*sz[1]]
                im = im2
            self.im = im
        else:
            self.im, self.file_size = jpegCompressImage(self.im,quality)
        
def _jpegCompressImage(I,quality=50):
    d = tempfile.mkdtemp()
    fname = os.path.join(d,"compressed_image.jpg")
    Image.fromarray(np.uint8(np.squeeze(I))).save(fname,quality=quality, quality_layers = [32])
    filesize = os.stat(fname).st_size
    im_file = Image.open(fname)
    im = np.array(im_file)
    shutil.rmtree(d)
    return im,filesize

def jpegCompressImage(I,quality=50,separate_channels = False):
    newI = np.zeros(I.shape)
    if separate_channels:
        file_size = 0
        for c in range(I.shape[2]):
            ii = I[:,:,c]
            ii,sz = _jpegCompressImage(ii,quality)
            newI[:,:,c] = ii
            file_size += sz
    else:
        newI, file_size = _jpegCompressImage(I,quality)
    return newI, file_size

