# Copyright (c) 2001-2016, Canal TP and/or its affiliates. All rights reserved.
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

from aniso8601.time import parse_time
import xml.etree.ElementTree as et
import logging
from jormungandr.interfaces.parsers import date_time_format
from aniso8601 import parse_time


class SynthesePassage(object):
    def __init__(self):
        self.date_time = None
        self.real_time = None
        self.waiting_time = None


class SyntheseRoutePoint(object):
    def __init__(self):
        self.syn_route_id = None
        self.syn_stop_point_id = None


class SyntheseXmlReader(object):

    def __get_value(self, item, xpath, val):
        value = item.find(xpath)
        if value == None:
            logging.getLogger(__name__).debug("Path not found: {path}".format(path=xpath))
            return None
        return value.get(val)

    def __get_synthese_passage(self, xml_journey):
        '''
        :return passage: object property
        :param xml_journey: journey information
        exceptions :
            ValueError: String is not a valid ISO8601 time./
                        Unable to parse datetime, day is out of range for month (for example)
        '''
        passage = SynthesePassage()
        passage.date_time = date_time_format(xml_journey.get('dateTime'))
        passage.real_time = (xml_journey.get('realTime') == 'yes')
        passage.waiting_time = parse_time(xml_journey.get('waiting_time'))
        return passage

    def find_route_point(self, route_id, stop_point_id, result):
        for key, value in result.items():
            if key.syn_stop_point_id == stop_point_id and key.syn_route_id == route_id:
                return key
        return None

    def __get_or_create_route_point(self, xml_journey, result):
        route_id = xml_journey.get('routeId')
        stop_point_id = self.__get_value(xml_journey, 'stop', 'id')
        route_point = self.find_route_point(route_id, stop_point_id, result)
        if not route_point:
            route_point = SyntheseRoutePoint()
            route_point.syn_route_id = route_id
            route_point.syn_stop_point_id = stop_point_id
            result[route_point] = []
        return route_point

    def __build(self, xml):
        try:
            root = et.fromstring(xml)
        except et.ParseError as e:
            logging.getLogger(__name__).error("invalid xml: {}".format(e.message))
            raise
        for xml_journey in root.findall('journey'):
            yield xml_journey

    def get_synthese_passages(self, xml):
        result = {}
        for xml_journey in self.__build(xml):
            route_point = self.__get_or_create_route_point(xml_journey, result)
            passage = self.__get_synthese_passage(xml_journey)
            result[route_point].append(passage)
        return result
