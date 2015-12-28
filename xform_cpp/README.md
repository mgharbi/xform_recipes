Efficient Cloud Photo Enhancement using Compact Transform Recipes
=================================================================

Michael Gharbi          <gharbi@mit.edu>,
Yichang Shih            <yichang@mit.edu>,
Gaurav Chaurasia        <gchauras@mit.edu>,
Sylvain Paris           <sparis@adobe.com>,
Jonathan Ragan-Kelley   <jrk@cs.stanford.edu>,
Fredo Durand            <fredo@mit.edu>

This release is a working version of our Transform Recipes implementations. Results may differ from the paper.
In particular this release does not contain the Lasso regression on the server-side.

## Dependencies

The library requires the following packages, use your package manager and CMake's autodiscovery

* libjpeg
* libpng
* png++
* Armadillo matrix library
* Halide
* CMake
* gTest framework (put it in the third_party folder and compile in source)
* swig
* Android SDK (client-side)
* Android NDK (client-side)

The following dependencies are optional:
* OpenMP (server-side)

The mobile app requires the android-SDK and android-NDK toolkits.

You can place the dependencies under `third_party` or use your favorite
package manager.


## Compilation

Once the requirements are satisfied, compile and test:

```shell
mkdir build
cd build
cmake ..
make
make test
```

## Android project setup

Using ant.

```shell
cd src/recipe_mobile
android update project -p .
```


## Known issues

The Python-SWIG interface can be a pain to link on OS X with Homebrew-Python
when using a virtualenv. Override the system python frameworks:

```shell
sudo mv /System/Library/Frameworks/Python.framework /System/Library/Frameworks/Python.framework.backup
sudo ln -s /usr/local/Cellar/python/2.7.10/Frameworks/Python.framework/ /System/Library/Frameworks/Python.framework
```



## Server setup for Ubuntu

Install apache server and wsgi module. Copy wsgi module from available to enabled.

```shell
sudo apt-get apache2 install libapache2-mod-wsgi
sudo ln -s /etc/apache2/mods-available/wsgi.load /etc/apache2/mods-enabled
sudo ln -s /etc/apache2/mods-available/wsgi.conf /etc/apache2/mods-enabled
sudo touch /etc/apache2/sites-available/xform.conf
sudo ln -s /etc/apache2/sites-available/xform.conf /etc/apache2/sites-enabled
```

Paste the following contents in `` /etc/apache2/sites-available/xform.conf  ``
```xml
<VirtualHost *:80>
    LogLevel info

    ServerName dev.xform.com
    ServerAdmin username@xform.com

    # Static files
    DocumentRoot "/var/www/xform.com"
    Alias /static/ /var/www/xform.com/static
    <Directory "/var/www/xform.com/static">
        Order deny,allow
        Allow from all
    </Directory>

    # WSGI
    WSGIDaemonProcess xform.com user=username processes=2 threads=15 python-path=/home/username/Projects/xform/xform_mobile/python/:/usr/local/lib/python2.7/dist-packages:/usr/lib/python2.7/dist-packages
    WSGIProcessGroup xform.com
    WSGIScriptAlias / /var/www/xform.com/application.py

    <Directory "/var/www/xform.com">
        WSGIScriptReloading On
        Options FollowSymLinks
        Order deny,allow
        Allow from all
    </Directory>

    ErrorLog "/var/www/logs/xform.com-error_log"
    CustomLog "/var/www/logs/xform.com-access_log" common

</VirtualHost>
```

Restart the apache server.
```shell
sudo a2dissite 000-default         # disable default site
sudo a2ensite xform                # enable xform site
sudo mkdir /var/www/logs           # create the log directory
sudo chown username /var/www/logs  # change permissions of log dir so that wsgi can write in it
sudo apachectl start               # restart the server
```

---

## Server setup for OS X

The server app is a python WSGI script. To run on a local apache server (tested on mac osx 10.9):

* Install the mod_wsgi (e.g. `brew install mod_wsgi`)

* Add this line to `/etc/apache2/httpd.conf`:

```xml
    LoadModule wsgi_module /usr/local/Cellar/mod_wsgi/4.4.9/libexec/mod_wsgi.so
```

* Add this to `/etc/apache2/extra/httpd-vhosts.conf`

```xml
    NameVirtualHost *:80
    include /private/etc/apache2/extra/vhosts/dev.xform.com.conf
```

* Create the file `/etc/apache2/extra/vhosts/dev.xform.com.conf` and add:

```xml
    <VirtualHost *:80>
        LogLevel info

        ServerName dev.xform.com
        ServerAdmin username@xform.com

        # Static files
        DocumentRoot "/Users/username/Sites/xform.com"
        Alias /static/ /Users/username/Sites/xform.com/static
        <Directory "/Users/username/Sites/xform.com/static">
                Order deny,allow
                Allow from all
        </Directory>

        # WSGI
        WSGIDaemonProcess xform.com user=username processes=2 threads=15 python-path=/Users/username/Documents/projects/xform_mobile/python/:/Users/username/.virtualenvs/default/lib/python2.7/site-packages

        WSGIProcessGroup xform.com

        WSGIScriptAlias / /Users/username/Sites/xform.com/application.py

        <Directory "/Users/username/Sites/xform.com">
            WSGIScriptReloading On
            Options FollowSymLinks
            Order deny,allow
            Allow from all
        </Directory>

        ErrorLog "/Users/username/Sites/logs/xform.com-error_log"
        CustomLog "/Users/username/Sites/logs/xform.com-access_log" common

    </VirtualHost>
```

For the python warpper to work on WSGI put libHalide.so in a standard lib directory (e.g. simlink)

    ln -s /Users/username/Documents/projects/xform_mobile/third_party/halide/bin/libHalide.so /usr/local/lib/libHalide.so


* Launch the web server: `sudo apachectl start`.

## Client App

The client app is an Android Java app with JNI bindings. Once compiled, you can install the app on your device with:
    
```shell
    cd src/recipe_mobile/
    ant debug
    ant installd
```

Change the server's ip address that is hardcoded in `src/recipe_mobile/src/com/xform/recipe_mobile/RecipeMobile.java`

Citation
--------

If you use any part of this code, please cite our paper:

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

