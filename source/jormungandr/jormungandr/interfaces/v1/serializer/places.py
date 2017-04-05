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

import serpy
from jormungandr.interfaces.v1.serializer.fields import *
import logging


class PropertyPathField(serpy.Field):
    def as_getter(self, serializer_field_name, serializer_cls):
        return lambda v: self.get_property(v, self.attr)

    def get_property(self, obj, path):
        properties = path.split(("."))
        for property_ in properties:
            result = obj.get(property_, {})
            obj = result

        return obj


class LiteralField(serpy.Field):
    def __init__(self, value, *args, **kwargs):
        super(LiteralField, self).__init__(*args, **kwargs)
        self.value = value

    def as_getter(self, serializer_field_name, serializer_cls):
        return lambda *args, **kwargs: self.value


def get_lon_lat(obj):
    if not obj or not obj.get('geometry') or not obj.get('geometry').get('coordinates'):
        return None, None

    coordinates = obj.get('geometry', {}).get('coordinates', [])
    if len(coordinates) == 2:
        lon = str(coordinates[0])
        lat = str(coordinates[1])
    else:
        lon = None
        lat = None
    return lon, lat


class CoordField(serpy.Field):
    def as_getter(self, serializer_field_name, serializer_cls):
        return lambda obj: self.generate_coord_field(obj)

    def generate_coord_field(self, obj):
        lon, lat = get_lon_lat(obj)
        return {
            "lat": lat,
            "lon": lon,
        }


class CoordId(serpy.Field):
    def as_getter(self, serializer_field_name, serializer_cls):
        return lambda obj: self.generate_coord_id(obj)

    def generate_coord_id(self, obj):
        if not obj:
            return None
        lon, lat = get_lon_lat(obj)
        return '{};{}'.format(lon, lat)


class IntPropertyPathField(serpy.Field):
    def as_getter(self, serializer_field_name, serializer_cls):
        return lambda v: self.get_property(v, self.attr)

    def get_property(self, obj, path):
        properties = path.split(("."))
        for property_ in properties:
            result = obj.get(property_, {})
            obj = result

        return obj

    def to_value(self, value):
        return int(value) if value else None


class SubAdministrativeRegionField(serpy.DictSerializer):
    id = serpy.Field()
    insee = serpy.Field()
    name = serpy.Field(attr="label")
    label = serpy.Field()
    level = serpy.MethodField()
    coord = serpy.MethodField()
    zip_code = serpy.MethodField()

    def get_coord(self, obj):
        coord = obj.get('coord', {})
        lat = str(coord.get('lat')) if coord and coord.get('lat') else None
        lon = str(coord.get('lon')) if coord and coord.get('lon') else None
        return {
            "lat": lat,
            "lon": lon
        }

    def get_level(self, obj):
        return int(obj.get('level')) if obj.get('level') else None

    def get_zip_code(self, obj):
        zip_codes = obj.get('zip_codes', [])
        if all(zip_code == "" for zip_code in zip_codes):
            return None
        elif len(zip_codes) == 1:
            return zip_codes[0]
        else:
            return '{}-{}'.format(min(zip_codes), max(zip_codes))


class AdministrativeRegionSerializer(serpy.DictSerializer):
    id = PropertyPathField(attr='properties.geocoding.id')
    name = PropertyPathField(attr='properties.geocoding.name')
    label = PropertyPathField(attr='properties.geocoding.label')
    zip_code = PropertyPathField(attr='properties.geocoding.postcode')
    coord = CoordField()
    insee = serpy.MethodField()
    level = IntPropertyPathField(attr='properties.geocoding.level')
    administrative_regions = serpy.MethodField()

    def get_insee(self, obj):
        geocoding = obj.get('properties', {}).get('geocoding', {})
        return geocoding.get('citycode') or geocoding.get('city_code')

    def get_administrative_regions(self, obj):
        geocoding = obj.get('properties', {}).get('geocoding', {})

        return [SubAdministrativeRegionField(r).data for r in geocoding.get('administrative_regions', [])]


class GeocodeAdminSerializer(serpy.DictSerializer):
    id = PropertyPathField(attr='properties.geocoding.id')
    name = PropertyPathField(attr='properties.geocoding.name')
    quality = LiteralField(0)
    embedded_type = LiteralField("administrative_region")
    administrative_region = serpy.MethodField()

    def get_administrative_region(self, obj):
        return AdministrativeRegionSerializer(obj).data


