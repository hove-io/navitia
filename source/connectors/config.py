#encoding: utf-8
from configobj import ConfigObj, flatten_errors
from validate import Validator


class Config(object):
    """
    class de configuration de at_connector

    """
    def __init__(self):
        self.rabbitmq_host = None
        self.rabbitmq_port = None
        self.rabbitmq_username = None
        self.rabbitmq_password = None
        self.rabbitmq_vhost = None

        self.at_connection_string = None

        self.exchange_name = None
        self.rt_topics = []

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
        charge la configuration depuis le fichier de conf et la valide
        si elle n'est pas valide une ValueError est levé
        """
        confspec = []
        confspec.append('[rabbitmq]')
        confspec.append('host = string(default="localhost")')
        confspec.append('port = integer(0, 65535, default=5672)')
        confspec.append('username = string(default="guest")')
        confspec.append('password = string(default="guest")')
        confspec.append('vhost = string(default="/")')

        confspec.append('[at]')
        confspec.append('connection-string = string()')

        config = ConfigObj(config_file, configspec=confspec, stringify=True)

        val = Validator()
        res = config.validate(val, preserve_errors=True)
        #validate retourne true, ou un dictionaire  ...
        if not res:
            error = self.build_error(config, res)
            raise ValueError("Config is not valid: " + error)

        self.rabbitmq_host = config['rabbitmq']['host']
        self.rabbitmq_port = config['rabbitmq']['port']
        self.rabbitmq_username = config['rabbitmq']['username']
        self.rabbitmq_password = config['rabbitmq']['password']
        self.rabbitmq_vhost = config['rabbitmq']['vhost']
        self.rabbitmq_vhost = config['rabbitmq']['vhost']
        self.at_connection_string = config['at']['connection-string']
        self.exchange_name = config['connector_at']['exchange-name']
        self.rt_topics = config['connector_at']['rt-topics']
