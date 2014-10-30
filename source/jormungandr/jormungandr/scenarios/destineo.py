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
from jormungandr.scenarios import default
import copy
import logging


class Scenario(default.Scenario):

    def journeys(self, request, instance):
        logger = logging.getLogger(__name__)
        request_tc = copy.deepcopy(request)
        request_tc['origin_mode'] = ['walking']
        request_tc['destination_mode'] = ['walking']

        request_alternative = copy.deepcopy(request)
        request_alternative['min_nb_journeys'] = None
        request_alternative['max_nb_journeys'] = None
        logger.debug('journeys only on TC')
        response_tc = super(Scenario, self).journeys(request_tc, instance)

        logger.debug('journeys with alternative mode')
        response_alternative = super(Scenario, self).journeys(request_alternative, instance)

        logger.debug('merge reponses')
        #we don't want to have a "best" alternative journey
        for journey in response_alternative.journeys:
            journey.tags.append('alternative')
            if journey.type == 'best':
                journey.type = 'rapid'

        self.merge_response(response_tc, response_alternative)
        return response_tc

