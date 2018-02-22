from __future__ import absolute_import, print_function, unicode_literals, division
import os

if not 'TYR_CONFIG_FILE' in os.environ:
    os.environ['TYR_CONFIG_FILE'] = os.path.dirname(os.path.realpath(__file__)) \
        + '/tests_default_settings.py'
