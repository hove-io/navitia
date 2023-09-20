# Copyright (c) 2001-2023, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
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
from jormungandr import utils


class NoRequestedPtJourneyFare(Exception):
    pass


class PtJourneyFareBackendManager(object):
    def __init__(self, instance, configs, db_configs_getter=None):
        # TODO: read from db
        self._db_configs_getter = db_configs_getter

        self.instance = instance
        kraken_default_config = {
            "class": "jormungandr.pt_journey_fare.kraken.Kraken",
            "args": {"instance": self.instance},
        }
        backends_configs = {
            "kraken": kraken_default_config,
        }
        backends_configs.update(configs or {})

        self.backends = {}

        self.init(backends_configs)

    def init(self, backends_configs):
        for k, v in backends_configs.items():
            self.backends[k] = utils.create_object(v)

    def get_pt_journey_fare(self, backend_id):
        backend = self.backends.get(backend_id)
        if backend:
            return backend
        raise NoRequestedPtJourneyFare("no requested pt_journey_fare backend: {}".format(backend_id))
