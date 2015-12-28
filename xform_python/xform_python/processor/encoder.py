# ----------------------------------------------------------------------------
# File:    encoder.py
# Author:  Michael Gharbi <gharbi@mit.edu>
# Created: 2015-02-12
# ----------------------------------------------------------------------------
#
#
#
# ---------------------------------------------------------------------------#


import numpy as np
from abc import ABCMeta, abstractmethod
from sklearn import cluster
from sklearn import preprocessing


from pipeline_node import PipelineNode

class Encoder(PipelineNode):
    __metaclass__ = ABCMeta

class Decoder(PipelineNode):
    __metaclass__ = ABCMeta

class UniformEncoder(Encoder):
    def process(self,model):
        rcp   = model.recipe_hp
        sz    = rcp.shape[0:2]
        nChan = rcp.shape[2]
        mins  = nChan*[None]
        maxs  = nChan*[None]

        nbins = 2**8-1
        result = np.zeros(rcp.shape, dtype = np.uint8)
        model._nbytes -= rcp.nbytes
        for c in range(nChan):
            vals    = rcp[:,:,c].astype(np.float32)
            mins[c] = np.amin(vals)
            maxs[c] = np.amax(vals)
            rng = maxs[c] - mins[c]
            if rng <= 0:
                rng = 1

            vals -= mins[c]
            vals /= rng
            vals *= nbins
            vals += 0.5

            result[:,:,c] = vals
        model.recipe_hp = result
        model.enc_mins = mins
        model.enc_maxs = maxs
        model._nbytes += model.recipe_hp.nbytes

        return (model,)

class UniformDecoder(Decoder):
    def process(self,model):
        rcp     = model.recipe_hp
        sz    = rcp.shape[0:2]
        nChan = rcp.shape[2]
        mins  = model.enc_mins
        maxs  = model.enc_maxs

        nbins = 2**8-1
        result = np.zeros(rcp.shape, dtype = np.float32)
        for c in range(nChan):
            vals    = rcp[:,:,c].astype(np.float32)
            rng = maxs[c] - mins[c]
            if rng <= 0:
                rng = 1

            vals /= nbins
            vals *= rng
            vals += mins[c]
            result[:,:,c] = vals
        model.recipe_hp = result
        del model.enc_mins
        del model.enc_maxs

        return (model,)
