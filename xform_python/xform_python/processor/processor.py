# ----------------------------------------------------------------------------
# File:    processor.py
# Author:  Michael Gharbi <gharbi@mit.edu>
# Created: 2015-02-11
# ----------------------------------------------------------------------------
#
#
#
# ---------------------------------------------------------------------------#


from datetime import datetime
import sys
import json

from settings import *

# Pipeline nodes
from data_io            import *
from compressor         import *
from input_compressor   import *
from encoder            import *
from error_metric       import *
from preprocessor       import *
from postprocessor      import *
from reconstructor      import *
from transform_model    import *


class Processor(object):
    """ Implements the complete compression scheme for client and server """

    def process(self,r):
        """ Run the pipeline and update the database record
        Args:
            r (Result): struct in which the results are to be saved
        """

        # Pass the result struct along
        model = (r,)

        # Run the complete pipeline sequentially
        start = datetime.now()
        for node in self.pipeline:
            model = node.process(*model)
        elapsed = (datetime.now())-start
        model = model[0]

        # Update the database record
        r                  = ImageErrorMetric.evaluate(model.Oref,model.R,r)
        r.computation_time = elapsed.total_seconds()
        r.height           = model.I.shape[0]
        r.width            = model.I.shape[1]
        r.compression_down = model.compression_ratio()
        r.compression_up   = model.compression_ratio_input()

        return r


    def __init__(self,p):
        """Initialize the processing pipeline from parameters given in JSON format.
        Args:
            r (Parameters): algorithm parameters
        """

        self.p         = p
        pipeline       = json.loads(self.p.pipeline)
        self.pipeline  = []
        current_module = sys.modules[__name__]
        for k in pipeline:
            att = getattr(current_module,k)()
            if hasattr(att,"setParams"):
                att.setParams(p)
            self.pipeline.append(att)
