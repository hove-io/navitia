#!/usr/bin/env python

from distutils.core import setup
from setuptools import find_packages

setup(name='navitiacommon',
      version='0.10.0',
      description='module shared between jormungandr and tyr',
      author='CanalTP',
      author_email='alexandre.jacquin@canaltp.fr',
      url='www.navitia.io',
      packages=find_packages()
      )
