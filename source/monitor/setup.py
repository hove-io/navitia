#!/usr/bin/env python

from distutils.core import setup
import glob

setup(name='monitor_kraken',
        version='0.39.0',
        description="Provide a small api for monitoring kraken",
        author='CanalTP',
        author_email='alexandre.jacquin@canaltp.fr',
        url='www.navitia.io',
        packages=['monitor_kraken']
)
