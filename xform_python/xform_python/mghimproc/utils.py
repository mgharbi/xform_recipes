import numpy as np

def float2uint8(R):
    return np.uint8(np.clip(np.round(R),0,255))
