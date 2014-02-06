#!/usr/bin/env python
# encoding: utf-8

from distutils.core import setup

setup(name='sindri',
      version='0.40.0',
      description="responsable de l'enregistrement du temps réel dans ED ",
      author='CanalTP',
      author_email='alexandre.jacquin@canaltp.fr',
      url='www.navitia.io',
      packages=['sindri', 'sindri.saver'],
      scripts=['sindri_service']
      )
