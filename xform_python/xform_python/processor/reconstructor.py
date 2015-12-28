# ----------------------------------------------------------------------------
# File:    reconstructor.py
# Author:  Michael Gharbi <gharbi@mit.edu>
# Created: 2015-02-12
# ----------------------------------------------------------------------------
#
#
#
# ---------------------------------------------------------------------------#


from pipeline_node import PipelineNode
from mghimproc import DifferenceImage
from mghimproc import float2uint8
from mghimproc import image_metrics as metrics
import numpy as np

class Reconstructor(PipelineNode):
    def process(self,model):
        model.reconstruct()
        return (model,)
