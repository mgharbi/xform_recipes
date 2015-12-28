from distutils.core import setup,Extension

import numpy
import os

try:
    numpy_include = numpy.get_include()
except AttributeError:
    numpy_include = numpy.get_numpy_include()

# print numpy_include

if os.getenv("DJANGO_ENV") == "PROD":
    cargs = [
        "-std=c99",
        "-static",
        "-O3"
    ]
else:
    cargs = []
cargs += [
    "-I",
    "/System/Library/Frameworks/vecLib.framework/Headers",
    "-I",
    "/usr/local",
]

mghimproc = Extension(
    '_mghimproc',
    sources=['mghimproc.c','mghimproc.i'],
    include_dirs = [numpy_include],
    extra_compile_args = cargs,
)

setup(
        name = 'Image processing routines',
        description = '',
        author = '',
        version = '0.1',
        ext_modules = [mghimproc],
    )
