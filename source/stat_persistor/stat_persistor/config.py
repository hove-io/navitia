#encoding: utf-8
from configobj import ConfigObj, flatten_errors
from validate import Validator
import json

class Config(object):
    """
    class de configuration de stat_persistor
    """
    def __init__(self):
        self.broker_url = None
        self.stat_connection_string = None
        self.exchange_name = None
        self.queue_name = None
        self.log_file = None
        self.log_level = None
        self.stat_file = None

    def build_error(self, config, validate_result):
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
            if type(error) is bool and not error:
                error = 'Missing value or section.'
            result += section_string + ' => ' + str(error) + "\n"
        return result

    def load(self, config_file):
        """
        Initialize from a configuration file.
        If not valid raise an error.
        """
        try:
            config_data = json.loads(open(config_file).read())

            if 'database' in config_data and \
                'connection-string' in config_data['database']:
                self.stat_connection_string = config_data['database']['connection-string']

            if 'stat_persistor' in config_data:
                if 'exchange-name' in config_data['stat_persistor']:
                    self.exchange_name = config_data['stat_persistor']['exchange-name']

                if 'queue-name' in config_data['stat_persistor']:
                    self.queue_name = config_data['stat_persistor']['queue-name']

                if 'broker-url' in config_data['stat_persistor']:
                    self.broker_url = config_data['stat_persistor']['broker-url']

                if 'log-level' in config_data['stat_persistor']:
                    self.log_level = config_data['stat_persistor']['log-level']

                if 'log-file' in config_data['stat_persistor']:
                    self.log_file = config_data['stat_persistor']['log-file']

        except ValueError as e:
            raise ValueError("Config is not valid: " + str(e))