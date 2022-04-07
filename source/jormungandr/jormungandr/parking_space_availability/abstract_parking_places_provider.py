# Copyright (c) 2001-2014, Hove and/or its affiliates. All rights reserved.
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
from __future__ import absolute_import, print_function, unicode_literals, division
from abc import abstractmethod, ABCMeta
import six
import logging
import shapely


class AbstractParkingPlacesProvider(six.with_metaclass(ABCMeta, object)):
    """
    abstract class managing calls to external service providing real-time parking places info
    """

    def __init__(self, **kwargs):
        self.boundary_shape = None
        self.log = logging.getLogger(__name__)
        self._init_boundary_shape(kwargs.get('geometry'))

    '''
    Initialise the object's shape using a GeoJson object.

    boundary_geometry : a dictionary representing a GeoJSON object. see https://geojson.org/
    '''

    def _init_boundary_shape(self, boundary_geometry):
        if boundary_geometry:
            try:
                boundary_shape = shapely.geometry.shape(boundary_geometry)
                if not boundary_shape.is_valid:
                    raise Exception("Geometry shape is invalid")
                self.boundary_shape = boundary_shape
            except Exception as e:
                self.log.error('Error while loading boundary shape :', str(e))
                self.log.error("Unable to parse geometry object : ", boundary_geometry)

    def has_boundary_shape(self):  # type () : bool
        return hasattr(self, 'boundary_shape') and self.boundary_shape != None

    def is_poi_coords_within_shape(self, poi):
        if self.has_boundary_shape() == False:
            '''
            We assume that a POI is within a provider with no shape to be backward compatible.
            So that we don't have to configure a shape for every single provider.
            '''
            return True

        try:
            coord = poi['coord']
            lon = float(coord['lon'])
            lat = float(coord['lat'])
            return self.boundary_shape.contains(shapely.geometry.Point([lon, lat]))
        except KeyError as e:
            self.log.error(
                "Coords illformed, 'poi' needs a coord dict with 'lon' and 'lat' attributes': ", str(e)
            )
        except ValueError as e:
            self.log.error("Cannot convert POI's coord to float : ", str(e))
        except Exception as e:
            self.log.error("Cannot find if coordinate is within shape: ", str(e))

        return False

    def handle_poi(self, poi):
        return self.support_poi(poi) and self.is_poi_coords_within_shape(poi)

    @abstractmethod
    def support_poi(self, poi):
        pass

    @abstractmethod
    def get_informations(self, poi):
        pass

    @abstractmethod
    def status(self):
        pass

    @abstractmethod
    def feed_publisher(self):
        pass
