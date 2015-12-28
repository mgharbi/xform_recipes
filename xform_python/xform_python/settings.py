# ----------------------------------------------------------------------------
# File:    settings.py
# Author:  Michael Gharbi <gharbi@mit.edu>
# Created: 2015-02-11
# ----------------------------------------------------------------------------
#
# Paths setup.
#
# ---------------------------------------------------------------------------#

import os.path as path

BASE_DIR            = path.split(path.dirname(path.abspath(__file__)))[0]
OUTPUT_DIR          = path.join(BASE_DIR,"output")
DEBUG_DIR           = path.join(OUTPUT_DIR,"_debug",)
DATA_DIR            = path.join(BASE_DIR,"data")
ALGO_DIR            = path.join(BASE_DIR,"algorithms")
CONFIG_DIR          = path.join(BASE_DIR,"config")
DEFAULT_CONFIG_FILE = path.join(CONFIG_DIR,"default.cfg")
