#!/usr/bin/env python
import os
import sys

from xform_app import create_app

def application(environ, start_response):
    # explicitly set environment variables from the WSGI-supplied ones
    ENVIRONMENT_VARIABLES = [
        'WWW_ROOT',
        'XFORM_ROOT',
    ]
    for key in ENVIRONMENT_VARIABLES:
        os.environ[key] = environ.get(key)

    app = create_app(debug = False)

    return app(environ, start_response)
