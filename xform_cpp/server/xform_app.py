import logging
import os
import struct
import subprocess
import sys
import time
from io import BytesIO

import numpy as np
from PIL import Image

import flask

from mikamatbridge import Matlab

import xform_interface as xf

def create_app(debug  = False):
    application       = flask.Flask(__name__)
    application.debug = debug

    ROOT       = os.path.join(os.environ["WWW_ROOT"], 'xform.com/')
    DATA       = os.path.join(ROOT, 'data')
    OUTPUT     = os.path.join(ROOT,'output')
    LOGFILE    = os.path.join(os.environ["WWW_ROOT"], 'logs', 'xform_log')

    XFORM_ROOT = os.environ['XFORM_ROOT']
    ALGORITHMS = '/Users/mgharbi/Dropbox (MIT)/_research/xform/xform_algorithms/'

    # Filters directly implemetent in cpp
    filter_cpp = [
        "local_laplacian",
        "style_transfer",
        "colorization",
    ]

    # Filters implemented in matlab
    filter_matlab = [
        "time_of_day",
        "portrait_transfer",
    ]
    filters = filter_cpp + filter_matlab

    log_handler = logging.FileHandler(LOGFILE)
    log_handler.setLevel(logging.DEBUG)
    application.logger.addHandler(log_handler)

    @application.route('/')
    def root():
        return "<h1> Xform WSGI is online ! </h1>"

    @application.route('/data/<path:path>')
    def data_files(path):
        return flask.send_from_directory(DATA,path)

    # Precompute a noise map
    sigma = 3.0/255
    noise = sigma*np.random.randn(4000**2).astype(np.float32)
    noise_data = noise.tobytes()

    def parse_image_request(request):
        start   = time.time()
        data    = request.get_data(cache     = False)
        elapsed = (time.time() - start)*1000

        start       = time.time()
        nbytes = len(data)
        paramsIO = BytesIO(data)

        params = xf.XformParams()

        width            = struct.unpack(">I", paramsIO.read(4))[0]
        height           = struct.unpack(">I", paramsIO.read(4))[0]
        scaleFactor      = struct.unpack(">I", paramsIO.read(4))[0]
        params.width = width
        params.height = height
        params.upsampling_factor = scaleFactor

        # Read filter to use and parameters
        filterType = struct.unpack(">I", paramsIO.read(4))[0]
        params.filter_type = filters[filterType]

        # Read filter parameters
        if params.filter_type == "local_laplacian":
            levels        = struct.unpack(">I", paramsIO.read(4))[0]
            alpha         = struct.unpack(">f", paramsIO.read(4))[0]
            params.levels = levels
            params.alpha  = alpha
            application.logger.info("\nLocal laplacian: %s %s", levels, alpha)
        elif params.filter_type == "style_transfer":
            levels            = struct.unpack(">I", paramsIO.read(4))[0]
            iterations        = struct.unpack(">I", paramsIO.read(4))[0]
            params.levels     = levels
            params.iterations = levels
            application.logger.info("\nStyle Transfer: %s ", levels)
        elif params.filter_type == "colorization":
            application.logger.info("\nColorization: ")
        elif params.filter_type == "time_of_day":
            application.logger.info("\nTime of Day: ")
        elif params.filter_type == "portrait_transfer":
            application.logger.info("\nPortrait Transfer: ")
        else:
            application.logger.info("\nUnknown Filter")

        application.logger.info("  - download:\t\t %dms" % elapsed)
        application.logger.info("  - received:\t\t %.2fMB" % (nbytes/1048576.0) )

        # Read image
        imdata = data[paramsIO.tell():]

        elapsed     = (time.time() - start)*1000
        application.logger.info("  - read stream:\t %dms" % elapsed)

        return (imdata, params)

    def get_additional_data(params):
        data = ""
        if params.filter_type == "colorization":
            f = open(os.path.join(DATA, "scribbles.jpg"),'r')
            data = f.read()
            f.close()
        if params.filter_type == "style_transfer":
            f = open(os.path.join(DATA, "style_target.jpg"),'r')
            data = f.read()
            f.close()
        return data


    @application.route('/ping', methods=['GET','POST'])
    def ping():
        if flask.request.method == 'POST':
            start   = time.time()
            data    = flask.request.get_data(cache = False)
            nbytes  = len(data)
            elapsed = (time.time() - start)*1000
            application.logger.info("\nPing")
            application.logger.info("  - download:\t\t %dms" % elapsed)
            application.logger.info("  - received:\t\t %.2fkB" % (nbytes/1024.0) )
            return ""
        else:
            return "post some data here"


    def save_imdata(imdata, path):
        f = open(path,'w')
        f.write(imdata)
        f.close()


    def load_imdata(path):
        memFile = BytesIO()
        f = open(path,'r')
        data = f.read()
        f.close()
        return data


    @application.route('/naive_cloud', methods=['GET','POST'])
    def naive_cloud():
        if flask.request.method == 'POST':
            tot_start       = time.time()
            (imdata, params) = parse_image_request(flask.request)
            params.jpeg_quality = 85

            start   = time.time()
            if params.filter_type in filter_cpp:
                extradata = get_additional_data(params)
                outdata = xf.naive_processing(imdata,extradata,params)
            else:
                in_path = os.path.join(OUTPUT,"unprocessed.jpg")
                out_path = os.path.join(OUTPUT,"processed.jpg")

                application.logger.info("  - save input to %s" % in_path)
                save_imdata(imdata,in_path)

                if params.filter_type == 'portrait_transfer':
                    script = os.path.join(ALGORITHMS,"PortraitTransfer","xform.m")
                elif params.filter_type == 'time_of_day':
                    script = os.path.join(ALGORITHMS,"TimeOfDay","xform.m")

                start_m   = time.time()
                mlab = Matlab(socket_addr='ipc:///tmp/pymatbridge')
                mlab.connect()
                application.logger.info("  - matlab:\t %dms" % ((time.time() - start_m)*1000))
                application.logger.info("  - matlab script:\t %s" % script)

                res = mlab.run_func(script,{"input_path": in_path, "output_path": out_path})
                application.logger.info("  - matlab ret:\t %s" % res)

                outdata = load_imdata(out_path)
            elapsed = (time.time() - start)*1000
            application.logger.info("  - process:\t\t %dms" % elapsed)

            # Pack HTTP response
            start    = time.time()
            response = flask.make_response(outdata)
            elapsed  = (time.time() - start)*1000
            application.logger.info("  - Make response:\t %dms" % elapsed)
            application.logger.info("  - sending back:\t %.2fMB" % (len(outdata)/1048576.0) )

            tot_elapsed     = (time.time() - tot_start)*1000
            application.logger.info("  - Total server time:\t %dms" % tot_elapsed)

            return response
        else:
            return "<h1> This is the url for naive cloud </h1>"


    @application.route('/recipe_cloud', methods=['GET','POST'])
    def recipe_cloud():
        if flask.request.method == 'POST':
            tot_start       = time.time()
            (imdata, params) = parse_image_request(flask.request)
            params.jpeg_quality = 85

            start   = time.time()
            if params.filter_type in filter_cpp:
                # Cpp-implemented filters are integrated with the recipe pipeline
                # (save some io conversions)
                extradata = get_additional_data(params)
                outdata = xf.recipe_processing(imdata, extradata, noise_data, params)
            else:
                # We save files to disk for the matlab implementations
                in_path = os.path.join(OUTPUT,"unprocessed.jpg")
                out_path = os.path.join(OUTPUT,"processed.jpg")

                # Preprocess input
                application.logger.info("  - preprocess and save input to %s" % in_path)
                imdata = xf.input_preprocessing(imdata,noise_data, params)
                save_imdata(imdata,in_path)

                if params.filter_type == 'portrait_transfer':
                    script = os.path.join(ALGORITHMS,"PortraitTransfer","xform.m")
                elif params.filter_type == 'time_of_day':
                    script = os.path.join(ALGORITHMS,"TimeOfDay","xform.m")

                start_m   = time.time()
                mlab = Matlab(socket_addr='ipc:///tmp/pymatbridge')
                mlab.connect()
                application.logger.info("  - matlab:\t %dms" % ((time.time() - start_m)*1000))
                application.logger.info("  - matlab script:\t %s" % script)

                res = mlab.run_func(script,{"input_path": in_path, "output_path": out_path})
                application.logger.info("  - matlab ret:\t %s" % res)
                outdata = load_imdata(out_path)

                # Fit recipe
                application.logger.info("  - fit recipe")
                outdata = xf.fit_recipe(imdata,outdata,params)

            elapsed = (time.time() - start)*1000
            application.logger.info("  - process:\t\t %dms" % elapsed)

            response = flask.make_response(outdata)
            application.logger.info("  - sending back:\t %.2fMB" % (len(outdata)/1048576.0) )

            tot_elapsed     = (time.time() - tot_start)*1000
            application.logger.info("  - Total server time:\t %dms" % tot_elapsed)

            return response
        else:
            return "cloud recipe page"

    return application
