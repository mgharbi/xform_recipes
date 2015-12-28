# ----------------------------------------------------------------------------
# File:    preprocessor.py
# Author:  Michael Gharbi <gharbi@mit.edu>
# Created: 2015-02-11
# ----------------------------------------------------------------------------
#
# Various prepropcessing operations.
#
# ---------------------------------------------------------------------------#


from abc import ABCMeta, abstractmethod

import numpy as np
from mghimproc.color import *
from settings import *

from pipeline_node import PipelineNode

class Preprocessor(PipelineNode):
    __metaclass__ = ABCMeta

class RGB2YCbCr(Preprocessor):
    """ Convert I and O to YCbCr space"""

    def process(self,model):
        print "      - Processing in YCbCr"
        if len(model.I.shape)>2 and model.I.shape[2] > 1:
            model.I = RGB_to_YCbCr(model.I)
            model.Iref = RGB_to_YCbCr(model.Iref)
        if len(model.O.shape)>2 and model.O.shape[2] > 1:
            model.O = RGB_to_YCbCr(model.O)
        return (model,)
