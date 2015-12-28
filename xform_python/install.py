import os
import subprocess

# Install dependencies
os.system("pip install -r requirements.txt")

# Compile C code
os.system("cd xform_python/mghimproc && python setup.py build")

