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
from jormungandr.scenarios import new_default
import logging
import copy


class Scenario(new_default.Scenario):

    def journeys(self, request, instance):
        logger = logging.getLogger(__name__)
        logger.debug('journeys with STIF scenario')

        original_request = copy.deepcopy(request)
        # set special STIF values:
        # - call one next journey
        # - filter on vj using same lines and same stops
        # - no minimum number of journeys (final filter would delete identical vjs on different times)
        if original_request['_min_journeys_calls'] is None:
            original_request['_min_journeys_calls'] = 2
        if original_request['_final_line_filter'] is None:
            original_request['_final_line_filter'] = True
        if original_request['min_nb_journeys'] is None:
            original_request['min_nb_journeys'] = 1

        original_response = super(Scenario, self).journeys(original_request, instance)

        return original_response
