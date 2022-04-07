#  Copyright (c) 2001-2016, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.kisio.com).
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

from jormungandr.street_network.street_network import AbstractStreetNetworkService, StreetNetworkPathType
from jormungandr import utils, fallback_modes as fm
import six


class Ridesharing(AbstractStreetNetworkService):
    def __init__(self, instance, service_url, modes=None, id=None, timeout=10, api_key=None, **kwargs):
        self.instance = instance
        self.modes = modes or [fm.FallbackModes.ridesharing.name]
        assert list(self.modes) == [fm.FallbackModes.ridesharing.name], (
            'Class: ' + str(self.__class__) + ' can only be used for ridesharing'
        )
        self.sn_system_id = id or 'ridesharing'
        config = kwargs.get('street_network', {})
        if 'service_url' not in config['args']:
            config['args'].update({'service_url': None})
        if 'instance' not in config['args']:
            config['args'].update({'instance': instance})
        config['args'].update({'modes': self.modes})
        self.street_network = utils.create_object(config)

    def status(self):
        return {
            'id': six.text_type(self.sn_system_id),
            'class': self.__class__.__name__,
            'modes': self.modes,
            'backend_class': self.street_network.__class__.__name__,
        }

    def _direct_path(
        self,
        instance,
        mode,
        pt_object_origin,
        pt_object_destination,
        fallback_extremity,
        request,
        direct_path_type,
        request_id,
    ):
        # TODO: the ridesharing_speed is stored in car_no_park_speed
        # a proper way to handle this is to override car_no_park_speed use the ridesharing_speed here
        # copy_request = copy.deepcopy(request)
        # copy_request["car_no_park_speed"] = copy_request["ridesharing_speed"]
        response = self.street_network._direct_path(
            instance,
            mode,
            pt_object_origin,
            pt_object_destination,
            fallback_extremity,
            request,
            direct_path_type,
            request_id,
        )
        for journey in response.journeys:
            journey.durations.ridesharing = journey.durations.car
            journey.distances.ridesharing = journey.distances.car
            journey.durations.car = 0
            journey.distances.car = 0
            for section in journey.sections:
                section.street_network.mode = fm.FallbackModes[mode].value

        return response

    def _get_street_network_routing_matrix(
        self, instance, origins, destinations, street_network_mode, max_duration, request, request_id, **kwargs
    ):
        # TODO: the ridesharing_speed is stored in car_no_park_speed
        # a proper way to handle this is to override car_no_park_speed use the ridesharing_speed here
        # copy_request = copy.deepcopy(request)
        # copy_request["car_no_park_speed"] = copy_request["ridesharing_speed"]
        return self.street_network.get_street_network_routing_matrix(
            instance, origins, destinations, street_network_mode, max_duration, request, request_id, **kwargs
        )

    def make_path_key(self, mode, orig_uri, dest_uri, streetnetwork_path_type, period_extremity):
        """
        :param orig_uri, dest_uri, mode: matters obviously
        :param streetnetwork_path_type: whether it's a fallback at
        the beginning, the end of journey or a direct path without PT also matters especially for car (to know if we
        park before or after)
        :param period_extremity: is a PeriodExtremity (a datetime and its meaning on the
        fallback period)
        Nota: period_extremity is not taken into consideration so far because we assume that a
        direct path from A to B remains the same even the departure time are different (no realtime)
        """
        return self.street_network.make_path_key(mode, orig_uri, dest_uri, streetnetwork_path_type, None)
