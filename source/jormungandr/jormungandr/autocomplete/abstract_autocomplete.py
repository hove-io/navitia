# coding=utf-8

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

from __future__ import absolute_import, print_function, unicode_literals, division
from abc import abstractmethod, ABCMeta
from jormungandr.exceptions import UnknownObject, TechnicalError, log_exception


class AbstractAutocomplete(object):
    """
    abstract class managing calls autocomplete
    """
    __metaclass__ = ABCMeta

    @abstractmethod
    def get(self, query, instance):
        pass

    @abstractmethod
    def get_by_uri(self, uri, instance=None, current_datetime=None):
        """
        look for an object with its uri

        if nothing is found, raise an ObjectUnkown exception
        :return the list of objects found
        """
        pass

    @abstractmethod
    def geo_status(self, instance):
        pass

    def get_object_by_uri(self, uri, instance=None, current_datetime=None):
        """
        same as get_by_uri, but more user friendly, return the object or none if nothing was found

        Note:
            We'd like to return a None to the caller even though the autocomplete service is not available
            (it'd be a pity that a whole journey is found but jormungandr crashes because of autocomplete service)
        """
        details = None
        try:
            details = self.get_by_uri(uri, instance=instance, current_datetime=current_datetime)
        except TechnicalError as e:
            log_exception(sender=None, exception=e)
        except UnknownObject:
            pass

        if not details:
            # impossible to find the object
            return None

        return details.get('places', [None])[0]

class GeoStatusResponse(object):
    def __init__(self):
        self.street_network_sources = []
        self.poi_sources = []
        self.nb_admins = None
        self.nb_admins_from_cities = None
        self.nb_ways = None
        self.nb_addresses = None
        self.nb_pois = None
