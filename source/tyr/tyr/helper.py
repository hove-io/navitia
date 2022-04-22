# coding: utf-8
# Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.hove.com).
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
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io

import logging
import logging.config
import uuid
import celery
from celery.exceptions import TimeoutError

from configobj import ConfigObj, flatten_errors
from validate import Validator
from flask import current_app
import os
import sys
import tempfile

from werkzeug.utils import secure_filename

END_POINT_NOT_EXIST_MSG = 'end_point doesn\'t exist'
BILLING_PLAN_NOT_EXIST_MSG = 'billing plan doesn\'t exist'


def get_message(key, email, args):
    if key not in ["end_point_id", "billing_plan_id"]:
        raise AttributeError
    msg = END_POINT_NOT_EXIST_MSG if key == "end_point_id" else BILLING_PLAN_NOT_EXIST_MSG
    if args[key]:
        return '{} for user email {}, you give "{}"'.format(msg, hide_domain(email), args[key])
    return msg


def wait_or_raise(async_result):
    """
    wait for the end of an async task.
    Reraise the exception from the task in case of an error
    """
    # wait() will only throw exception at the start of the call
    # if the worker raise an exception once the waiting has started the exception isn't raised by wait
    # so we do a periodic polling on wait...
    while True:
        try:
            async_result.wait(timeout=5)
            return  # wait ended, we stop the endless loop
        except TimeoutError:
            pass  # everything is fine: let's poll!


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
    celery_app = celery.Celery(app.import_name, broker=app.config['CELERY_BROKER_URL'])
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


def is_activate_autocomplete_version(version=2):
    if version not in [2, 7]:
        return False
    if version == 2:
        return current_app.config.get('MIMIR_URL', None) != None
    return current_app.config.get('MIMIR7_URL', None) != None


def load_instance_config(instance_name):
    def b(unicode):
        """
        convert unicode to bytestring
        """
        return unicode.encode('utf-8')

    confspec = []
    confspec.append(b(u'[instance]'))
    confspec.append(b(u'source-directory = string()'))
    confspec.append(b(u'backup-directory = string()'))
    confspec.append(b(u'target-file = string()'))
    confspec.append(b(u'exchange = string(default="navitia")'))
    confspec.append(b(u'synonyms_file = string(default="")'))
    confspec.append(b(u'aliases_file = string(default="")'))
    confspec.append(b(u'name = string()'))
    confspec.append(b(u'is-free = boolean(default=False)'))

    confspec.append(b(u'[database]'))
    confspec.append(b(u'host = string()'))
    confspec.append(b(u'dbname = string()'))
    confspec.append(b(u'username = string()'))
    confspec.append(b(u'password = string()'))
    confspec.append(b(u'port = string(default="5432")'))

    ini_file = os.path.join(os.path.realpath(current_app.config['INSTANCES_DIR']), instance_name + ".ini")
    if not os.path.isfile(ini_file):
        raise ValueError("File doesn't exists or is not a file %s" % ini_file)

    config = ConfigObj(ini_file, configspec=confspec, stringify=True)
    val = Validator()
    res = config.validate(val, preserve_errors=True)
    # validate retourne true, ou un dictionaire  ...
    if type(res) is dict:
        error = build_error(config, res)
        raise ValueError("Config is not valid: %s in %s" % (error, ini_file))
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


def get_instance_logger(instance, task_id=None):
    return _get_individual_logger('instance.{}'.format(instance.name), instance.name, task_id)


def get_autocomplete_instance_logger(a_instance, task_id=None):
    # Note: it is called instance.autocomplete to use by default the same logger as 'instance'
    return _get_individual_logger('instance.autocomplete.{}'.format(a_instance.name), a_instance.name, task_id)


def _get_individual_logger(logger_name, name, task_id=None):
    """
    return the logger for this instance

    get the logger name 'instance' as the parent logger

    For file handler, all log will be in a file specific to this instance
    """
    logger = logging.getLogger(logger_name)

    # if the logger has already been inited, we can stop
    if logger.handlers:
        return get_task_logger(logger, task_id)

    for handler in logger.parent.handlers:
        # trick for FileHandler, we change the file name
        if isinstance(handler, logging.FileHandler):
            # we use the %(name) notation to use the same grammar as the python module
            log_filename = handler.stream.name.replace('%(name)', name)
            new_handler = logging.FileHandler(log_filename)
            new_handler.setFormatter(handler.formatter)
            handler = new_handler

        logger.addHandler(handler)

    logger.propagate = False
    return get_task_logger(logger, task_id)


def get_task_logger(logger, task_id=None):
    """
    wrap a logger in a Adapter logger.
    If a job_id has been given we use it as the 'task_id', else we create a unique 'task_id'

    The 'task_id' can be used in the formated to trace tasks
    """
    task_id = task_id or str(uuid.uuid4())
    return logging.LoggerAdapter(logger, extra={'task_id': task_id})


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
    :return: filename
    """
    filename = secure_filename(file_storage.filename)
    tmp_file = os.path.join(tempfile.gettempdir(), filename)
    file_storage.save(tmp_file)
    return tmp_file


def hide_domain(email):
    if not email:
        return email
    if "@" not in email:
        return email
    return "{}******".format(email.split('@')[0])
