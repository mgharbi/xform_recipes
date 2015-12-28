import cv2
import numpy as np

def getNlevels(I,minSize):
    sz = np.amin(I.shape[0:2]).astype(float)
    nLevels = (np.ceil(np.log2(sz/minSize))+1).astype(int)
    return nLevels

def buildGaussianPyramid(I, nLevels= -1, minSize = 16):
    if nLevels == -1:
        nLevels = getNlevels(I,minSize)

    pyramid = nLevels*[None]
    pyramid[0] = I
    for i in range(nLevels-1):
        pyramid[i+1] = cv2.pyrDown(pyramid[i])

    return pyramid

def buildUpsampledLaplacianPyramid(I, nLevels= -1, minSize = 16):
    if nLevels == -1:
        nLevels = getNlevels(I,minSize)

    pyramid = nLevels*[None]
    pyramid[0] = I
    if len(pyramid[0].shape) < 3:
        pyramid[0].shape += (1,)

    for i in range(nLevels-1):
        srcSz = pyramid[i].shape[0:2]
        newSz = tuple([a/2 for a in pyramid[i].shape[0:2]])
        newSz = (newSz[1],newSz[0])
        pyramid[i+1] = cv2.pyrDown(pyramid[i])
        if len(pyramid[i+1].shape) < 3:
            pyramid[i+1].shape += (1,)

    for i in range(nLevels-1):
        newSz = pyramid[i].shape[0:2]
        up = cv2.pyrUp(pyramid[i+1],dstsize=(newSz[1],newSz[0])).astype(np.float32)
        if len(up.shape) < 3:
            up.shape += (1,)
        pyramid[i] = pyramid[i].astype(np.float32) - up

    # Make a stack
    for lvl in range(0,nLevels-1):
        for i in range(nLevels-1,lvl,-1):
            newSz = pyramid[i-1].shape[0:2]
            up = cv2.pyrUp(pyramid[i],dstsize=(newSz[1],newSz[0]))
            if len(up.shape) < 3:
                up.shape += (1,)
            pyramid[i] = np.array(up)

    return pyramid


def buildLaplacianPyramid(I, nLevels= -1, minSize = 16, useStack = False):
    if nLevels == -1:
        nLevels = getNlevels(I,minSize)

    pyramid = nLevels*[None]
    pyramid[0] = I
    if len(pyramid[0].shape) < 3:
        pyramid[0].shape += (1,)
    # All levels have the same resolution
    if useStack:
        # Gaussian pyramid
        for i in range(nLevels-1):
            srcSz = pyramid[i].shape[0:2]
            newSz = tuple([a/2 for a in pyramid[i].shape[0:2]])
            newSz = (newSz[1],newSz[0])
            pyramid[i+1] = cv2.pyrDown(pyramid[i])
            if len(pyramid[i+1].shape) < 3:
                pyramid[i+1].shape += (1,)

        # Make a stack
        for lvl in range(0,nLevels-1):
            for i in range(nLevels-1,lvl,-1):
                newSz = pyramid[i-1].shape[0:2]
                up = cv2.pyrUp(pyramid[i],dstsize=(newSz[1],newSz[0]))
                if len(up.shape) < 3:
                    up.shape += (1,)
                pyramid[i] = np.array(up)

        lapl = nLevels*[None]
        lapl[nLevels-1] = np.copy(pyramid[nLevels-1])
        for i in range(0,nLevels-1):
            lapl[i] = pyramid[i].astype(np.float32) - pyramid[i+1].astype(np.float32)
        pyramid = lapl

    else:
        for i in range(nLevels-1):
            srcSz = pyramid[i].shape[0:2]
            newSz = tuple([a/2 for a in pyramid[i].shape[0:2]])
            newSz = (newSz[1],newSz[0])
            pyramid[i+1] = cv2.pyrDown(pyramid[i])
            if len(pyramid[i+1].shape) < 3:
                pyramid[i+1].shape += (1,)

        for i in range(nLevels-1):
            newSz = pyramid[i].shape[0:2]
            up = cv2.pyrUp(pyramid[i+1],dstsize=(newSz[1],newSz[0])).astype(np.float32)
            if len(up.shape) < 3:
                up.shape += (1,)
            pyramid[i] = pyramid[i].astype(np.float32) - up



    return pyramid

def reconstructFromLaplacianPyramid(pyramid):
    nLevels = len(pyramid)
    out = pyramid[-1]
    if len(pyramid) == 1:
        return out

    useStack = False
    if pyramid[0].shape[0:2] == pyramid[-1].shape[0:2]:
        useStack = True

    dtp = out.dtype
    for i in range(nLevels-2,-1,-1):
        newSz = pyramid[i].shape[0:2]
        if useStack:
            up = out
        else:
            up = cv2.pyrUp(out,dstsize=(newSz[1],newSz[0]))
        if len(up.shape) < 3:
            up.shape += (1,)
        out =  up + pyramid[i]
        out = out.astype(dtp)

    return out
