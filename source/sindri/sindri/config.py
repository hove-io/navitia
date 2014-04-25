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

        self.log_file = None
        self.log_level = None

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
        confspec.append('log-file = string(default=None)')
        confspec.append('log-level = string(default="debug")')


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

        self.log_level = config['sindri']['log-level']
        self.log_file = config['sindri']['log-file']
