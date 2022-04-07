/* Copyright © 2001-2022, Hove and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Hove (www.hove.com).
Help us simplify mobility and open public transport:
    a non ending quest to the responsive locomotion way of traveling!

LICENCE: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.

Stay tuned using
twitter @navitia
channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#pragma once

namespace ed {
namespace connectors {

static const std::string DEFAULT_JSON_POI_TYPES = R"(
{
  "poi_types": [
    {"id": "amenity:college", "name": "École"},
    {"id": "amenity:university", "name": "Université"},
    {"id": "amenity:theatre", "name": "Théâtre"},
    {"id": "amenity:hospital", "name": "Hôpital"},
    {"id": "amenity:post_office", "name": "Bureau de poste"},
    {"id": "amenity:bicycle_rental", "name": "Station VLS"},
    {"id": "amenity:bicycle_parking", "name": "Parking vélo"},
    {"id": "amenity:parking", "name": "Parking"},
    {"id": "amenity:police", "name": "Police, gendarmerie"},
    {"id": "amenity:townhall", "name": "Mairie"},
    {"id": "leisure:garden", "name": "Jardin"},
    {"id": "leisure:park", "name": "Parc, espace vert"}
  ],
  "rules": [
    {
      "osm_tags_filters": [
        {"key": "amenity", "value": "college"}
      ],
      "poi_type_id": "amenity:college"
    },
    {
      "osm_tags_filters": [
        {"key": "amenity", "value": "university"}
      ],
      "poi_type_id": "amenity:university"
    },
    {
      "osm_tags_filters": [
        {"key": "amenity", "value": "theatre"}
      ],
      "poi_type_id": "amenity:theatre"
    },
    {
      "osm_tags_filters": [
        {"key": "amenity", "value": "hospital"}
      ],
      "poi_type_id": "amenity:hospital"
    },
    {
      "osm_tags_filters": [
        {"key": "amenity", "value": "post_office"}
      ],
      "poi_type_id": "amenity:post_office"
    },
    {
      "osm_tags_filters": [
        {"key": "amenity", "value": "bicycle_rental"}
      ],
      "poi_type_id": "amenity:bicycle_rental"
    },
    {
      "osm_tags_filters": [
        {"key": "amenity", "value": "bicycle_parking"}
      ],
      "poi_type_id": "amenity:bicycle_parking"
    },
    {
      "osm_tags_filters": [
        {"key": "amenity", "value": "parking"}
      ],
      "poi_type_id": "amenity:parking"
    },
    {
      "osm_tags_filters": [
        {"key": "amenity", "value": "police"}
      ],
      "poi_type_id": "amenity:police"
    },
    {
      "osm_tags_filters": [
        {"key": "amenity", "value": "townhall"}
      ],
      "poi_type_id": "amenity:townhall"
    },
    {
      "osm_tags_filters": [
        {"key": "leisure", "value": "garden"}
      ],
      "poi_type_id": "leisure:garden"
    },
    {
      "osm_tags_filters": [
        {"key": "leisure", "value": "park"}
      ],
      "poi_type_id": "leisure:park"
    }
  ]
}
)";
}
}  // namespace ed
