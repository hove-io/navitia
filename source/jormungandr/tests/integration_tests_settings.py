# encoding: utf-8
# Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
#     powered by Hove (www.hove.com).
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
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io
from __future__ import absolute_import
import os

START_MONITORING_THREAD = False

SAVE_STAT = True

DISABLE_DATABASE = True
# for tests we want only 1 second timeout instead of the normal 10s
INSTANCE_TIMEOUT = int(os.environ.get('CUSTOM_INSTANCE_TIMEOUT', 1))
STAT_CIRCUIT_BREAKER_MAX_FAIL = int(os.getenv('JORMUNGANDR_STAT_CIRCUIT_BREAKER_MAX_FAIL', 1000))
STAT_CIRCUIT_BREAKER_TIMEOUT_S = int(os.getenv('JORMUNGANDR_STAT_CIRCUIT_BREAKER_TIMEOUT_S', 1))

# do not authenticate for tests
PUBLIC = True

LOGGER = {
    'version': 1,
    'disable_existing_loggers': False,
    'formatters': {
        'default': {'format': '[%(asctime)s] [%(levelname)5s] [%(process)5s] [%(name)10s] %(message)s'}
    },
    'handlers': {'default': {'level': 'INFO', 'class': 'logging.StreamHandler', 'formatter': 'default'}},
    'loggers': {
        '': {'handlers': ['default'], 'level': 'INFO', 'propagate': True},
        'navitiacommon.default_values': {'handlers': ['default'], 'level': 'ERROR', 'propagate': True},
    },
}

CACHE_CONFIGURATION = {'CACHE_TYPE': 'null'}

# List of enabled modules
MODULES = {
    'v1': {  # API v1 of Navitia
        'import_path': 'jormungandr.modules.v1_routing.v1_routing',
        'class_name': 'V1Routing',
    }
}

# circuit breaker parameters, for the tests by default we don't want the circuit breaker
CIRCUIT_BREAKER_MAX_INSTANCE_FAIL = 99999
CIRCUIT_BREAKER_INSTANCE_TIMEOUT_S = 1

GRAPHICAL_ISOCHRONE = True
HEAT_MAP = True

PATCH_WITH_GEVENT_SOCKET = True

GREENLET_POOL_FOR_RIDESHARING_SERVICES = True
