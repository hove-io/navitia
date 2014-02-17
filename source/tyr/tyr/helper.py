import logging
import celery


from configobj import ConfigObj, flatten_errors
from validate import Validator
from flask import current_app
import os


def configure_logger(app):
    """
    initialize logging
    """
    filename = app.config["LOG_FILENAME"]
    celery.log.setup_logger(loglevel=app.config["LOG_LEVEL"], logfile=filename)
    app.logger.setLevel(app.config["LOG_LEVEL"])

    logging.getLogger('sqlalchemy.engine').setLevel(
            app.config["LOG_LEVEL_SQLALCHEMY"])
    logging.getLogger('sqlalchemy.pool').setLevel(
            app.config["LOG_LEVEL_SQLALCHEMY"])
    logging.getLogger('sqlalchemy.dialects.postgresql')\
            .setLevel(app.config["LOG_LEVEL_SQLALCHEMY"])


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

    confspec.append('[database]')
    confspec.append('host = string()')
    confspec.append('dbname = string()')
    confspec.append('username = string()')
    confspec.append('password = string()')

    ini_file = '%s/%s.ini' % \
            (current_app.config['INSTANCES_DIR'], instance_name)

    config = ConfigObj(ini_file, configspec=confspec, stringify=True)
    val = Validator()
    res = config.validate(val, preserve_errors=True)
    #validate retourne true, ou un dictionaire  ...
    if res != True:
        error = build_error(config, res)
        raise ValueError("Config is not valid: %s in %s" \
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
        if error == False:
            error = 'Missing value or section.'
        result += section_string + ' => ' + str(error) + "\n"
    return result


def get_instance_logger(instance):
    """
    return the logger for this instance
    all log will be in a file specific to this instance
    """
    logger = logging.getLogger('tyr.{0}'.format(instance.name))
    #does not add the handler at each time
    if not logger.handlers:
        log_dir = os.path.dirname(current_app.config['LOG_FILENAME'])
        log_filename = log_dir + '/{0}.log'.format(instance.name)
        logger.setLevel(current_app.config["LOG_LEVEL"])
        handler = logging.FileHandler(log_filename)
        log_format = '[%(asctime)s: %(levelname)s/%(processName)s]' \
                ' %(message)s'
        handler.setFormatter(logging.Formatter(log_format))
        logger.addHandler(handler)

    logger.propagate = False
    return logger
