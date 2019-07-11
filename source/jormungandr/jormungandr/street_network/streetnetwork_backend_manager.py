# Copyright (c) 2001-2019, Canal TP and/or its affiliates. All rights reserved.
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

from jormungandr import utils
from jormungandr import fallback_modes as fm
from jormungandr.exceptions import TechnicalError
from importlib import import_module

from collections import defaultdict
import logging, itertools, copy


class StreetNetworkBackendManager(object):
    def __init__(self):
        self.logger = logging.getLogger(__name__)
        # dict { "instance" : [street_network_backends] }
        self._streetnetwork_backends_by_instance = defaultdict(list)

    def init_streetnetwork_backends(self, instance, instance_configuration):
        instance_configuration = self._append_default_street_network_to_config(instance_configuration)
        self._create_street_network_backends(instance, instance_configuration)

    # For street network modes that are not set in the given config file,
    # we set kraken as their default engine
    def _append_default_street_network_to_config(self, instance_configuration):
        if not isinstance(instance_configuration, list):
            instance_configuration = []

        kraken = {'class': 'jormungandr.street_network.Kraken', 'args': {'timeout': 10}}
        taxi = {'class': 'jormungandr.street_network.Taxi', 'args': {'street_network': kraken}}
        ridesharing = {'class': 'jormungandr.street_network.Ridesharing', 'args': {'street_network': kraken}}

        default_sn_class = {mode: kraken for mode in fm.all_fallback_modes}
        # taxi mode's default class is changed to 'taxi' not kraken
        default_sn_class.update({fm.FallbackModes.taxi.name: taxi})
        default_sn_class.update({fm.FallbackModes.ridesharing.name: ridesharing})

        modes_in_configs = set(
            list(itertools.chain.from_iterable(config.get('modes', []) for config in instance_configuration))
        )
        modes_not_set = fm.all_fallback_modes - modes_in_configs

        for mode in modes_not_set:
            config = {"modes": [mode]}
            config.update(default_sn_class[mode])
            instance_configuration.append(copy.deepcopy(config))

        return instance_configuration

    def _create_street_network_backends(self, instance, instance_configuration):
        for config in instance_configuration:
            # Set default arguments
            if 'args' not in config:
                config['args'] = {}
            if 'service_url' not in config['args']:
                config['args'].update({'service_url': None})
            if 'instance' not in config['args']:
                config['args'].update({'instance': instance})
            # for retrocompatibility, since 'modes' was originaly outside 'args'
            if 'modes' not in config['args']:
                config['args']['modes'] = config.get('modes', [])

            backend = utils.create_object(config)

            self._streetnetwork_backends_by_instance[instance].append(backend)
            self.logger.info(
                '** StreetNetwork {} used for direct_path with mode: {} **'.format(
                    type(backend).__name__, backend.modes
                )
            )

    def get_street_network(self, instance, mode, request):
        overriden_sn_id = request.get('_street_network')
        if overriden_sn_id:

            def predicate(s):
                return s.sn_system_id == overriden_sn_id

        else:

            def predicate(s):
                return mode in s.modes

        sn = next((s for s in self._streetnetwork_backends_by_instance[instance] if predicate(s)), None)
        if sn is None:
            raise TechnicalError(
                'impossible to find a streetnetwork module for {} ({})'.format(mode, overriden_sn_id)
            )
        return sn

    def get_all_street_networks(self, instance):
        return self._streetnetwork_backends_by_instance[instance]
