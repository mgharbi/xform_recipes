# ----------------------------------------------------------------------------
# File:    compressor.py
# Author:  Michael Gharbi <gharbi@mit.edu>
# Created: 2015-02-12
# ----------------------------------------------------------------------------
#
#
#
# ---------------------------------------------------------------------------#


import os,sys
from abc import ABCMeta, abstractmethod

import numpy as np
from PIL import Image

from mghimproc import save_and_get_img
from pipeline_node import PipelineNode
from settings import *


class Compressor(PipelineNode):
    __metaclass__ = ABCMeta

class Decompressor(PipelineNode):
    __metaclass__ = ABCMeta

class RecipeImageCompressor(Compressor):
    """ Saves the recipe coefficients as an image file.

    Leverages off-the-shelf lossless image compression.

    """

    def process(self,model):
        nbytes = model._nbytes
        d      = model.r.outputPath
        if not os.path.exists(d):
            os.makedirs(d)
        ext    = ".png"
        ext_lp = ext
        if model.recipe_lp.dtype == np.float16:
            ext_lp = ".tif"

        fname_lp = os.path.join(d,"compressed_model_lp"+ext_lp)
        fname_hp = os.path.join(d,"compressed_model_hp"+ext)

        # Save and load highpass as image, to leverage image compression
        recipe_hp        = model.recipe_hp
        hp_img           = self.mosaic(recipe_hp)
        hp_img, fSize_hp = save_and_get_img(hp_img,fname_hp)
        recipe_hp        = self.demosaic(hp_img,recipe_hp.shape).astype(model.recipe_hp.dtype)
        model.recipe_hp  = recipe_hp

        # Update recipe size
        model._nbytes = fSize_hp

        recipe_lp           = model.recipe_lp
        lp_img              = self.mosaic(recipe_lp)
        recipe_lp, fSize_lp = save_and_get_img(lp_img,fname_lp)
        recipe_lp           = self.demosaic(lp_img,model.recipe_lp.shape).astype(model.recipe_lp.dtype)
        model.recipe_lp     = recipe_lp
        model._nbytes      += fSize_lp

        compression  = 100.0*(1.0*model._nbytes/nbytes)
        print "      + %s compression: %.2f%%" % (ext[1:].upper(),compression)

        return (model,)

    def mosaic(self,im):
        sz = im.shape
        new_im = np.zeros((sz[0],sz[1]*sz[2]))
        for c in range(sz[2]):
            new_im[:,c*sz[1]:(c+1)*sz[1]] = im[:,:,c]
        return new_im

    def demosaic(self,im, sz):
        new_im = np.zeros(sz)
        for c in range(sz[2]):
            new_im[:,:,c] = im[:,c*sz[1]:(c+1)*sz[1]]
        return new_im
