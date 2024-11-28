# Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
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
from jormungandr import fallback_modes as fm
from jormungandr.exceptions import TechnicalError
from jormungandr.instance import Instance
from jormungandr.street_network.street_network import AbstractStreetNetworkService
from jormungandr.utils import can_connect_to_database
from navitiacommon.models.streetnetwork_backend import StreetNetworkBackend
from collections import defaultdict
import logging, itertools, copy, datetime
from typing import Any, Dict, List, Optional


class StreetNetworkBackendManager(object):
    def __init__(self, sn_backends_getter=None, update_interval=60):
        self.logger = logging.getLogger(__name__)
        self._streetnetwork_backends_by_instance_legacy = defaultdict(
            list
        )  # type: Dict[Instance, List[AbstractStreetNetworkService]]
        # sn_backends_getter contains all the street_network backends in the table streetnetwork_backend
        # with discarded = false (models.StreetNetworkBackend.all)
        self._sn_backends_getter = sn_backends_getter
        self._streetnetwork_backends = {}  # type: Dict[str, AbstractStreetNetworkService]
        self._streetnetwork_backends_last_update = {}  # type: Dict[str, datetime]
        self._last_update = datetime.datetime(1970, 1, 1)
        self._update_interval = update_interval

    def init_streetnetwork_backends_legacy(self, instance, instance_configuration):
        """
        Initialize street_network backends from configuration files
        This function is no more used except for debugging
        :param instance:
        :param instance_configuration:
        :return:
        """
        instance_configuration = self._append_default_street_network_to_config(instance_configuration)
        self._create_street_network_backends(instance, instance_configuration)

    # For street network modes that are not set in the given config file,
    # we set kraken as their default engine
    def _append_default_street_network_to_config(self, instance_configuration):
        # type: (Any) -> List[Dict[str, Dict[str, Any]]]
        if not isinstance(instance_configuration, list):
            instance_configuration = []

        kraken = {'class': 'jormungandr.street_network.Kraken', 'args': {'timeout': 10}, 'modes': []}
        taxi = {
            'class': 'jormungandr.street_network.Taxi',
            'args': {'street_network': copy.deepcopy(kraken)},
            'modes': ['taxi'],
        }
        ridesharing = {
            'class': 'jormungandr.street_network.Ridesharing',
            'args': {'street_network': copy.deepcopy(kraken)},
            'modes': ['ridesharing'],
        }

        modes_in_configs = set(
            list(itertools.chain.from_iterable(config.get('modes', []) for config in instance_configuration))
        )

        modes_not_set = fm.all_fallback_modes - modes_in_configs

        if 'taxi' in modes_not_set:
            instance_configuration.append(taxi)
        if 'ridesharing' in modes_not_set:
            instance_configuration.append(ridesharing)

        kraken['modes'] = [m for m in modes_not_set - {'taxi', 'ridesharing'}]
        instance_configuration.append(copy.deepcopy(kraken))

        return instance_configuration

    def _create_street_network_backends(self, instance, instance_configuration):
        # type: (Instance, List[Dict[str, Any]]) -> None
        for config in instance_configuration:
            # Set default arguments
            if 'args' not in config:
                config['args'] = {}

            config['args'].setdefault('service_url', None)
            config['args'].setdefault('instance', instance)
            # for retrocompatibility, since 'modes' was originaly outside 'args'
            config['args'].setdefault('modes', config.get('modes', []))

            try:
                backend = utils.create_object(config)
                self._streetnetwork_backends_by_instance_legacy[instance].append(backend)
                self.logger.info(
                    '** StreetNetwork {} used for direct_path with mode: {} **'.format(
                        type(backend).__name__, backend.modes
                    )
                )
            except Exception as e:
                logging.getLogger(__name__).error('Routing service not active (error: {})'.format(e))

    def _create_backend_from_db(self, sn_backend, instance):
        # type: (StreetNetworkBackend, Instance) -> AbstractStreetNetworkService
        config = {"class": sn_backend.klass, "args": sn_backend.args}
        config['args'].setdefault('service_url', None)
        config['args'].setdefault('instance', instance)

        return utils.create_object(config)

    def _update_sn_backend(self, sn_backend, instance):
        # type: (StreetNetworkBackend, Instance) -> None
        """
        Updates street_network backend 'sn_backend' with last_update value for 'instance'.
        :param sn_backend:
        :param instance:
        :return:
        """
        self.logger.info('Updating / Adding {} streetnetwork backend'.format(sn_backend.id))
        try:
            self._streetnetwork_backends[sn_backend.id] = self._create_backend_from_db(sn_backend, instance)
            self._streetnetwork_backends_last_update[sn_backend.id] = sn_backend.last_update()
        except Exception:
            self.logger.exception('impossible to initialize streetnetwork backend')

    def _can_connect_to_database(self):
        return can_connect_to_database()

    def _update_config(self, instance):
        # type: (Instance) -> None
        """
        Maintains and updates list of street_network backend connectors from the table streetnetwork_backend
        with discarded=false after update interval or data update for instance.
        :param instance: instance
        :return: None
        """
        if (
            self._last_update + datetime.timedelta(seconds=self._update_interval) > datetime.datetime.utcnow()
            or not self._sn_backends_getter
        ):
            return

        # If database is not accessible we update the value of self._last_update and exit
        if not self._can_connect_to_database():
            self.logger.debug('Database is not accessible')
            self._last_update = datetime.datetime.utcnow()
            return

        self.logger.debug('Updating streetnetwork backends from db')
        self._last_update = datetime.datetime.utcnow()

        try:
            # Getting all the connectors from the table streetnetwork_backend with discarded=false
            sn_backends = self._sn_backends_getter()
        except Exception as e:
            self.logger.exception('No access to table streetnetwork_backend (error: {})'.format(e))
            # database is not accessible, so let's use the values already present in self._streetnetwork_backends
            # avoid sending query to the database for another update_interval
            self._last_update = datetime.datetime.utcnow()
            return

        if not sn_backends:
            self.logger.debug('No streetnetwork backends/All streetnetwork backends disabled in db')
            self._streetnetwork_backends = {}
            self._streetnetwork_backends_last_update = {}
            return

        for sn_backend in sn_backends:
            if (
                sn_backend.id not in self._streetnetwork_backends_last_update
                or sn_backend.last_update() > self._streetnetwork_backends_last_update[sn_backend.id]
            ):
                self._update_sn_backend(sn_backend, instance)

    def get_street_network_db(self, instance, streetnetwork_backend_id):
        # type: (Instance, StreetNetworkBackend) -> Optional[AbstractStreetNetworkService]
        # Make sure we update the streetnetwork_backends list from the database before returning them
        """
        Gets the concerned street network service from the table streetnetwork_backend
        with discarded=false and id = streetnetwork_backend_id
        :param instance:
        :param streetnetwork_backend_id:
        :return:
        """
        self._update_config(instance)

        sn = self._streetnetwork_backends.get(streetnetwork_backend_id, None)
        if sn is None:
            raise TechnicalError(
                'impossible to find a streetnetwork module for instance {} with configuration {}'.format(
                    instance, streetnetwork_backend_id
                )
            )
        return sn

    def get_all_street_networks_db(self, instance):
        # type: (Instance) -> List[AbstractStreetNetworkService]
        self._update_config(instance)

        # Since modes aren't set in the backends when they are retrieved from the db
        # We have to set them manually here
        all_street_networks_with_modes = defaultdict(list)  # type: Dict[AbstractStreetNetworkService, List[str]]
        for mode in fm.all_fallback_modes:
            streetnetwork_backend_conf = getattr(instance, "street_network_{}".format(mode))
            sn = self._streetnetwork_backends.get(
                streetnetwork_backend_conf, None
            )  # type: Optional[AbstractStreetNetworkService]
            if not sn:
                continue
            all_street_networks_with_modes[sn].append(mode)

        for sn, modes in all_street_networks_with_modes.items():
            sn.modes = modes

        return [sn for sn in all_street_networks_with_modes]

    def get_street_network_legacy(self, instance, mode, request=None):
        # type: (Instance, str, Dict[str, Any]) -> AbstractStreetNetworkService
        overriden_sn_id = (request or {}).get('_street_network')
        if overriden_sn_id:

            def predicate(s):
                return s.sn_system_id == overriden_sn_id

        else:

            def predicate(s):
                return mode in s.modes

        sn = next((s for s in self._streetnetwork_backends_by_instance_legacy[instance] if predicate(s)), None)
        if sn is None:
            raise TechnicalError(
                'impossible to find a streetnetwork module for {} ({})'.format(mode, overriden_sn_id)
            )
        return sn

    def get_all_street_networks_legacy(self, instance):
        # type: (Instance) -> List[AbstractStreetNetworkService]
        return self._streetnetwork_backends_by_instance_legacy[instance]
