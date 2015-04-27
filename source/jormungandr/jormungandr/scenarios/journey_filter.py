# Copyright (c) 2001-2015, Canal TP and/or its affiliates. All rights reserved.
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
import logging
from jormungandr.scenarios.utils import compare

def journeys_gen(list_responses):
    for r in list_responses:
        for j in r.journeys:
            yield j

def similar_journeys_generator(journey):
    for s in journey.sections:
        yield s.pt_display_informations.uris.vehicle_journey


def filter_journeys(response_list, args):
    """
    Filter by side effect the list of pb responses's journeys

    first draft, we only remove the journeys with the same vjs
    """
    if args['debug']:
        return response_list

    logger = logging.getLogger(__name__)
    to_delete = []

    for response_idx, r in enumerate(response_list):
        for j_idx, j in enumerate(r.journeys):

            # for other_response in range(response_idx + 1, len(response_list)):
            #     for other_j2 in range(response_idx + 1, len(response_list)):

            if any(compare(j, other, similar_journeys_generator)
                   for other in journeys_gen(response_list)
                   if other != j and (response_idx, j_idx) not in to_delete):
                to_delete.append((response_idx, j_idx))

    logger.info('filtering {} journeys because they are dupplicates ({})'.format(len(to_delete), to_delete))
    for response_idx, j_idx in reversed(to_delete):
        del response_list[response_idx].journeys[j_idx]

    return response_list