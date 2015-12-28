# ----------------------------------------------------------------------------
# File:    transform_model.py
# Author:  Michael Gharbi <gharbi@mit.edu>
# Created: 2015-02-11
# ----------------------------------------------------------------------------
#
# Main algorithms for recipe fitting and reconstruction
#
# ---------------------------------------------------------------------------#


from abc import ABCMeta, abstractmethod
import sys
import time

import numpy as np
from sklearn.linear_model import Lasso
from scipy.ndimage.filters import (
    correlate,
    gaussian_filter
)

from pipeline_node import PipelineNode
from mghimproc import imresize
from mghimproc import compressImage, DifferenceImage
from mghimproc import float2uint8

from mghimproc.pyramid import (
    buildLaplacianPyramid,
    buildUpsampledLaplacianPyramid,
    reconstructFromLaplacianPyramid
)

class Model(object):
    """ This is the central object that gets transformed throughout the pipeline

        The model class holds, the parameters, input, output and reconstructed
        images as well as any temporary additional parameters that might be
        added by the pipeline nodes.

        Attributes:
            Iref (np.array): unaltered input image the user want to process
            Oref (np.array): result of processing `Iref` with the filter
            I (np.array): degraded input image sent by the user
            O (np.array): result of processing `I` with the filter
            model (Parameters): coefficients of the model
            R (np.array): result of our reconstruction using `Iref` and model.
            approximates `Oref`
            p (Parameters): algorithm parameters

    """
    __metaclass__ = ABCMeta

    def __init__(self,I,O,p):
        self.I = I
        self.O = O

        self.Iref = self.I
        self.Oref = self.O

        self.p          = p

    @abstractmethod
    def reconstruct(self,I):
        pass

    def compression_ratio(self):
        s = self.Oref.shape
        if len(s) == 2 or s[2] == 1:
            factor = 3
        else:
            factor = 1
        return float(self.nbytes())/(factor*(self.Oref.astype(np.uint8)).nbytes)

    def compression_ratio_input(self):
        return float(self.nbytes_input())/(self.Iref.astype(np.uint8)).nbytes

    def nbytes(self):
        return self._nbytes

    def nbytes_input(self):
        return self._nbytes_input


