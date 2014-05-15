#encoding: utf-8

#  Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
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

from configobj import ConfigObj, flatten_errors
import logging
import logging.config
from validate import Validator
import json
import sys


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

    def load(self, config_file):
        """
        Initialize from a configuration file.
        If not valid raise an error.
        """
        config_data = json.loads(open(config_file).read())

        if 'database' in config_data and \
            'connection-string' in config_data['database']:
            self.stat_connection_string = config_data['database']['connection-string']
        else:
            raise ValueError("Config is not valid, a connection-string is needed")

        if 'exchange-name' in config_data:
            self.exchange_name = config_data['exchange-name']
        else:
            raise ValueError("Config is not valid, exchange-name is needed")

        if 'queue-name' in config_data:
            self.queue_name = config_data['queue-name']
        else:
            raise ValueError("Config is not valid, queue-name is needed")


        if 'broker-url' in config_data:
            self.broker_url = config_data['broker-url']
        else:
            raise ValueError("Config is not valid, broker-url is needed")

        if 'logger' in config_data:
            logging.config.dictConfig(config_data['logger'])
        else:  # Default is std out
            handler = logging.StreamHandler(stream=sys.stdout)
            logging.getLogger().addHandler(handler)
            logging.getLogger().setLevel('INFO')
