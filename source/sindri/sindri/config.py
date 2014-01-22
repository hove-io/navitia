#encoding: utf-8
from configobj import ConfigObj, flatten_errors
from validate import Validator


class Config(object):
    """
    class de configuration de sindri

    """
    def __init__(self):
        self.broker_url = None
        self.ed_connection_string = None

        self.exchange_name = None
        self.instance_name = None
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
        confspec.append('[ed]')
        confspec.append('connection-string = string()')

        confspec.append('[sindri]')
        confspec.append('exchange-name = string(default="navitia")')
        confspec.append('instance-name = string()')
        confspec.append('rt-topics = string_list()')
        confspec.append('broker-url = string(default="amqp://guest:guest@localhost:5672//")')

        config = ConfigObj(config_file, configspec=confspec, stringify=True)

        val = Validator()
        res = config.validate(val, preserve_errors=True)
        #validate retourne true, ou un dictionaire  ...
        if type(res) is dict:
            error = self.build_error(config, res)
            raise ValueError("Config is not valid: " + error)

        self.broker_url = config['sindri']['broker-url']
        self.ed_connection_string = config['ed']['connection-string']
        self.instance_name = config['sindri']['instance-name']
        self.rt_topics = config['sindri']['rt-topics']
        self.exchange_name = config['sindri']['exchange-name']
