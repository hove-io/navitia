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
from collections import defaultdict
import logging
from jormungandr.travelers_profile import default_traveler_profiles


class Scenario(default.Scenario):

    KEOLIS_MAP_TYPE = {'standard':  {'rapid': ['best'],
                                     'comfort': ['less_fallback_walk', 'less_fallback_bss'],
                                     'healthy': ['non_pt_walk', 'non_pt_bss', 'comfort', 'fastest']},
                       'slow_walker': {'rapid': ['best'],
                                       'comfort': ['less_fallback_walk'],
                                       'healthy': ['non_pt_walk', 'comfort', 'fastest']},
                       'fast_walker': {'rapid': ['best'],
                                       'comfort': ['less_fallback_walk', 'less_fallback_bss'],
                                       'healthy': ['non_pt_walk', 'non_pt_bss', 'comfort', 'fastest']},
                       'wheelchair':  {'rapid': ['best'],
                                       'comfort': ['less_fallback_walk'],
                                       'healthy': ['non_pt_walk', 'comfort', 'fastest']},
                       'luggage': {'rapid': ['best'],
                                   'comfort': ['less_fallback_walk'],
                                   'healthy': ['non_pt_walk', 'comfort', 'fastest']},
                       'cyclist': {'rapid': ['best'],
                                   'comfort': ['less_fallback_walk', 'less_fallback_bss', 'less_fallback_bike'],
                                   'healthy': ['non_pt_walk', 'non_pt_bss', 'non_pt_bike', 'comfort', 'fastest']},
                       'motorist': {'rapid': ['best'],
                                    'comfort': ['car', 'less_fallback_walk'],
                                    'healthy': ['non_pt_walk', 'comfort', 'fastest']},

                        }

    def __on_journeys(self, requested_type, request, instance):
        if request.get('traveler_type') is not None:
            request['traveler_type'] = 'standard'
            profile = default_traveler_profiles[request['traveler_type']]
            profile.override_params(request)#we override the params here if there was no traveler_type set
        else:
            profile = default_traveler_profiles[request['traveler_type']]

        resp = super(Scenario, self).__on_journeys(requested_type, request, instance)
        self._qualification(request, resp, profile)
        return resp

    def _qualification(self, request, response, profile):
        logging.getLogger(__name__).debug('qualification of journeys')
        map_type_journey = defaultdict(list)
        for journey in response.journeys:
            map_type_journey[journey.type].append(journey)
            if request['debug']:
                journey.tags.append(journey.type)
            journey.type = ''#we reset the type of all journeys

        for name, types in self.KEOLIS_MAP_TYPE[profile.traveler_type].iteritems():
            for t in types:
                journeys = map_type_journey[t]
                if journeys:
                    journeys[0].type = name
                    break

        self.delete_journeys(resp=response, request=request)


    def delete_journeys(self, resp, request, *args, **kwargs):
        """
        override the delete_journeys of the default script for keeping all the non_pt journeys
        """

        if request["debug"] or not resp:
            return #in debug we want to keep all journeys

        if "destination" not in request or not request['destination']:
            return #for isochrone we don't want to filter

        if resp.HasField("error"):
            return #we don't filter anything if errors

        #filter on journey type (the qualifier)
        to_delete = []
        tag_to_delete = ['']
        to_delete.extend([idx for idx, j in enumerate(resp.journeys) if j.type in tag_to_delete])

        # list comprehension does not work with repeated field, so we have to delete them manually
        to_delete.sort(reverse=True)
        for idx in to_delete:
            del resp.journeys[idx]
