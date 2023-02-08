#  Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
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

from __future__ import absolute_import, print_function, unicode_literals, division
import logging
from jormungandr.exceptions import TechnicalError
from jormungandr import app
from jormungandr.transient_socket import TransientSocket
from jormungandr.fallback_modes import FallbackModes
from jormungandr.street_network.kraken import Kraken
from jormungandr.utils import get_pt_object_coord
from jormungandr.street_network.utils import (
    make_speed_switcher,
    crowfly_distance_between,
    create_kraken_matrix_request,
)
from jormungandr.street_network.street_network import StreetNetworkPathType

from navitiacommon import response_pb2, request_pb2, type_pb2
from zmq import green as zmq
import six
from enum import Enum
import pybreaker


# Possible values implemented. Full languages within the doc:
# https://valhalla.readthedocs.io/en/latest/api/turn-by-turn/api-reference/#supported-language-tags
# Be careful, the syntax has to be exact
class Languages(Enum):
    bulgarian = "bg-BG"
    catalan = "ca-ES"
    czech = "cs-CZ"
    danish = "da-DK"
    german = "de-DE"
    greek = "el-GR"
    english_gb = "en-GB"
    english_pirate = "en-US-x-pirate"
    english_us = "en-US"
    spanish = "es-ES"
    estonian = "et-EE"
    finnish = "fi-FI"
    french = "fr-FR"
    hindi = "hi-IN"
    hungarian = "hu-HU"
    italian = "it-IT"
    japanese = "ja-JP"
    bokmal = "nb-NO"
    dutch = "nl-NL"
    polish = "pl-PL"
    portuguese_br = "pt-BR"
    portuguese_pt = "pt-PT"
    romanian = "ro-RO"
    russian = "ru-RU"
    slovak = "sk-SK"
    slovenian = "sl-SI"
    swedish = "sv-SE"
    turkish = "tr-TR"
    ukrainian = "uk-UA"


class DirectPathProfile(object):
    def __init__(
        self,
        bike_use_roads=0.5,
        bike_use_hills=0.5,
        bike_use_ferry=0.5,
        bike_avoid_bad_surfaces=0.25,
        bike_shortest=False,
        bicycle_type='hybrid',
        bike_use_living_streets=0.5,
        bike_maneuver_penalty=5,
        bike_service_penalty=0,
        bike_service_factor=1,
        bike_country_crossing_cost=600,
        bike_country_crossing_penalty=0,
        walking_walkway_factor=1,
        walking_sidewalk_factor=1,
        walking_alley_factor=2,
        walking_driveway_factor=5,
        walking_step_penalty=30,
        walking_use_ferry=0.5,
        walking_use_living_streets=0.6,
        walking_use_tracks=1,
        walking_use_hills=0,
        walking_service_penalty=0,
        walking_service_factor=1,
        walking_max_hiking_difficulty=1,
        walking_shortest=False,
        walking_ignore_oneways=True,
        tag=None,
    ):
        self.bike_use_roads = bike_use_roads
        self.bike_use_hills = bike_use_hills
        self.bike_use_ferry = bike_use_ferry
        self.bike_avoid_bad_surfaces = bike_avoid_bad_surfaces
        self.bike_shortest = bike_shortest
        self.bicycle_type = bicycle_type
        self.bike_use_living_streets = bike_use_living_streets
        self.bike_maneuver_penalty = bike_maneuver_penalty
        self.bike_service_penalty = bike_service_penalty
        self.bike_service_factor = bike_service_factor
        self.bike_service_penalty = bike_service_penalty
        self.bike_country_crossing_cost = bike_country_crossing_cost
        self.bike_country_crossing_penalty = bike_country_crossing_penalty

        self.walking_walkway_factor = walking_walkway_factor
        self.walking_sidewalk_factor = walking_sidewalk_factor
        self.walking_alley_factor = walking_alley_factor
        self.walking_driveway_factor = walking_driveway_factor
        self.walking_step_penalty = walking_step_penalty
        self.walking_use_ferry = walking_use_ferry
        self.walking_use_living_streets = walking_use_living_streets
        self.walking_use_tracks = walking_use_tracks
        self.walking_use_hills = walking_use_hills
        self.walking_service_penalty = walking_service_penalty
        self.walking_service_factor = walking_service_factor
        self.walking_max_hiking_difficulty = walking_max_hiking_difficulty
        self.walking_shortest = walking_shortest
        self.walking_ignore_oneways = walking_ignore_oneways

        self.profile_tag = tag


