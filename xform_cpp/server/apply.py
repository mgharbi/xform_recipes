# -------------------------------------------------------------------
# File:    create_degraded_input.py
# Author:  Michael Gharbi <gharbi@mit.edu>
# Created: 2014-11-05
# -------------------------------------------------------------------
#
# Generate degraded images for input compression.
#
# ------------------------------------------------------------------#

import os,sys
import re
import shutil
import argparse
import json
import subprocess
import numpy as np

import numpy as np

from PIL import Image

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "transform_compression"))
from settings import *
from mghimproc import imresize, save_and_get_img
import utils.filesystem as fs

from pymatbridge import Matlab

# For matlab ZMQ bridge
os.system("export LD_LIBRARY_PATH=/usr/local/lib")

mlabCats = [
    "DetailManipulation",
    "L0",
    "LocalLaplacian",
    "Matting",
    "PortraitTransfer",
    "Recoloring",
    "StyleTransfer",
    "TimeOfDay",
]

def compute_dehazing_airlight(catDir):
    for img in os.listdir(catDir):
        imDir = os.path.join(catDir,img)
        if not os.path.isdir(imDir):
            continue
        for f in os.listdir(imDir):
            match = re.match("unprocessed\..*", f)
            if not match:
                continue
            print f
            binary = os.path.join(ALGO_DIR,"Dehazing","bin","airlight")
            input_path  = os.path.join(imDir,f)
            out = os.path.dirname(input_path)
            out = os.path.join(out,"airlight.bin")
            if os.path.exists(out):
                continue

            subprocess.call([binary,input_path,out])

def main(category, mlab):
    dataDir = DATA_DIR


    for cat in os.listdir(dataDir):
        if category != "" and cat != category:
            continue
        catDir = os.path.join(dataDir,cat)
        if not os.path.isdir(catDir):
            continue
        print "Category %s" % category
        if category == "Dehazing":
            compute_dehazing_airlight(catDir)
        for img in os.listdir(catDir):
            imDir = os.path.join(catDir,img)
            if not os.path.isdir(imDir):
                continue
            for f in os.listdir(imDir):
                match = re.match("unprocessed.*", f)
                if not match:
                    continue
                match = re.match(".*.json", f)
                if match:
                    continue
                print "processing %s - %s" % (img,f)

                input_path  = os.path.join(imDir,f)
                output_path = os.path.join(imDir,f)
                output_path = output_path.replace("unprocessed", "processed")

                if os.path.exists(output_path):
                    continue

                if cat in mlabCats:
                    script = os.path.join(ALGO_DIR,cat,"xform.m")
                    res = mlab.run_func(script,{"input_path": input_path,
                        "output_path": output_path})
                elif cat == "Dehazing":
                    binary = os.path.join(ALGO_DIR,cat,"bin","dehaze")
                    airlight = os.path.dirname(input_path)
                    airlight = os.path.join(airlight,"airlight.bin")
                    subprocess.call([binary,input_path,output_path,airlight])
                else:
                    continue


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("category")
    args = parser.parse_args()
    print "Filtering %s" % args.category
    if args.category == 'all':
        mlab = Matlab()
        mlab.start()
        allCat = fs.listSubdir(DATA_DIR)
        try:
            for c in allCat:
                main(c, mlab)
        except Exception, e:
            raise e
        finally:
            mlab.stop()
    else:
        if args.category in mlabCats:
            mlab = Matlab()
            mlab.start()
            try:
                main(args.category, mlab)
            except Exception, e:
                raise e
            finally:
                mlab.stop()
        else:
            main(args.category, None)


