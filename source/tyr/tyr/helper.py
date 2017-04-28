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
import tempfile


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
        self.target_file = None
        self.exchange = None
        self.synonyms_file = None
        self.aliases_file = None
        self.pg_host = None
        self.pg_dbname = None
        self.pg_username = None
        self.pg_password = None
        self.pg_port = None
        self.name = None


def load_instance_config(instance_name):
    confspec = []
    confspec.append('[instance]')
    confspec.append('source-directory = string()')
    confspec.append('backup-directory = string()')
    confspec.append('target-file = string()')
    confspec.append('exchange = string(default="navitia")')
    confspec.append('synonyms_file = string(default="")')
    confspec.append('aliases_file = string(default="")')
    confspec.append('name = string()')
    confspec.append('is-free = boolean(default=False)')

    confspec.append('[database]')
    confspec.append('host = string()')
    confspec.append('dbname = string()')
    confspec.append('username = string()')
    confspec.append('password = string()')
    confspec.append('port = string(default="5432")')

    ini_file = '%s/%s.ini' % \
               (os.path.realpath(current_app.config['INSTANCES_DIR']), instance_name)
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
    instance.target_file = config['instance']['target-file']
    instance.exchange = config['instance']['exchange']
    instance.synonyms_file = config['instance']['synonyms_file']
    instance.aliases_file = config['instance']['aliases_file']
    instance.name = config['instance']['name']
    instance.is_free = config['instance']['is-free']

    instance.pg_host = config['database']['host']
    instance.pg_dbname = config['database']['dbname']
    instance.pg_username = config['database']['username']
    instance.pg_password = config['database']['password']
    instance.pg_port = config['database']['port']
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

    get the logger name 'instance' as the parent logger

    For file handler, all log will be in a file specific to this instance
    """
    logger = logging.getLogger('instance.{0}'.format(instance.name))

    # if the logger has already been inited, we can stop
    if logger.handlers:
        return logger

    for handler in logger.parent.handlers:
        #trick for FileHandler, we change the file name
        if isinstance(handler, logging.FileHandler):
            #we use the %(name) notation to use the same grammar as the python module
            log_filename = handler.stream.name.replace('%(name)', instance.name)
            new_handler = logging.FileHandler(log_filename)
            new_handler.setFormatter(handler.formatter)
            handler = new_handler

        logger.addHandler(handler)

    logger.propagate = False
    return logger

def get_named_arg(arg_name, func, args, kwargs):
    if kwargs and arg_name in kwargs:
        return kwargs[arg_name]
    else:
        idx = func.func_code.co_varnames.index(arg_name)
        if args and idx < len(args):
            return args[idx]
        else:
            return None


def save_in_tmp(file_storage):
    """
    Save stream file in temp directory
    :param file_storage: stream file
    :return: fileneme
    """
    tmp_file = os.path.join(tempfile.gettempdir(), file_storage.filename)
    file_storage.save(tmp_file)
    return tmp_file
