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

"""
This file contains the default values for parameters used in the journey planner.
This parameters are used on the creation of the instances in the tyr database, they will not be updated automatically.
This parameters can be used directly by jormungandr if the instance is not known in tyr, typically in development setup
"""

import logging
import sys

max_walking_duration_to_pt = 30 * 60

max_bike_duration_to_pt = 30 * 60

max_bss_duration_to_pt = 30 * 60

max_car_duration_to_pt = 30 * 60

max_car_no_park_duration_to_pt = 30 * 60

max_ridesharing_duration_to_pt = 30 * 60

max_taxi_duration_to_pt = 30 * 60

walking_speed = 1.12

bike_speed = 4.1

bss_speed = 4.1

car_speed = 11.11

car_no_park_speed = 11.11

ridesharing_speed = 6.94  # 25km/h

taxi_speed = 11.11  # same as car_speed

max_nb_transfers = 10

journey_order = 'arrival_time'

min_bike = 4 * 60

min_bss = 4 * 60 + 3 * 60  # we want 4minute on the bike, so we add the time to pick and put back the bss

min_car = 5 * 60

min_ridesharing = 10 * 60

min_taxi = 4 * 60

car_park_duration = 5 * 60

# specify the latest time point of request, relative to datetime, in second
max_duration = 60 * 60 * 24  # seconds

# the penalty on walking duration for each transfer
walking_transfer_penalty = 120  # seconds

# the penalty on arrival time for each transfer
arrival_transfer_penalty = 120  # seconds

# night bus filtering parameter
night_bus_filter_max_factor = 1.5

# night bus filtering parameter
night_bus_filter_base_factor = 15 * 60  # seconds

# priority value to be used to choose a kraken instance among valid instances. The greater it is, the more the instance
# will be chosen.
priority = 0

# activate / desactivate call to bss provider
bss_provider = True

# activate / desactivate call to car parking provider
car_park_provider = True

# Maximum number of connections allowed in journeys is calculated as
# max_additional_connections + minimum connections among the journeys
max_additional_connections = 2

# the id of physical_mode, as sent by kraken to jormungandr, used by _max_successive_physical_mode rule
successive_physical_mode_to_limit_id = 'physical_mode:Bus'

# Number of greenlets simultaneously used for Real-Time proxies calls
realtime_pool_size = 3

# Minimum number of different suggested journeys
min_nb_journeys = 0

# Maximum number of different suggested journeys
max_nb_journeys = 99999

# Maximum number of successive physical modes for an itinerary
max_successive_physical_mode = 99

# Minimum number of calls to kraken
min_journeys_calls = 1

# Filter on vj using same lines and same stops
final_line_filter = False

# Maximum number of second pass to get more itineraries
max_extra_second_pass = 0

max_nb_crowfly_by_mode = {
    'walking': 5000,
    'car': 5000,
    'car_no_park': 5000,
    'bike': 5000,
    'bss': 5000,
    'taxi': 5000,
}

autocomplete_backend = 'kraken'

# Additionnal time in second after the taxi section when used as first section mode
additional_time_after_first_section_taxi = 5 * 60

# Additionnal time in second before the taxi section when used as last section mode
additional_time_before_last_section_taxi = 5 * 60

max_walking_direct_path_duration = 24 * 60 * 60

max_bike_direct_path_duration = 24 * 60 * 60

max_bss_direct_path_duration = 24 * 60 * 60

max_car_direct_path_duration = 24 * 60 * 60

max_car_no_park_direct_path_duration = 24 * 60 * 60

max_taxi_direct_path_duration = 24 * 60 * 60

max_ridesharing_direct_path_duration = 24 * 60 * 60

street_network_car = "kraken"
street_network_car_no_park = "kraken"
street_network_walking = "kraken"
street_network_bike = "kraken"
street_network_bss = "kraken"
street_network_ridesharing = "ridesharingKraken"
street_network_taxi = "taxiKraken"

# Here - https://developer.here.com/
here_max_matrix_points = 100

stop_points_nearby_duration = 5 * 60  # In secs

asynchronous_ridesharing = False

greenlet_pool_for_ridesharing_services = False

ridesharing_greenlet_pool_size = 10

external_service = 'free_floatings'

# max_waiting_duration default value 4h: 4*60*60 = 14400 secondes
max_waiting_duration = 4 * 60 * 60

# Bss parameters
bss_rent_duration = 2 * 60
bss_rent_penalty = 0
bss_return_duration = 1 * 60
bss_return_penalty = 0

# max_{mode}_direct_path_distance default
max_bike_direct_path_distance = 50000  # 50 Km
max_bss_direct_path_distance = 50000  # 50 Km
max_walking_direct_path_distance = 50000  # 50 Km
max_car_direct_path_distance = 200000  # 200 Km
max_car_no_park_direct_path_distance = 200000  # 200 Km
max_ridesharing_direct_path_distance = 200000  # 200 Km
max_taxi_direct_path_distance = 200000  # 200 Km

# Proximity radius for autocomplete used in /places endpoint
places_proximity_radius = 20000  # 20 Km

# Asgard language by default
asgard_language = "english_us"

# Compute pathways using the street_network engine for transfers between surface physical modes
transfer_path = False

# use/disuse access points in journey computations
access_points = False

default_pt_planner = 'kraken'

# {
#     "kraken": {
#         "class": "jormungandr.pt_planners.kraken.Kraken",
#         "args": {
#             "zmq_socket": "ipc:///tmp/dummy_kraken"
#         }
#     },
#     "loki": {
#         "class": "jormungandr.pt_planners.loki.Loki",
#         "args": {
#             "zmq_socket": "ipc:///tmp/dummy_kraken"
#         }
#     }
# }
pt_planners_configurations = dict()  # type: dict


def get_value_or_default(attr, instance, instance_name):
    if not instance or getattr(instance, attr, None) == None:
        value = getattr(sys.modules[__name__], attr)
        if not instance:
            logger = logging.getLogger(__name__)
            logger.warning(
                'instance %s not found in db, we use the default value (%s) for the param %s',
                instance_name,
                value,
                attr,
            )
        return value
    else:
        return getattr(instance, attr)
