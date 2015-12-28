# ----------------------------------------------------------------------------
# File:    error_metric.py
# Author:  Michael Gharbi <gharbi@mit.edu>
# Created: 2015-02-12
# ----------------------------------------------------------------------------
#
#
#
# ---------------------------------------------------------------------------#


import mghimproc.image_metrics as metrics

class ImageErrorMetric(object):
    @staticmethod
    def evaluate(I1,I2,r):
        if hasattr(r,"psnr"):
            psnr, idx = metrics.computePSNR(I1,I2)
            r.max_error_x = idx[0]
            r.max_error_y = idx[1]
            setattr(r, "psnr", psnr )
        if hasattr(r,"ssim"):
            setattr(r, "ssim", metrics.compute_ssim(I1,I2))
        return r
