# ----------------------------------------------------------------------------
# File:    postprocessor.py
# Author:  Michael Gharbi <gharbi@mit.edu>
# Created: 2015-02-12
# ----------------------------------------------------------------------------
#
#
#
# ---------------------------------------------------------------------------#


from abc import ABCMeta, abstractmethod

import numpy as np
from scipy.misc import imread, imsave
from mghimproc import buildLaplacianPyramid, reconstructFromLaplacianPyramid
from mghimproc.color import *
from settings import *

import cv2
import skimage
from PIL import Image

from pipeline_node import PipelineNode

class Postprocessor(PipelineNode):
    __metaclass__ = ABCMeta

class YCbCr2RGB(Postprocessor):
    def process(self,model):
        if len(model.I.shape)>2 and model.I.shape[2] > 1:
            model.I = YCbCr_to_RGB(model.I)
            model.Iref = YCbCr_to_RGB(model.Iref)
        if len(model.R.shape)>2 and model.R.shape[2] > 1:
            model.R = YCbCr_to_RGB(model.R)

        if len(model.O.shape)>2 and model.O.shape[2] > 1:
            model.O = YCbCr_to_RGB(model.O)
        # if len(model.O.shape)>2 and model.O.shape[2] > 1:
        #     model.lowpassO = YCbCr_to_RGB(model.lowpassO)
        return (model,)

class ClampToUint8(Postprocessor):
    def process(self, model):
        model.R = float2uint8(model.R)
        return (model,)
