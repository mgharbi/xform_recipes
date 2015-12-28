Transform Recipes for Efficient Cloud Photo Enhancement (2015)
==============================================================

- Michael Gharbi          <gharbi@mit.edu>,
- Yichang Shih            <yichang@mit.edu>,
- Gaurav Chaurasia        <gchauras@mit.edu>,
- Sylvain Paris           <sparis@adobe.com>,
- Jonathan Ragan-Kelley   <jrk@cs.stanford.edu>,
- Fredo Durand            <fredo@mit.edu>

Installation
------------

Run the convenience installation script:

```shell
    ./install.py
```

Alternatively, follow the steps below.
To install the required python packages and the versions used for development:

```shell
    pip install -r requirements.txt
```

Compile the cpp-extensions:

```shell
    cd xform_python/mghimproc && python setup.py build
```

You should be all set!


Usage
-----

You can run the example using the following command:

```shell
    python xform_python/run.py LocalLaplacian 0001
```

The input preprocessing part can be run as:

```shell
    python path/to/input.png output.png
```


Folder structure
----------------

`data/` contains the input data.

`output/` is where all the generated data as well as the sqlite database are stored.

`transform_compression/` holds the core of our algorithm.

The main algorithms for recipe fitting and reconstruction can be found in
`xform_python/processor/transform_model.py`.

The input pre-processing code is in
`xform_python/input_transfer.py`.

Data format
-----------

It is assumed in the code that the input data is stored in the `data/` folder .
For an image filter (also referred as category) *filt*, an input/output pair
*img*, there should be the following images:

    `data/dataset/img/unprocessed.ext`
    `data/dataset/img/processed.ext`

corresponding the unprocessed and processed images. *ext* is any image format
extension, we currently handle png and jpeg.

Citation
--------

If you use any part of this code, please cite our paper:

```
@article{Gharbi:2015:TRE:2816795.2818127,
    author = {Gharbi, Micha\"{e}l and Shih, YiChang and Chaurasia, Gaurav and Ragan-Kelley, Jonathan and Paris, Sylvain and Durand, Fr{\'e}do},
    title = {Transform Recipes for Efficient Cloud Photo Enhancement},
    journal = {ACM Trans. Graph.},
    issue_date = {November 2015},
    volume = {34},
    number = {6},
    month = oct,
    year = {2015},
    issn = {0730-0301},
    pages = {228:1--228:12},
    articleno = {228},
    numpages = {12},
    url = {http://doi.acm.org/10.1145/2816795.2818127},
    doi = {10.1145/2816795.2818127},
    acmid = {2818127},
    publisher = {ACM},
    address = {New York, NY, USA},
    keywords = {energy-efficient cloud computing, image filter approximation, mobile image processing},
}
```

