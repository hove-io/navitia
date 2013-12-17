#!/usr/bin/env python

from distutils.core import setup
from setuptools import find_packages

setup(name='jormungandr',
      version='0.10.0',
      description='webservice d\'exposition en http de kraken',
      author='CanalTP',
      author_email='vincent.lara@canaltp.fr',
      url='www.navitia.io',
      packages=find_packages()
      )
