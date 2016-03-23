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


class Property(object):
    def __init__(self):
        self.date_time = None
        self.real_time = None
        self.waiting_time = None


class Journey(object):
    def __init__(self):
        self.property = Property()
        self.line_id = None
        self.route_id = None
        self.stop_point_id = None
        self.stop_area_id = None


class XmlBuilder(object):
    def __init__(self):
        self.journeys = []

    def __get_value(self, item, xpath, val):
        value = item.find(xpath)
        if value == None:
            logging.getLogger(__name__).Debug("Path not found: {path}".format(path=xpath))
            return None
        return value.get(val)

    def __make_property(self, property, xml_journey):
        '''
        :param property: object property
        :param xml_journey: journey information
        exceptions :
            ValueError: String is not a valid ISO8601 time./
                        Unable to parse datetime, day is out of range for month (for example)
        '''
        property.date_time = date_time_format(xml_journey.get('dateTime'))
        property.real_time = (xml_journey.get('realTime') == 'yes')
        property.waiting_time = parse_time(xml_journey.get('waiting_time'))

    def __make_journey(self, xml_journey):
        journey = Journey()
        journey.route_id = xml_journey.get('routeId')
        journey.stop_point_id = self.__get_value(xml_journey, 'stop', 'id')
        journey.line_id = self.__get_value(xml_journey, 'line', 'id')
        journey.stop_area_id = self.__get_value(xml_journey, 'stopArea', 'id')
        self.__make_property(journey.property, xml_journey)
        return journey

    def build(self, xml):
        try:
            root = et.fromstring(xml)
        except et.ParseError as e:
            logging.getLogger(__name__).error("invalid xml: {}".format(e.message))
            raise
        self.journeys = [self.__make_journey(xml_journey) for xml_journey in root.findall('journey')]