class RecipeModel(Model):
    """ This is the central object that gets transformed throughout the pipeline

    Attributes:
        step (int): stepping size between two (potentially overlapping) neighbor patches
        rcp_channels (list of int): number of recipe channels for each output channel
        recipe_hp_shape (list of int): shape of the highpass recipe array
        n_ms_levels (int): number of multiscale feature levels
        recipe_lp (np.array): lowpass coefficients of the recipe
        recipe_hp (np.array): highpass coefficients of the recipe
        _nbytes (int): current data size in bytes

    """
    def get_model(self):
        """ Fetch the regression model to use """
        return Lasso(alpha = self.p.epsilon, fit_intercept = True, precompute = True,  max_iter = 1e4)

    def set_recipe_shape(self):
        """ Setup the number of recipes channel for the given parameters"""

        if self.p.use_patch_overlap:
            self.step = self.p.wSize/2
        else:
            self.step = self.p.wSize

        # Linear terms + affine offset
        recipe_channels  = self.I.shape[2] + 1

        # Additional luma channels
        add_luma_channels  = 0
        if self.p.use_multiscale_feat:
            if self.p.ms_levels < 0:
                self.n_ms_levels = (np.log2(self.p.wSize)-1).astype(int)
            else:
                self.n_ms_levels = self.p.ms_levels
            add_luma_channels += self.n_ms_levels
        if self.p.use_tonecurve_feat:
            add_luma_channels += self.p.luma_bands-1

        sz = self.O.shape
        recipe_h   = int(np.ceil((1.0*sz[0])/(self.step)))
        recipe_w   = int(np.ceil((1.0*sz[1])/(self.step)))
        recipe_d   = sz[2]*recipe_channels + add_luma_channels

        # Number of regression channels per output channels
        self.rcp_channels      = sz[2]*[recipe_channels]
        self.rcp_channels[0]  += add_luma_channels

        # High pass parameters
        self.recipe_hp_shape  = [recipe_h, recipe_w, recipe_d]


    def get_lowpass_image(self, I):
        """ Downsample image """

        lp_ratio   = self.p.wSize
        lp_sz = [s/lp_ratio for s in I.shape[0:2]]
        lowpassI   = imresize(I,lp_sz)
        return lowpassI


    def get_lowpass_recipe(self, lowpassI, lowpassO):
        """ Computes the low frequency part of the recipe """
        if lowpassO.shape[2] == 1:
            lI = lowpassI[:,:,0]
            lI.shape += (1,)
            lO = (lowpassO+1)/(lI+1)
        else:
            lO = (lowpassO+1)/(lowpassI+1)
        lowpassO = lO
        lowpassO = lowpassO.astype(np.float16)

        return lowpassO


    def reconstruct_lowpass_residual(self, lowpassI):
        """ Reconstruct lowpass part of the image from recipe """

        lowpassO = self.recipe_lp.astype(np.float64)
        if lowpassO.shape[2] == 1:
            lI = lowpassI[:,:,0]
            lI.shape += (1,)
            lowpassO = lowpassO * (lI+1) - 1
        else:
            lowpassO = lowpassO * (lowpassI+1) - 1
        lowpassO = imresize(lowpassO,self.Oref.shape[0:2])
        return lowpassO


    def get_multiscale_luminance(self,I):
        """ Build the maps for multiscale luminance features """
        if self.p.use_multiscale_feat:
            II = np.copy(I)[:,:,0]
            ms = np.zeros((I.shape[0],I.shape[1],self.n_ms_levels))
            L = buildLaplacianPyramid(II,nLevels = self.n_ms_levels+1, useStack = True)
            ms = np.zeros((I.shape[0],I.shape[1],self.n_ms_levels))
            L.pop() # Remove lowpass-residual
            for i,p in enumerate(L):
                ms[:,:,i] = p[:,:,0]
            return ms

        else:
            return None


    def get_patch_features(self, I, i_rng, j_rng):
        """ Reshape patch data to feature vectors """

        patch = I[i_rng[0]:i_rng[1], j_rng[0]:j_rng[1]]
        sz = patch.shape
        X  = np.reshape(patch,(sz[0]*sz[1],sz[2]))
        X  = np.float64(X)
        return X


    def extend_features(self, X, Xr, X_degraded = None, i_rng = None, j_rng = None,  ms_levels = None):
        """ Add features for luminance prediction

        Args:
            X (np.array): basic features from the highpass
            Xr (np.array): pixel values of the image
                (or degraded image during estimation)
            X_degraded (np.array): pixel values of the degraded image (to get the
                range estimate during reconstruction)
            i_rng (list of int): i coordinates of the patch
            j_rng (list of int): j coordinates of the patch
            ms_levels (np.array): pyramid maps for the multiscale features

        """

        if self.p.use_tonecurve_feat:
            luma_band_thresh = 5
            Xl = Xr[:,0]
            if X_degraded is not None:
                mini = np.amin(X_degraded[:,0])
                maxi = np.amax(X_degraded[:,0])
            else:
                mini = np.amin(Xl)
                maxi = np.amax(Xl)
            l_step = (maxi-mini)/self.p.luma_bands
            for il in range(1,self.p.luma_bands):
                bp = mini+il*l_step
                if l_step < luma_band_thresh:
                    Xl1 = np.zeros(Xl.shape)
                else:
                    Xl1 = (Xl>=bp).astype(np.float64)
                    Xl1 = Xl1*(Xl-bp)
                Xl1.shape += (1,)
                X = np.concatenate((X,Xl1), axis = 1)
        if ms_levels is not None and self.p.use_multiscale_feat:
            Xd = self.get_patch_features(ms_levels,i_rng, j_rng)
            X = np.concatenate((X,Xd),axis=1)
        return X


    def build(self):
        """ Build recipe from the degraded input/output pair"""

        I        = np.copy(self.I).astype(np.float64)
        O        = np.copy(self.O).astype(np.float64)
        wSize    = self.p.wSize

        print "      - wsize %d, %dx%d" % (wSize,I.shape[0],I.shape[1])

        self.set_recipe_shape()
        recipe_hp = np.zeros(self.recipe_hp_shape,dtype = np.float64)
        recipe_h  = recipe_hp.shape[0]
        recipe_w  = recipe_hp.shape[1]
        lowpassI  = self.get_lowpass_image(I);
        lowpassO  = self.get_lowpass_image(O);

        # Lowpass residual
        self.recipe_lp = self.get_lowpass_recipe(lowpassI, lowpassO)

        # Multiscale features
        ms_luma = self.get_multiscale_luminance(I)

        # High pass component
        lowpassI  = imresize(lowpassI,I.shape[0:2])
        lowpassO  = imresize(lowpassO,O.shape[0:2])
        highpassI = I - lowpassI
        highpassO = O - lowpassO

        idx = 0
        sys.stdout.write("\n")
        for imin in range(0,I.shape[0],self.step):
            for jmin in range(0,I.shape[1],self.step):
                idx += 1
                sys.stdout.write("\r      - Build: %.2f%%" % (100.0*idx/recipe_h/recipe_w))

                # Recipe indices
                patch_i = imin/self.step
                patch_j = jmin/self.step

                # Patch indices in the full-res image
                i_rng    = (imin, min(imin+wSize,I.shape[0]))
                j_rng    = (jmin, min(jmin+wSize,I.shape[1]))

                X  = self.get_patch_features(highpassI,i_rng,j_rng)
                Y  = self.get_patch_features(highpassO,i_rng,j_rng)
                Xr = self.get_patch_features(self.I,i_rng,j_rng)

                X = self.extend_features(X,Xr,i_rng = i_rng,j_rng = j_rng, ms_levels = ms_luma)

                rcp_chan = 0
                for chanO in range(O.shape[2]):
                    rcp_stride = self.rcp_channels[chanO]
                    reg        = self.get_model()
                    reg.fit(X[:, 0:rcp_stride-1],Y[:,chanO])
                    coefs = np.append(reg.coef_, reg.intercept_)

                    # Fill in the recipe
                    recipe_hp[patch_i, patch_j,
                              rcp_chan:rcp_chan+rcp_stride] = coefs

                    rcp_chan += rcp_stride
        sys.stdout.write("\n")

        # Compute data usage
        self.recipe_hp = recipe_hp
        self._nbytes   = self.recipe_hp.nbytes
        self._nbytes  += self.recipe_lp.nbytes


    def reconstruct(self):
        """ Reconstruct and approximation of the output image, from recipe"""

        Ideg = imresize(self.I, self.Iref.shape[0:2])

        # We reconstruct the output based on the reference input
        I         = np.copy(self.Iref).astype(np.float64)
        recipe_hp = self.recipe_hp
        wSize    = self.p.wSize

        recipe_h = recipe_hp.shape[0]
        recipe_w = recipe_hp.shape[1]

        # Construct four images, in case of overlap, which we'll later linearly
        # interpolate
        if self.p.use_patch_overlap:
            R = []
            for i in range(4):
                R.append(np.zeros((I.shape[0],I.shape[1],self.Oref.shape[2])))
        else:
            R = [np.zeros((I.shape[0],I.shape[1],self.Oref.shape[2]))]

        lowpassI = self.get_lowpass_image(I);

        # Reconstruct lowpass
        lowpassO = self.reconstruct_lowpass_residual(lowpassI)
        for i in range(len(R)):
            R[i] = np.copy(lowpassO)

        # Multiscale features
        ms_luma = self.get_multiscale_luminance(I)

        # High pass component
        lowpassI = imresize(lowpassI,I.shape[0:2])
        highpassI = I - lowpassI

        idx = 0
        sys.stdout.write("\n")
        for imin in range(0,I.shape[0],self.step):
            for jmin in range(0,I.shape[1],self.step):
                idx += 1
                sys.stdout.write("\r      - Reconstruct: %.2f%%" % (100.0*idx/recipe_h/recipe_w))

                # Recipe indices
                patch_i = imin/self.step
                patch_j = jmin/self.step

                # XXX: this is hacky
                patch_i = min(patch_i, recipe_hp.shape[0]-1)
                patch_j = min(patch_j, recipe_hp.shape[1]-1)

                if self.p.use_patch_overlap:
                    r_index = (patch_i % 2)*2 + (patch_j % 2)
                else:
                    r_index = 0

                # Patch indices in the full-res image
                i_rng = (imin, min(imin+wSize,I.shape[0]))
                j_rng = (jmin, min(jmin+wSize,I.shape[1]))

                X          = self.get_patch_features(highpassI,i_rng,j_rng)
                Xr         = self.get_patch_features(self.Iref,i_rng,j_rng)
                X_degraded = self.get_patch_features(Ideg, i_rng, j_rng)

                X = self.extend_features(X,Xr,X_degraded = X_degraded, i_rng = i_rng, j_rng = j_rng, ms_levels = ms_luma)

                rcp_chan = 0
                for chanO in range(self.Oref.shape[2]):
                    rcp_stride = self.rcp_channels[chanO]
                    reg        = self.get_model()
                    coefs = recipe_hp[patch_i, patch_j,
                              rcp_chan:rcp_chan+rcp_stride]

                    # hack because sklearn needs to be fitted to initialize coef_, intercept_
                    reg.fit(np.zeros((2,2)), np.zeros((2,)))

                    reg.coef_      = coefs[0:-1]
                    reg.intercept_ = coefs[-1]

                    recons         = reg.predict(X[:,0:rcp_stride-1])
                    R[r_index][i_rng[0]:i_rng[1], j_rng[0]:j_rng[1],chanO] += np.reshape(recons, (i_rng[1]-i_rng[0],j_rng[1]-j_rng[0]))
                    rcp_chan += rcp_stride
        sys.stdout.write("\n")

        if self.p.use_patch_overlap:
            self.R = self.linear_interpolate(R)
        else:
            self.R = R[0]

    def linear_interpolate(self,R):
        """ Linearly interpolate pixel values of overlapping patches"""

        res  = np.array(R[0])
        res2 = np.array(R[2])

        sz = R[0].shape
        s  = self.step

        x = np.linspace(0,1,s)
        x = np. reshape(x,(1,s))
        x = np.concatenate((x,np.fliplr(x)),axis=1)
        x = np.tile(x,(sz[0],sz[1]/(s*2)+1))
        x = x[:,0:sz[1]]
        x[:,0:s] = 1
        x.shape += (1,)

        res  = res*x + (1-x)*R[1]
        res2 = res2*x + (1-x)*R[3]

        x = np.linspace(0,1,s)
        x = np. reshape(x,(s,1))
        x = np.concatenate((x,np.flipud(x)),axis=0)
        x = np.tile(x,(sz[0]/(s*2)+1,sz[1]))
        x = x[0:sz[0],:]
        x[0:s,:] = 1
        x.shape += (1,)

        res = x*res + (1-x)*res2

        return res


def get_transform_model(p):
    """ Helper method to get the actual transform model class """

    current_module = sys.modules[__name__]
    xform_mdl      = getattr(current_module,p.transform_model)
    return xform_mdl


class ModelBuilder(PipelineNode):
    """ Pipeline node for the generation of the model """

    def process(self,model):
        model.build()
        current_module = sys.modules[__name__]
        return (model,)
