# ----------------------------------------------------------------------------
# File:    input_compressor.py
# Author:  Michael Gharbi <gharbi@mit.edu>
# Created: 2015-02-11
# ----------------------------------------------------------------------------
#
# Interface to load images precomputed. Simulates the operation of the whole
# pipeline on a degraded input.
#
# ---------------------------------------------------------------------------#


import os,sys,re
import numpy as np
import json
from abc import ABCMeta, abstractmethod

from settings import *
from mghimproc import get_img
from pipeline_node import PipelineNode

class InputCompressor(PipelineNode):
    __metaclass__ = ABCMeta

    def getFilename(self,path,name,ext):
        if not os.path.exists(os.path.join(path,name+ext)):
            # Extension search
            for f in os.listdir(path):
                match = re.match(name+"\.(png|jpg|jpeg)", f)
                if match:
                    name = f
                    break
        else:
            name = name+ext
        return name


class JPEGinputCompressor(InputCompressor):
    """ Reads a precomputed input/output pair

        This simulates the scenario in which the clients send a downsize, jpeg
        degraded input that is processed by the server

    """
    def process(self,model):
        """ Replace I and O in the model by the degraded versions """

        self.i_quality = model.p.in_quality
        self.i_ratio   = model.p.in_downsampling
        self.sz        = model.I.shape[0:2]

        # Use original
        if self.i_quality == 100 and self.i_ratio == 1:
            print "      + using full quality image pair"
            return (model,)


        validI, I,nbytes = self.getCompressedImage(model.r.unprocessedPath(), getStat = True)
        validO, O = self.getCompressedImage(model.r.processedPath(), getStat = False)[0:2]

        if validI and validO:
            model.I             = I
            model._nbytes_input = nbytes
            model.O             = O
            s = "      + jpeg compressed input %d" % self.i_quality
            if self.p.add_noise:
                s += " with noise"
            print s
        else:
            print "      + couldn't find a compressed input/output pair, skipping."
            raise Exception

        return (model,)

    def getCompressedImage(self,path, getStat = True):
        """ Load a compressed image, according to our specified format"""

        path,name = os.path.split(path)
        name, ext = os.path.splitext(name)

        if self.i_ratio > 1:
            flag = "ds"
            noise = ""
            transfer = ""
            flag = "us"
            if self.p.add_noise:
                noise = "_noise"
            if self.p.transfer_multiscale:
                transfer = "_transfer"

            name += "_%s_%02d_jpeg_%02d%s%s" % (flag,self.i_ratio, self.i_quality,noise,transfer)
        print name

        stat_file = os.path.join(path,name+".json")
        name      = self.getFilename(path,name,ext)
        path      = os.path.join(path, name)

        if not os.path.exists(path):
            return False,None,None

        I,fsize = get_img(path)

        if getStat and os.path.exists(stat_file):
            f = open(stat_file, 'r')
            stat = json.loads(f.readline())
            fsize = stat['fsize']
            f.close()

        # Make sure grey images have 3 dimensions
        if len(I.shape) < 3:
            I.shape += (1,)

        return True,I,fsize
