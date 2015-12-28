import os
from PIL import Image
from io import BytesIO
import numpy as np
import sys

root_dir = os.path.split(os.path.abspath(os.path.dirname(__file__)))[0]
sys.path.append(os.path.join(root_dir,"lib"))
import pyrecipe as recipe

data_dir = os.path.join(root_dir,"data", "test_fixtures")
unprocessed = os.path.join(data_dir,"0001.jpg")
processed   = os.path.join(data_dir,"0001-processed.jpg")

f = open(unprocessed,'r')
unprocessed_data = f.read()
f.close()

f = open(processed,'r')
processed_data = f.read()
f.close()

params = recipe.XformParams()

recipe = recipe.fit_recipe(unprocessed_data, processed_data)

# params.filter_type       = "local_laplacian"
# params.levels            = 30
# params.alpha             = 10.0
# params.jpeg_quality      = 85
# params.upsampling_factor = 1
# pro       = os.path.join(dirname,"debug.jpg")

# us = 4
#
# if us == 1:
#     unp = os.path.join(dirname,"unprocessed.jpg")
# elif us ==2:
#     unp = os.path.join(dirname,"unprocessed_ds_2.jpg")
# elif us ==4:
#     unp = os.path.join(dirname,"unprocessed_ds_4.jpg")
# elif us ==8:
#     unp = os.path.join(dirname,"unprocessed_ds_8.jpg")

# sigma = 3.0/255
# noise = sigma*np.random.randn(4000**2).astype(np.float32)
# noise_data = noise.tobytes()


# outdata                  = recipe.debug_recipe(data,ref_data,data2,noise_data,params)
# outdata                  = recipe.debug_recipe(data,data2,noise_data,params)
# outdata                  = recipe.recipe_processing(data,data2,noise_data,params)

# f = open(pro,'w')
# f.write(outdata)
# f.close()


