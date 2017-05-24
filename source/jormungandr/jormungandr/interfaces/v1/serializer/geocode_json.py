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
from base import LiteralField, NestedPropertyField, IntNestedPropertyField, value_by_path
from flask.ext.restful import abort
from jormungandr.interfaces.v1.serializer import jsonschema


class CoordField(jsonschema.Field):
    def __init__(self, schema_type=None, schema_metadata={}, **kwargs):
        schema_metadata.update({
            "type": "object",
            "properties": {
                "lat": { "type": ["string", "null"] },
                "lon": { "type": ["string", "null"] }
            },
            "required": ["lat", "lon"]
        })
        super(CoordField, self).__init__(schema_type, schema_metadata, **kwargs)

    def as_getter(self, serializer_field_name, serializer_cls):
        return lambda obj: self.generate_coord_field(obj)

    def generate_coord_field(self, obj):
        coords = value_by_path(obj, 'geometry.coordinates')
        res = {'lat': None, 'lon': None}
        if coords and len(coords) >= 2:
            res.update({'lat': str(coords[1]), 'lon': str(coords[0])})
        return res


class CoordId(jsonschema.Field):
    def __init__(self, schema_type=None, schema_metadata={}, **kwargs):
        schema_metadata.update({
            "type": ["string", "null"]
        })
        super(CoordId, self).__init__(schema_type, schema_metadata, **kwargs)

    def as_getter(self, serializer_field_name, serializer_cls):
        return lambda obj: self.generate_coord_id(obj)

    def generate_coord_id(self, obj):
        coords = value_by_path(obj, 'geometry.coordinates')
        if coords and len(coords) >= 2:
            return '{};{}'.format(coords[0], coords[1])
        return None


class SubAdministrativeRegionField(serpy.DictSerializer):
    id = jsonschema.Field(schema_type=str)
    insee = jsonschema.Field(schema_type=str)
    name = jsonschema.Field(schema_type=str, attr="label")
    label = jsonschema.Field(schema_type=str)
    level = serpy.IntField()
    coord = jsonschema.MethodField()
    zip_code = jsonschema.MethodField()

    def get_coord(self, obj):
        lat = value_by_path(obj, 'coord.lat')
        lon = value_by_path(obj, 'coord.lon')
        res = {'lat': None, 'lon': None}
        if lon and lat:
            res.update({'lat': str(lat), 'lon': str(lon)})
        return res

    def get_zip_code(self, obj):
        zip_codes = obj.get('zip_codes', [])
        if all(zip_code == "" for zip_code in zip_codes):
            return None
        elif len(zip_codes) == 1:
            return zip_codes[0]
        else:
            return '{}-{}'.format(min(zip_codes), max(zip_codes))


class AdministrativeRegionSerializer(serpy.DictSerializer):
    id = NestedPropertyField(attr='properties.geocoding.id')
    name = NestedPropertyField(attr='properties.geocoding.name')
    label = NestedPropertyField(attr='properties.geocoding.label')
    zip_code = NestedPropertyField(attr='properties.geocoding.postcode')
    coord = CoordField()
    insee = NestedPropertyField(attr='properties.geocoding.citycode')
    level = IntNestedPropertyField(attr='properties.geocoding.level')
    administrative_regions = jsonschema.MethodField()

    def get_administrative_regions(self, obj):
        geocoding = obj.get('properties', {}).get('geocoding', {})

        return [SubAdministrativeRegionField(r).data for r in geocoding.get('administrative_regions', [])]


class GeocodeAdminSerializer(serpy.DictSerializer):
    id = NestedPropertyField(attr='properties.geocoding.id')
    name = NestedPropertyField(attr='properties.geocoding.name')
    quality = LiteralField(0)
    embedded_type = LiteralField("administrative_region")
    administrative_region = jsonschema.MethodField()

    def get_administrative_region(self, obj):
        return AdministrativeRegionSerializer(obj).data


class PoiTypeSerializer(serpy.DictSerializer):
    id = serpy.StrField()
    name = serpy.StrField()


