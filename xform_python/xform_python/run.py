#!/usr/bin/python

# ----------------------------------------------------------------------------
# File:    run.py
# Author:  Michael Gharbi <gharbi@mit.edu>
# Created: 2015-02-11
# ----------------------------------------------------------------------------
#
# Main entry point to the recipe algorithm.
#
# ---------------------------------------------------------------------------#

import sys
import os
import argparse

os.chdir(os.path.dirname(os.path.abspath(__file__)))

from settings import *
from processor.processor import Processor
from models import Result, Parameters

def main(args):
    """ Runs the pipeline on a given input/output pair and dataset.

        @params
          args.dataset: subfolder of data/
          args.name: name of the image to process, input and output are stored
          under data/args.dataset/args.name
    """

    # Setup params
    params    = Parameters()
    processor = Processor(params)

    # I/O paths
    outputDir = os.path.join(OUTPUT_DIR, args.dataset)
    inputDir  = os.path.join(DATA_DIR, args.dataset)

    # Result struct
    r            = Result()
    r.dataset    = args.dataset
    r.name       = args.name
    r.dataPath   = os.path.join(inputDir,args.name)
    r.outputPath = os.path.join(outputDir,args.name)
    r.error      = ""

    # Run
    r = processor.process(r)

    print "---------------------------"
    print "* Processed image %s/%s" % (args.dataset, args.name)
    print "  - time\t%.2f s." % r.computation_time
    print "  - PSNR:\t %5.2f dB"      % r.psnr
    print "  - input:\t %5.2f %%"     % (r.compression_up*100)
    print "  - output:\t %5.2f %%"    % (r.compression_down*100)

if __name__ == '__main__':
    parser = argparse.ArgumentParser("Run the algorithm on the selected image")
    parser.add_argument('dataset', help='dataset of the image to process')
    parser.add_argument('name', help='name of the image to process')
    args = parser.parse_args()
    main(args)