class PoiTypeSerializer(serpy.DictSerializer):
    id = serpy.StrField()
    name = serpy.StrField()


class PoiSerializer(serpy.DictSerializer):
    id = CoordId()
    coord = CoordField()
    label = PropertyPathField(attr='properties.geocoding.label')
    name = PropertyPathField(attr='properties.geocoding.name')
    administrative_regions = serpy.MethodField()
    poi_type = serpy.MethodField(display_none=False)

    def get_administrative_regions(self, obj):
        geocoding = obj.get('properties', {}).get('geocoding', {})

        return [SubAdministrativeRegionField(r).data for r in geocoding.get('administrative_regions', [])]

    def get_poi_type(self, obj):
        poi_types = obj.get('properties', {}).get('geocoding', {}).get('poi_types', [])
        return PoiTypeSerializer(poi_types[0]).data if isinstance(poi_types, list) and poi_types else None


class GeocodePoiSerializer(serpy.DictSerializer):
    embedded_type = LiteralField("poi")
    quality = LiteralField(0)
    id = CoordId()
    name = PropertyPathField(attr='properties.geocoding.label')
    poi = serpy.MethodField()

    def get_poi(self, obj):
        return PoiSerializer(obj).data


class AddressSerializer(serpy.DictSerializer):
    id = CoordId()
    coord = CoordField()
    house_number = serpy.MethodField()
    label = PropertyPathField(attr='properties.geocoding.label')
    name = PropertyPathField(attr='properties.geocoding.name')
    administrative_regions = serpy.MethodField()

    def get_administrative_regions(self, obj):
        geocoding = obj.get('properties', {}).get('geocoding', {})

        return [SubAdministrativeRegionField(r).data for r in geocoding.get('administrative_regions', [])]

    def get_house_number(self, obj):
        geocoding = obj.get('properties', {}).get('geocoding', {})
        hn = 0
        import re
        numbers = re.findall(r'^\d+', geocoding.get('housenumber') or "0")
        if len(numbers) > 0:
            hn = numbers[0]
        return int(hn)


class GeocodeAddressSerializer(serpy.DictSerializer):
    embedded_type = LiteralField("address")
    quality = LiteralField(0)
    id = CoordId()
    name = PropertyPathField(attr='properties.geocoding.label')
    address = serpy.MethodField()

    def get_address(self, obj):
        return AddressSerializer(obj).data


class StopAreaSerializer(serpy.DictSerializer):
    id = PropertyPathField(attr='properties.geocoding.id')
    coord = CoordField()
    label = PropertyPathField(attr='properties.geocoding.label')
    name = PropertyPathField(attr='properties.geocoding.name')
    administrative_regions = serpy.MethodField()
    timezone = LiteralField(None)

    def get_administrative_regions(self, obj):
        geocoding = obj.get('properties', {}).get('geocoding', {})

        return [SubAdministrativeRegionField(r).data for r in geocoding.get('administrative_regions', [])]

    def get_house_number(self, obj):
        geocoding = obj.get('properties', {}).get('geocoding', {})
        hn = 0
        import re
        numbers = re.findall(r'^\d+', geocoding.get('housenumber') or "0")
        if len(numbers) > 0:
            hn = numbers[0]
        return int(hn)


class GeocodeStopAreaSerializer(serpy.DictSerializer):
    embedded_type = LiteralField("stop_area")
    quality = LiteralField(0)
    id = PropertyPathField(attr='properties.geocoding.id')
    name = PropertyPathField(attr='properties.geocoding.label')
    stop_area = serpy.MethodField()

    def get_stop_area(self, obj):
        return StopAreaSerializer(obj).data


class PlacesSerializer(serpy.DictSerializer):
    places = serpy.MethodField()

    def get_places(self, obj):
        res = []
        for feature in obj.get('features', {}):
            type_ = feature.get('properties', {}).get('geocoding', {}).get('type')
            if type_ == 'city':
                res.append(GeocodeAdminSerializer(feature).data)
            elif type_ in ('street', 'house'):
                res.append(GeocodeAddressSerializer(feature).data)
            elif type_ == 'poi':
                res.append(GeocodePoiSerializer(feature).data)
            elif type_ == 'public_transport:stop_area':
                res.append(GeocodeStopAreaSerializer(feature).data)
        return res
