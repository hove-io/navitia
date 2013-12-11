import logging
from celery import Celery
from pyed.models import Instance

from configobj import ConfigObj, flatten_errors
from validate import Validator
import glob


def configure_logger(app):
    """
    configuration du logging
    """
    filename = app.config["LOG_FILENAME"]
    handler = logging.FileHandler(filename)
    log_format = '[%(asctime)s] [%(levelname)s] [%(name)s] - %(message)s - ' \
            '%(filename)s:%(lineno)d in %(funcName)s'
    handler.setFormatter(logging.Formatter(log_format))
    app.logger.addHandler(handler)
    app.logger.setLevel(app.config["LOG_LEVEL"])

    logging.getLogger('sqlalchemy.engine').setLevel(
            app.config["LOG_LEVEL_SQLALCHEMY"])
    logging.getLogger('sqlalchemy.pool').setLevel(
            app.config["LOG_LEVEL_SQLALCHEMY"])
    logging.getLogger('sqlalchemy.dialects.postgresql')\
            .setLevel(app.config["LOG_LEVEL_SQLALCHEMY"])

    logging.getLogger('sqlalchemy.engine').addHandler(handler)
    logging.getLogger('sqlalchemy.pool').addHandler(handler)
    logging.getLogger('sqlalchemy.dialects.postgresql').addHandler(handler)


def make_celery(app):
    celery = Celery(app.import_name, broker=app.config['CELERY_BROKER_URL'])
    celery.conf.update(app.config)
    TaskBase = celery.Task

    class ContextTask(TaskBase):
        abstract = True

        def __init__(self):
            pass

        def __call__(self, *args, **kwargs):
            with app.app_context():
                return TaskBase.__call__(self, *args, **kwargs)

    celery.Task = ContextTask
    return celery


def load_instances(app):
    confspec = []
    confspec.append('[instance]')
    confspec.append('name = string()')
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
    instances = {}

    ini_files = glob.glob(app.config['INSTANCES_DIR'] + '/*.ini')

    for filename in ini_files:
        config = ConfigObj(filename, configspec=confspec, stringify=True)
        val = Validator()
        res = config.validate(val, preserve_errors=True)
        #validate retourne true, ou un dictionaire  ...
        if res != True:
            error = build_error(config, res)
            raise ValueError("Config is not valid: %s in %s" \
                    % (error, filename))
        instance = Instance(config)
        instances[instance.name] = instance

    app.instances = instances


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