class PoiSerializer(serpy.DictSerializer):
    id = NestedPropertyField(attr='properties.geocoding.id')
    coord = CoordField()
    label = NestedPropertyField(attr='properties.geocoding.label')
    name = NestedPropertyField(attr='properties.geocoding.name')
    administrative_regions = jsonschema.MethodField()
    poi_type = jsonschema.MethodField(display_none=False)

    def get_administrative_regions(self, obj):
        geocoding = obj.get('properties', {}).get('geocoding', {})

        return [SubAdministrativeRegionField(r).data for r in geocoding.get('administrative_regions', [])]

    def get_poi_type(self, obj):
        poi_types = obj.get('properties', {}).get('geocoding', {}).get('poi_types', [])
        return PoiTypeSerializer(poi_types[0]).data if isinstance(poi_types, list) and poi_types else None


class GeocodePoiSerializer(serpy.DictSerializer):
    embedded_type = LiteralField("poi")
    quality = LiteralField(0)
    id = NestedPropertyField(attr='properties.geocoding.id')
    name = NestedPropertyField(attr='properties.geocoding.label')
    poi = jsonschema.MethodField()

    def get_poi(self, obj):
        return PoiSerializer(obj).data


class AddressSerializer(serpy.DictSerializer):
    id = CoordId()
    coord = CoordField()
    house_number = jsonschema.MethodField()
    label = NestedPropertyField(attr='properties.geocoding.label')
    name = NestedPropertyField(attr='properties.geocoding.name')
    administrative_regions = jsonschema.MethodField()

    def get_house_number(self, obj):
        geocoding = obj.get('properties', {}).get('geocoding', {})
        hn = 0
        import re
        numbers = re.findall(r'^\d+', geocoding.get('housenumber') or "0")
        if len(numbers) > 0:
            hn = numbers[0]
        return int(hn)

    def get_administrative_regions(self, obj):
        geocoding = obj.get('properties', {}).get('geocoding', {})

        return [SubAdministrativeRegionField(r).data for r in geocoding.get('administrative_regions', [])]


class GeocodeAddressSerializer(serpy.DictSerializer):
    embedded_type = LiteralField("address")
    quality = LiteralField(0)
    id = CoordId()
    name = NestedPropertyField(attr='properties.geocoding.label')
    address = jsonschema.MethodField()

    def get_address(self, obj):
        return AddressSerializer(obj).data


class StopAreaSerializer(serpy.DictSerializer):
    id = NestedPropertyField(attr='properties.geocoding.id')
    coord = CoordField()
    label = NestedPropertyField(attr='properties.geocoding.label')
    name = NestedPropertyField(attr='properties.geocoding.name')
    administrative_regions = jsonschema.MethodField()
    timezone = LiteralField(None)

    def get_administrative_regions(self, obj):
        geocoding = obj.get('properties', {}).get('geocoding', {})

        return [SubAdministrativeRegionField(r).data for r in geocoding.get('administrative_regions', [])]


class GeocodeStopAreaSerializer(serpy.DictSerializer):
    embedded_type = LiteralField("stop_area")
    quality = LiteralField(0)
    id = NestedPropertyField(attr='properties.geocoding.id')
    name = NestedPropertyField(attr='properties.geocoding.label')
    stop_area = jsonschema.MethodField()

    def get_stop_area(self, obj):
        return StopAreaSerializer(obj).data


class WarningSerializer(serpy.DictSerializer):
    id = LiteralField("beta_endpoint")
    message = LiteralField("This service is under construction. You can help through github.com/CanalTP/navitia")


class GeocodePlacesSerializer(serpy.DictSerializer):
    places = jsonschema.MethodField()
    warnings = LiteralField([{
        'id': 'beta_endpoint',
        'message': 'This service is under construction. You can help through github.com/CanalTP/navitia'
    }])

    def get_places(self, obj):
        map_serializer = {
            'city': GeocodeAdminSerializer,
            'street': GeocodeAddressSerializer,
            'house': GeocodeAddressSerializer,
            'poi': GeocodePoiSerializer,
            'public_transport:stop_area': GeocodeStopAreaSerializer
        }
        res = []
        for feature in obj.get('features', {}):
            type_ = feature.get('properties', {}).get('geocoding', {}).get('type')
            if not type_ or type_ not in map_serializer:
                abort(404, message='Unknown places type {}'.format(type_))
            res.append(map_serializer[type_](feature).data)
        return res
