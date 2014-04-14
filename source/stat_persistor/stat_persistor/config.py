#encoding: utf-8
from configobj import ConfigObj, flatten_errors
from validate import Validator


class Config(object):
    """
    class de configuration de stat_persistor

    """
    def __init__(self):
        self.broker_url = None
        self.stat_connection_string = None
        self.exchange_name = None
        self.rt_topic =None
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
        confspec = []
        confspec.append('[stat]')
        confspec.append('connection-string = string()')

        confspec.append('[stat_persistor]')
        confspec.append('exchange-name = string(default="navitia")')
        confspec.append('stat-topic = string(default="stat.sender")')
        confspec.append('queue-name = string(default="stat_persistor")')
        confspec.append('broker-url = string(default="amqp://guest:guest@localhost:5672//")')
        confspec.append('log-file = string(default=None)')
        confspec.append('log-level = string(default="debug")')
        confspec.append('stat-file = string(default="/home/canaltp/NAViTiA/data/stat_persistor.log")')


        config = ConfigObj(config_file, configspec=confspec, stringify=True)

        val = Validator()
        res = config.validate(val, preserve_errors=True)
        #validate retourne true, ou un dictionaire  ...
        if type(res) is dict:
            error = self.build_error(config, res)
            raise ValueError("Config is not valid: " + error)

        self.stat_connection_string = config['stat']['connection-string']
        self.broker_url = config['stat_persistor']['broker-url']
        self.rt_topic = config['stat_persistor']['stat-topic']
        self.queue_name = config['stat_persistor']['queue-name']
        self.exchange_name = config['stat_persistor']['exchange-name']

        self.log_level = config['stat_persistor']['log-level']
        self.log_file = config['stat_persistor']['log-file']
        self.stat_file = config['stat_persistor']['stat-file']
