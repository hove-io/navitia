#encoding: utf-8
from configobj import ConfigObj, flatten_errors
from validate import Validator


class Config(object):
    """
    class de configuration of at_connector

    """
    def __init__(self):
        self.broker_url = None

        self.at_connection_string = None

        self.exchange_name = None
        self.rt_topic = []

        self.redisqueque_host = None
        self.redisqueque_port = None
        self.redisqueque_password = None
        self.redisqueque_db = None

    def build_error(self, config, validate_result):
        """
        build error messages with the return of validate
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
        load and validate the configuration from the file.
        if she is not valid an ValueError is raise
        """
        confspec = []
        confspec.append('[connector-at]')
        confspec.append('rt-topic = string()')
        confspec.append('exchange-name = string(default="navitia")')
        confspec.append('at-connection-string = string()')
        confspec.append('broker-url = string()')
        confspec.append('last-exec-time-file = string(default="./last_exec_time.txt")')

        confspec.append('[redishelper]')
        confspec.append('host = string(default="localhost")')
        confspec.append('password = string(default="password")')
        confspec.append('port = integer(default=6379)')
        confspec.append('db = string(default=0)')

        config = ConfigObj(config_file, configspec=confspec, stringify=True)

        val = Validator()
        res = config.validate(val, preserve_errors=True)
        #validate returns true, or a dict...
        if type(res) is dict:
            error = self.build_error(config, res)
            raise ValueError("Config is not valid: " + error)

        self.broker_url = config['connector-at']['broker-url']
        self.at_connection_string = config['connector-at']['at-connection-string']
        self.exchange_name = config['connector-at']['exchange-name']
        self.rt_topics = config['connector-at']['rt-topic']
        self.last_exec_time_file = config['connector-at']['last-exec-time-file']

        self.redishelper_host = config['redishelper']['host']
        self.redishelper_password = config['redishelper']['password']
        self.redishelper_port = config['redishelper']['port']
        self.redishelper_db = config['redishelper']['db']
