# Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Canal TP (www.canaltp.fr).
# Help us simplify mobility and open public transport:
#     a non ending quest to the responsive locomotion way of traveling!
#
# LICENCE: This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#
# Stay tuned using
# twitter @navitia
# IRC #navitia on freenode
# https://groups.google.com/d/forum/navitia
# www.navitia.io

import logging
import logging.config
import celery

from configobj import ConfigObj, flatten_errors
from validate import Validator
from flask import current_app
import os
import sys


def configure_logger(app):
    """
    initialize logging
    """
    if 'LOGGER' in app.config:
        logging.config.dictConfig(app.config['LOGGER'])
    else:  # Default is std out
        handler = logging.StreamHandler(stream=sys.stdout)
        app.logger.addHandler(handler)
        app.logger.setLevel('INFO')

def make_celery(app):
    celery_app = celery.Celery(app.import_name,
                               broker=app.config['CELERY_BROKER_URL'])
    celery_app.conf.update(app.config)
    TaskBase = celery_app.Task

    class ContextTask(TaskBase):
        abstract = True

        def __init__(self):
            pass

        def __call__(self, *args, **kwargs):
            with app.app_context():
                return TaskBase.__call__(self, *args, **kwargs)

    celery_app.Task = ContextTask
    return celery_app


class InstanceConfig(object):
    def __init__(self):
        self.source_directory = None
        self.backup_directory = None
        self.tmp_file = None
        self.target_file = None
        self.rt_topics = None
        self.exchange = None
        self.synonyms_file = None
        self.aliases_file = None
        self.pg_host = None
        self.pg_dbname = None
        self.pg_username = None
        self.pg_password = None
        self.name = None


def load_instance_config(instance_name):
    confspec = []
    confspec.append('[instance]')
    confspec.append('source-directory = string()')
    confspec.append('backup-directory = string()')
    confspec.append('tmp-file = string()')
    confspec.append('target-file = string()')
    confspec.append('rt-topics = string_list()')
    confspec.append('exchange = string(default="navitia")')
    confspec.append('synonyms_file = string(default="")')
    confspec.append('aliases_file = string(default="")')
    confspec.append('name = string()')

    confspec.append('[database]')
    confspec.append('host = string()')
    confspec.append('dbname = string()')
    confspec.append('username = string()')
    confspec.append('password = string()')

    ini_file = '%s/%s.ini' % \
               (current_app.config['INSTANCES_DIR'], instance_name)
    if not os.path.isfile(ini_file):
        raise ValueError("File doesn't exists or is not a file %s" % ini_file)

    config = ConfigObj(ini_file, configspec=confspec, stringify=True)
    val = Validator()
    res = config.validate(val, preserve_errors=True)
    #validate retourne true, ou un dictionaire  ...
    if type(res) is dict:
        error = build_error(config, res)
        raise ValueError("Config is not valid: %s in %s"
                % (error, ini_file))
    instance = InstanceConfig()
    instance.source_directory = config['instance']['source-directory']
    instance.backup_directory = config['instance']['backup-directory']
    instance.tmp_file = config['instance']['tmp-file']
    instance.target_file = config['instance']['target-file']
    instance.rt_topics = config['instance']['rt-topics']
    instance.exchange = config['instance']['exchange']
    instance.synonyms_file = config['instance']['synonyms_file']
    instance.aliases_file = config['instance']['aliases_file']
    instance.name = config['instance']['name']

    instance.pg_host = config['database']['host']
    instance.pg_dbname = config['database']['dbname']
    instance.pg_username = config['database']['username']
    instance.pg_password = config['database']['password']
    return instance


def build_error(config, validate_result):
    """
    construit la chaine d'erreur si la config n'est pas valide
    """
    result = ""
    for entry in flatten_errors(config, validate_result):
        # each entry is a tuple
        section_list, key, error = entry
        if key is not None:
            section_list.append(key)
        else:
            section_list.append('[missing section]')
        section_string = ', '.join(section_list)
        if error is False:
            error = 'Missing value or section.'
        result += section_string + ' => ' + str(error) + "\n"
    return result

def get_instance_logger(instance):
    """
    return the logger for this instance
    all log will be in a file specific to this instance
    """
    logger = logging.getLogger('tyr.{0}'.format(instance.name))
    root_logger = logging.root
    logger.setLevel(root_logger.level)
    file_hanlders = [h for h in root_logger.handlers
                    if isinstance(h, logging.FileHandler)]
    #does not add the handler at each time or if we do not log to a file
    if not logger.handlers and file_hanlders:
        log_dir = os.path.dirname(file_hanlders[0].stream.name)
        log_filename = '{log_dir}/{name}.log'.format(log_dir=log_dir,
                                                     name=instance.name)
        handler = logging.FileHandler(log_filename)
        handler.setFormatter(file_hanlders[0].formatter)
        logger.addHandler(handler)
        logger.propagate = False


    return logger
