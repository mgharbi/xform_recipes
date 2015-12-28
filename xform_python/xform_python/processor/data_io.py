# ----------------------------------------------------------------------------
# File:    data_io.py
# Author:  Michael Gharbi <gharbi@mit.edu>
# Created: 2015-02-11
# ----------------------------------------------------------------------------
#
# Load input images, and export reconstructed image
#
# ---------------------------------------------------------------------------#

import sys

import os, re, sys
from settings import *
from scipy.misc import imread, imsave
import numpy as np
from pipeline_node import PipelineNode
from PIL import Image
from io import BytesIO

from transform_model import get_transform_model
from PIL import Image

class ImageInitializer(PipelineNode):
    def process(self,r):
        xform_mdl = get_transform_model(self.p)
        I,isize   = self.load(r.unprocessedPath())
        O,osize   = self.load(r.processedPath())
        self.validate(I,O)

        model               = xform_mdl(I,O,self.p)
        model.r             = r

        # Store the data size of the input, to measure compression later
        model._nbytes_input = isize
        return (model,)


    def validate(self,I,O):
        if I.shape[0] != O.shape[0] or I.shape[1] != O.shape[1]:
            raise IOError(0, "I and O shape dont match")


    def load(self,path):
        """Load image, trying other extensions if not found"""

        path, name = os.path.split(path)
        name, ext  = os.path.splitext(name)

        # Try other extensions if file not found
        if not os.path.exists(os.path.join(path,name+ext)):
            # Extension search
            for f in os.listdir(path):
                match = re.match(name+"\.(png|jpg|jpeg)", f)
                if match:
                    name = f
                    break
        else:
            name = name+ext

        path    = os.path.join(path, name)
        im_file = Image.open(path)
        I       = np.array(im_file)
        fsize   = os.stat(path).st_size

        # Make sure grey images have 3 dimensions
        if len(I.shape) < 3:
            I.shape += (1,)

        return I,fsize


class ImageExporter(PipelineNode):
    def process(self,model):
        if not os.path.exists(model.r.outputPath):
            os.makedirs(model.r.outputPath)
        self.save(model.R, model.r.reconstructedPath())
        return (model,)


    def save(self,R, path):
        d = os.path.dirname(path)
        if not os.path.exists(d):
           os.makedirs(d)

        sz = R.shape

        # Collapse singleton dimension (for grey images)
        R = np.squeeze(R)

        imsave(path,R)
