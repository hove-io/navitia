#!/usr/bin/env python

from distutils.core import setup
from setuptools import find_packages
import glob

setup(name='connectors',
        version='0.1.0',
        description="Provide connectors to different realtime sources",
        author='CanalTP',
        author_email='alexandre.jacquin@canaltp.fr',
        url='www.navitia.io',
        packages=find_packages(),
        requires=['sqlalchemy', 'ampq', 'anyjson', 'argparse', 'kombu',
                  'protobuf', 'pymssql', 'redis', 'wsgiref', 'configobj'],
        scripts=['connector_at.py'])

