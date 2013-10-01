"""
Class to read the Configuration 
"""
#coding=utf-8
import logging
from configobj import ConfigObj, flatten_errors
from validate import Validator

class ConfigException(Exception):
    """ Exception raised when reading the configuration """
    pass

class Config:

    """
    Reads a pyed configuration file

    The file has to contain a instance section with the params name and the 
    path of the directory where there is the following file tree :
    source/ (the directory where to put the data you want to be computed)
    backup/ (you'll find directories with backup data of the data computed)
    error/ (where the error logs go)
    data/ (computed datas)
    gtfs2ed the gtfs2ed program
    osm2ed the osm2ed program
    ed2nav the ed2nav program
    .
    Another section database, with params, name, user, host and password.
    Another one logs_files, with the params : osm2ed, gtfs2ed, ed2nav, pyed.

    Example file :
    [instance]
    name = instance_name
    exec_directory=/srv/ed/instance_name
    source_directory = /srv/ed/instance_name/source
    working_directory = /srv/ed/working_directory
    aliases = /usr/share/ed/aliases.txt
    synonyms = /usr/share/ed/synonyms.txt

    [database]
    host = localhost
    name = database_name
    user = user_name
    password = password

    [log_files] =
    osm2ed = /var/log/ed/osm2ed_instance_name
    gtfs2ed = /var/log/ed/gtfs2ed_instance_name
    ed2nav = /var/log/ed/ed2nav_instance_name
    pyed = /var/log/ed/pyed_instance_name

    [broker]
    host = localhost
    port = 5672
    username = guest
    password = guest
    vhost = /
    exchange = navitia
    """
    def __init__(self, filename):
        """ Init the configuration,
            if the file need new fields it can be just added to config
        """
        self.logger = logging.getLogger('root')
        self.logger.info("Initalizing config")
        self.filename = filename
        self.config = {"instance" : {
                                "name" : None,
                                "exec_directory": None,
                                "source_directory" : None,
                                "target_file" : None,
                                "tmp_file" : None,
                                "working_directory" : None,
                                "aliases" : None,
                                "synonyms" : None
                            },
                            "database" : {
                                "name" : None,
                                "user" : None,
                                "host" : None,
                                "password" : None
                                    },
                            "log_files" : {
                                "osm2ed" : None,
                                "gtfs2ed" : None,
                                "ed2nav" : None,
                                "pyed" : None,
                                "nav2rt" : None
                                    },
                            'broker' : {
                                'host' : None,
                                'port' : None,
                                'username' : None,
                                'password' : None,
                                'vhost' : None,
                                'exchange': None
                                }
                            }
        self.is_valid_ = False
        self.logger.info("Reading config")
        self.read()


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
            if error == False:
                error = 'Missing value or section.'
            result += section_string + ' => ' + str(error) + "\n"
        return result


    def read(self):
        """ Read and parse the configuration, according to config """
        confspec = []
        confspec.append("[instance]")
        confspec.append("name=string")
        confspec.append("exec_directory=string")
        confspec.append("source_directory=string")
        confspec.append("target_file=string")
        confspec.append("working_directory=string")
        confspec.append("[database]")
        confspec.append("name=string")
        confspec.append("user=string")
        confspec.append("host=string")
        confspec.append("password=string")
        confspec.append("[log_files]")
        confspec.append("osm2ed=string")
        confspec.append("gtfs2ed=string")
        confspec.append("ed2nav=string")
        confspec.append("pyed=string")
        confspec.append('[broker]')
        confspec.append('host = string(default="localhost")')
        confspec.append('port = integer(0, 65535, default=5672)')
        confspec.append('username = string(default="guest")')
        confspec.append('password = string(default="guest")')
        confspec.append('vhost = string(default="/")')
        confspec.append('rt-topics = string_list()')

        self.config = ConfigObj(self.filename, configspec=confspec, \
                                stringify=True)

        val = Validator()
        res = self.config.validate(val, preserve_errors=True)
        #validate retourne true, ou un dictionaire ...
        if res != True:
            error = self.build_error(self.config, res)
            raise ValueError("Config is not valid: " + error)


    def get(self, section, param_name):
        """ Retrieve the fields idenfied by it section and param_name.
            raise an error if it doesn't exists
        """
        error = None
        if not section in self.config:
            error = "Section : " + section + " isn't in the conf"
        elif not param_name in self.config[section]:
            error = "Param : " + param_name + " isn't in the conf"
        elif not self.config[section][param_name]:
            error = "Section : "+ section + " Param : "
            error += param_name + " wasn't specified"
        if error:
            self.logger.error(error)
            raise ConfigException(error)
        return self.config[section][param_name]

    def is_valid(self):
        """ Retrieves if the config is valid or nor """
        return self.is_valid_