# yes it's hardcoded
BALANCED = DirectPathProfile(
    bike_use_living_streets=0.8, bike_maneuver_penalty=0, bike_use_roads=0.3, tag='balanced'
)
COMFORT = DirectPathProfile(
    bike_use_living_streets=1, bike_maneuver_penalty=0, bike_use_roads=0.2, bike_use_hills=0, tag='comfort'
)
SHORTEST = DirectPathProfile(bike_shortest=True, tag='shortest')


DIRECT_PATH_ALTERNATIVES_PROFILES = [BALANCED, COMFORT, SHORTEST]


class Asgard(TransientSocket, Kraken):
    def __init__(
        self,
        instance,
        service_url,
        asgard_socket,
        modes=None,
        id=None,
        timeout=10,
        api_key=None,
        socket_ttl=app.config['ASGARD_ZMQ_SOCKET_TTL_SECONDS'],
        **kwargs
    ):
        super(Asgard, self).__init__(
            name="asgard_{}".format(instance.name),
            zmq_context=instance.context,
            zmq_socket=app.config.get(str("ASGARD_ZMQ_SOCKET")) or asgard_socket,
            socket_ttl=socket_ttl,
            instance=instance,
            service_url=service_url,
            modes=modes or [],
            id=id or 'asgard',
            timeout=timeout,
            api_key=api_key,
            **kwargs
        )
        self.timeout = timeout

        self.breaker = pybreaker.CircuitBreaker(
            fail_max=app.config['CIRCUIT_BREAKER_MAX_ASGARD_FAIL'],
            reset_timeout=app.config['CIRCUIT_BREAKER_ASGARD_TIMEOUT_S'],
        )
        self.socket_ttl = socket_ttl
        self.logger = logging.getLogger(__name__)

    def status(self):
        return {
            'id': six.text_type(self.sn_system_id),
            'class': self.__class__.__name__,
            'modes': self.modes,
            'timeout': self.timeout,
            'circuit_breaker': {
                'current_state': self.breaker.current_state,
                'fail_counter': self.breaker.fail_counter,
                'reset_timeout': self.breaker.reset_timeout,
            },
            'zmq_socket_ttl': self.socket_ttl,
            'language': self.instance.asgard_language,
        }

    def make_location(self, obj):
        coord = get_pt_object_coord(obj)
        return type_pb2.LocationContext(
            place="", access_duration=0, lon=round(coord.lon, 5), lat=round(coord.lat, 5)
        )

    def get_language_parameter(self, request):
        language = request.get('_asgard_language', "english_us")
        language_tag = getattr(Languages, language, Languages.english_us).value
        return language_tag

    def _create_sn_routing_matrix_request(
        self, origins, destinations, street_network_mode, max_duration, speed_switcher, request, **kwargs
    ):
        # asgard and kraken have a part of request in common
        req = create_kraken_matrix_request(
            self, origins, destinations, street_network_mode, max_duration, speed_switcher, request, **kwargs
        )

        # Asgard/Valhalla bike
        req.sn_routing_matrix.streetnetwork_params.bike_use_roads = request['bike_use_roads']
        req.sn_routing_matrix.streetnetwork_params.bike_use_hills = request['bike_use_hills']
        req.sn_routing_matrix.streetnetwork_params.bike_use_ferry = request['bike_use_ferry']
        req.sn_routing_matrix.streetnetwork_params.bike_avoid_bad_surfaces = request['bike_avoid_bad_surfaces']
        req.sn_routing_matrix.streetnetwork_params.bike_shortest = request['bike_shortest']
        req.sn_routing_matrix.streetnetwork_params.bicycle_type = type_pb2.BicycleType.Value(
            request['bicycle_type']
        )
        req.sn_routing_matrix.streetnetwork_params.bike_use_living_streets = request['bike_use_living_streets']
        req.sn_routing_matrix.streetnetwork_params.bike_maneuver_penalty = request['bike_maneuver_penalty']
        req.sn_routing_matrix.streetnetwork_params.bike_service_penalty = request['bike_service_penalty']
        req.sn_routing_matrix.streetnetwork_params.bike_service_factor = request['bike_service_factor']
        req.sn_routing_matrix.streetnetwork_params.bike_country_crossing_cost = request[
            'bike_country_crossing_cost'
        ]
        req.sn_routing_matrix.streetnetwork_params.bike_country_crossing_penalty = request[
            'bike_country_crossing_penalty'
        ]

        req.sn_routing_matrix.asgard_max_walking_duration_coeff = request.get(
            "_asgard_max_walking_duration_coeff"
        )
        req.sn_routing_matrix.asgard_max_bike_duration_coeff = request.get("_asgard_max_bike_duration_coeff")
        req.sn_routing_matrix.asgard_max_bss_duration_coeff = request.get("_asgard_max_bss_duration_coeff")
        req.sn_routing_matrix.asgard_max_car_duration_coeff = request.get("_asgard_max_car_duration_coeff")

        for attr in ("bss_rent_duration", "bss_rent_penalty", "bss_return_duration", "bss_return_penalty"):
            setattr(req.sn_routing_matrix.streetnetwork_params, attr, request[attr])

        return req

    def _get_street_network_routing_matrix(
        self, instance, origins, destinations, mode, max_duration, request, request_id, **kwargs
    ):
        speed_switcher = make_speed_switcher(request)

        req = self._create_sn_routing_matrix_request(
            origins, destinations, mode, max_duration, speed_switcher, request, **kwargs
        )

        # asgard doesn't know what to do with 'car_no_park'...
        if mode == FallbackModes.car_no_park.name:
            req.sn_routing_matrix.mode = FallbackModes.car.name

        res = self._call_asgard(req, request_id)
        self._check_for_error_and_raise(res)
        return res.sn_routing_matrix

    def _add_cycle_lane_length(self, response):
        def _is_cycle_lane(path):
            if path.HasField(str("cycle_path_type")):
                return path.cycle_path_type != response_pb2.NoCycleLane

            return False

        # We have multiple journeys and multiple sections in direct path
        for journey in response.journeys:
            for section in journey.sections:
                # do not add cycle_lane_length for bss_rent/bss_return & walking sections
                if (
                    section.type == response_pb2.STREET_NETWORK
                    and section.street_network.mode == response_pb2.Bike
                ):
                    cycle_lane_length = sum(
                        (s.length for s in section.street_network.street_information if _is_cycle_lane(s))
                    )
                    # Since path.length are doubles and we want an int32 in the proto
                    section.cycle_lane_length = int(cycle_lane_length)

        return response

    @staticmethod
    def handle_car_no_park_modes(mode):
        if mode in (FallbackModes.ridesharing.name, FallbackModes.taxi.name, FallbackModes.car_no_park.name):
            return FallbackModes.car.name
        return mode

    @staticmethod
    def is_alternatives_request(mode, direct_path_type, request):
        return (
            mode == 'bike'
            and direct_path_type == StreetNetworkPathType.DIRECT
            and request['direct_path'] == 'only_with_alternatives'
        )

    def _set_profile_param(self, profile_param, mode, language, request, dp_profile):
        profile_param.origin_mode = self.handle_car_no_park_modes(mode)
        profile_param.destination_mode = self.handle_car_no_park_modes(mode)
        profile_param.walking_speed = request['walking_speed']
        profile_param.max_walking_duration_to_pt = request['max_walking_duration_to_pt']
        profile_param.bike_speed = request['bike_speed']
        profile_param.max_bike_duration_to_pt = request['max_bike_duration_to_pt']
        profile_param.bss_speed = request['bss_speed']
        profile_param.max_bss_duration_to_pt = request['max_bss_duration_to_pt']
        profile_param.car_speed = request['car_speed']
        profile_param.max_car_duration_to_pt = request['max_car_duration_to_pt']
        profile_param.language = language
        if mode in (
            FallbackModes.ridesharing.name,
            FallbackModes.taxi.name,
            FallbackModes.car_no_park.name,
            FallbackModes.car.name,
        ):
            profile_param.car_no_park_speed = request['{}_speed'.format(mode)]
            profile_param.max_car_no_park_duration_to_pt = request['max_{}_duration_to_pt'.format(mode)]

        # In addition to the request for kraken, we add more params for asgard

        # Asgard/Valhalla bss
        for attr in ("bss_rent_duration", "bss_rent_penalty", "bss_return_duration", "bss_return_penalty"):
            setattr(profile_param, attr, request[attr])

        profile_param.enable_instructions = request['_enable_instructions']

        # Asgard/Valhalla bike
        profile_param.bike_use_roads = dp_profile.bike_use_roads
        profile_param.bike_use_hills = dp_profile.bike_use_hills
        profile_param.bike_use_ferry = dp_profile.bike_use_ferry
        profile_param.bike_avoid_bad_surfaces = dp_profile.bike_avoid_bad_surfaces
        profile_param.bike_shortest = dp_profile.bike_shortest
        profile_param.bicycle_type = type_pb2.BicycleType.Value(dp_profile.bicycle_type)
        profile_param.bike_use_living_streets = dp_profile.bike_use_living_streets
        profile_param.bike_maneuver_penalty = dp_profile.bike_maneuver_penalty
        profile_param.bike_service_penalty = dp_profile.bike_service_penalty
        profile_param.bike_service_factor = dp_profile.bike_service_factor
        profile_param.bike_country_crossing_cost = dp_profile.bike_country_crossing_cost
        profile_param.bike_country_crossing_penalty = dp_profile.bike_country_crossing_penalty

        # Asgard/Valhalla bike
        profile_param.walking_walkway_factor = dp_profile.walking_walkway_factor
        profile_param.walking_sidewalk_factor = dp_profile.walking_sidewalk_factor
        profile_param.walking_alley_factor = dp_profile.walking_alley_factor
        profile_param.walking_driveway_factor = dp_profile.walking_driveway_factor
        profile_param.walking_step_penalty = dp_profile.walking_step_penalty
        profile_param.walking_use_ferry = dp_profile.walking_use_ferry
        profile_param.walking_use_living_streets = dp_profile.walking_use_living_streets
        profile_param.walking_use_tracks = dp_profile.walking_use_tracks
        profile_param.walking_use_hills = dp_profile.walking_use_hills
        profile_param.walking_service_penalty = dp_profile.walking_service_penalty
        profile_param.walking_service_factor = dp_profile.walking_service_factor
        profile_param.walking_max_hiking_difficulty = dp_profile.walking_max_hiking_difficulty
        profile_param.walking_shortest = dp_profile.walking_shortest
        profile_param.walking_ignore_oneways = dp_profile.walking_ignore_oneways

        if dp_profile.profile_tag is not None:
            profile_param.profile_tag = dp_profile.profile_tag

        # Asgard/Valhalla walking

    def _create_direct_path_request(
        self,
        mode,
        pt_object_origin,
        pt_object_destination,
        fallback_extremity,
        request,
        direct_path_type,
        language="en-US",
    ):
        req = request_pb2.Request()
        req.requested_api = type_pb2.direct_path
        req.direct_path.origin.CopyFrom(self.make_location(pt_object_origin))
        req.direct_path.destination.CopyFrom(self.make_location(pt_object_destination))
        req.direct_path.datetime = fallback_extremity.datetime
        req.direct_path.clockwise = fallback_extremity.represents_start

        profiles = [
            DirectPathProfile(
                bike_use_roads=request['bike_use_roads'],
                bike_use_hills=request['bike_use_hills'],
                bike_use_ferry=request['bike_use_ferry'],
                bike_avoid_bad_surfaces=request['bike_avoid_bad_surfaces'],
                bike_shortest=request['bike_shortest'],
                bicycle_type=request['bicycle_type'],
                bike_use_living_streets=request['bike_use_living_streets'],
                bike_maneuver_penalty=request['bike_maneuver_penalty'],
                bike_service_penalty=request['bike_service_penalty'],
                bike_service_factor=request['bike_service_factor'],
                bike_country_crossing_cost=request['bike_country_crossing_cost'],
                bike_country_crossing_penalty=request['bike_country_crossing_penalty'],
                walking_walkway_factor=request['walking_walkway_factor'],
                walking_sidewalk_factor=request['walking_sidewalk_factor'],
                walking_alley_factor=request['walking_alley_factor'],
                walking_driveway_factor=request['walking_driveway_factor'],
                walking_step_penalty=request['walking_step_penalty'],
                walking_use_ferry=request['walking_use_ferry'],
                walking_use_living_streets=request['walking_use_living_streets'],
                walking_use_tracks=request['walking_use_tracks'],
                walking_use_hills=request['walking_use_hills'],
                walking_service_penalty=request['walking_service_penalty'],
                walking_service_factor=request['walking_service_factor'],
                walking_max_hiking_difficulty=request['walking_max_hiking_difficulty'],
                walking_shortest=request['walking_shortest'],
                walking_ignore_oneways=request['walking_ignore_oneways'],
                tag=None,
            )
        ]

        if self.is_alternatives_request(mode, direct_path_type, request):
            profiles = DIRECT_PATH_ALTERNATIVES_PROFILES

        for p in profiles:
            profile_param = req.direct_path.profiles_params.add()
            self._set_profile_param(profile_param, mode, language, request, p)

        return req

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
        # if the crowfly distance between origin and destination is too large, there is no need to call asgard
        crowfly_distance = crowfly_distance_between(
            get_pt_object_coord(pt_object_origin), get_pt_object_coord(pt_object_destination)
        )

        # if the crowfly distance between origin and destination is
        # bigger than max_{mode}_direct_path_distance don't compute direct_path
        if crowfly_distance > int(request['max_{mode}_direct_path_distance'.format(mode=mode)]):
            return response_pb2.Response()

        if (
            crowfly_distance / float(request['{mode}_speed'.format(mode=mode)])
            > request['max_{mode}_direct_path_duration'.format(mode=mode)]
        ):
            return response_pb2.Response()

        language = self.get_language_parameter(request)

        req = self._create_direct_path_request(
            mode,
            pt_object_origin,
            pt_object_destination,
            fallback_extremity,
            request,
            direct_path_type,
            language,
        )

        response = self._call_asgard(req, request_id)

        # car_no_park is interpreted as car for Asgard, we need to overwrite the streetnetwork mode here
        if mode == "car_no_park":
            try:
                response.journeys[0].sections[0].street_network.mode = response_pb2.CarNoPark
            except AttributeError:
                pass
            except Exception as e:
                raise e

        if response and mode in (FallbackModes.bike.name, FallbackModes.bss.name):
            response = self._add_cycle_lane_length(response)

        return response

    def get_uri_pt_object(self, pt_object):
        return 'coord:{c.lon}:{c.lat}'.format(c=get_pt_object_coord(pt_object))

    def _call_asgard(self, request, request_id):
        def _request():
            request.request_id = request_id
            pb = self.call(request.SerializeToString(), self.timeout)
            resp = response_pb2.Response()
            resp.ParseFromString(pb)
            return resp

        try:
            return self.breaker.call(_request)
        except pybreaker.CircuitBreakerError as e:
            self.logger.error('Asgard routing service dead (error: {})'.format(e))
            self.record_external_failure('circuit breaker open')
            raise
        except Exception as e:
            self.logger.exception('Asgard routing error')
            self.record_external_failure(str(e))
            raise
