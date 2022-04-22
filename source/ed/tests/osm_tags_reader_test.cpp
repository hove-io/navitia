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

#include "ed/data.h"
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_ed
#include <boost/test/unit_test.hpp>
#include <boost/property_tree/exceptions.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "conf.h"
#include "ed/build_helper.h"
#include "ed/connectors/osm_tags_reader.h"
#include "ed/default_poi_types.h"
#include "tests/utils_test.h"

struct logger_initialized {
    logger_initialized() { navitia::init_logger(); }
};
BOOST_GLOBAL_FIXTURE(logger_initialized);

/*
 * default poi-types and minimal json should be ok
 */
BOOST_AUTO_TEST_CASE(def_json_poi_types_ok) {
    auto default_json = ed::connectors::DEFAULT_JSON_POI_TYPES;
    BOOST_CHECK_NO_THROW(const ed::connectors::PoiTypeParams params(default_json));
    auto minimal_json = R"(
        {
          "poi_types": [
            {"id": "amenity:bicycle_rental", "name": "Station VLS"},
            {"id": "amenity:parking", "name": "Parking"}
          ],
          "rules": []
        })";
    BOOST_CHECK_NO_THROW(const ed::connectors::PoiTypeParams params(minimal_json));
}

/*
 * poi-type parking and bss-rental are mandatory (used by kraken)
 */
BOOST_AUTO_TEST_CASE(no_parking_json_poi_types_ko) {
    auto no_bss_rental_json = R"(
        {
          "poi_types": [
            {"id": "amenity:parking", "name": "Parking"}
          ],
          "rules": []
        })";
    BOOST_CHECK_THROW(const ed::connectors::PoiTypeParams params(no_bss_rental_json), std::invalid_argument);
    auto no_parking_json = R"(
        {
          "poi_types": [
            {"id": "amenity:bicycle_rental", "name": "Station VLS"}
          ],
          "rules": []
        })";
    BOOST_CHECK_THROW(const ed::connectors::PoiTypeParams params(no_parking_json), std::invalid_argument);
}

/*
 * poi-type id can't be defined twice
 */
BOOST_AUTO_TEST_CASE(double_poi_types_ko) {
    auto json = R"(
        {
          "poi_types": [
            {"id": "amenity:bicycle_rental", "name": "Station VLS"},
            {"id": "amenity:bicycle_rental", "name": "BSS station"},
            {"id": "amenity:parking", "name": "Parking"}
          ],
          "rules": []
        })";
    BOOST_CHECK_THROW(const ed::connectors::PoiTypeParams params(json), std::invalid_argument);
}

/*
 * can't use a poi-type id that's not defined
 */
BOOST_AUTO_TEST_CASE(undefined_poi_type_ko) {
    auto json = R"(
        {
          "poi_types": [
            {"id": "amenity:bicycle_rental", "name": "Station VLS"},
            {"id": "amenity:parking", "name": "Parking"}
          ],
          "rules": [
            {
              "osm_tags_filters": [
                {"key": "leisure", "value": "park"}
              ],
              "poi_type_id": "leisure:park"
            }
          ]
        })";
    BOOST_CHECK_THROW(const ed::connectors::PoiTypeParams params(json), std::invalid_argument);
}

/*
 * can't provide an empty json, or an ill-formed json
 */
BOOST_AUTO_TEST_CASE(ill_json_poi_type_ko) {
    auto json = R"({})";
    BOOST_CHECK_THROW(const ed::connectors::PoiTypeParams params(json), boost::property_tree::ptree_bad_path);
    auto ill_json = R"(
        {
          "poi_types":
            {"id": "amenity:bicycle_rental", "name": "Station VLS"},
            {"id": "amenity:parking", "name": "Parking"}
          "rules": [
            {
              non-sense!!!!!
          ]
        })";
    BOOST_CHECK_THROW(const ed::connectors::PoiTypeParams params(ill_json),
                      boost::property_tree::json_parser::json_parser_error);
}

/*
 * default tagging: check that default parameters chooses the good tag
 */
BOOST_AUTO_TEST_CASE(default_tagging) {
    auto default_json = ed::connectors::DEFAULT_JSON_POI_TYPES;
    const ed::connectors::PoiTypeParams params(default_json);

    BOOST_CHECK(params.get_applicable_poi_rule({}) == nullptr);
    BOOST_CHECK_EQUAL(params.get_applicable_poi_rule({{"amenity", "parking"}})->poi_type_id, "amenity:parking");
    BOOST_CHECK_EQUAL(params.get_applicable_poi_rule({{"leisure", "garden"}})->poi_type_id, "leisure:garden");
}

/*
 * multiple matches tagging: check that first matches
 */
BOOST_AUTO_TEST_CASE(multiple_match_tagging) {
    auto velib_prior_json = R"(
        {
          "poi_types": [
            {"id": "amenity:bicycle_rental", "name": "Station VLS"},
            {"id": "velib", "name": "Velib Station"},
            {"id": "amenity:parking", "name": "Parking"}
          ],
          "rules": [
            {
              "osm_tags_filters": [
                {"key": "amenity", "value": "bicycle_rental"},
                {"key": "bss_type", "value": "velib"}
              ],
              "poi_type_id": "velib"
            },
            {
              "osm_tags_filters": [
                {"key": "amenity", "value": "bicycle_rental"}
              ],
              "poi_type_id": "amenity:bicycle_rental"
            }
          ]
        })";
    const ed::connectors::PoiTypeParams velib_prior_params(velib_prior_json);
    auto bss_prior_json = R"(
        {
          "poi_types": [
            {"id": "amenity:bicycle_rental", "name": "Station VLS"},
            {"id": "velib", "name": "Velib Station"},
            {"id": "amenity:parking", "name": "Parking"}
          ],
          "rules": [
            {
              "osm_tags_filters": [
                {"key": "amenity", "value": "bicycle_rental"}
              ],
              "poi_type_id": "amenity:bicycle_rental"
            },
            {
              "osm_tags_filters": [
                {"key": "amenity", "value": "bicycle_rental"},
                {"key": "bss_type", "value": "velib"}
              ],
              "poi_type_id": "velib"
            }
          ]
        })";
    const ed::connectors::PoiTypeParams bss_prior_params(bss_prior_json);

    Hove::Tags velib_tags = {{"amenity", "bicycle_rental"}, {"bss_type", "velib"}};
    Hove::Tags bss_tags = {{"amenity", "bicycle_rental"}, {"bss_type", "unknown"}};

    BOOST_CHECK_EQUAL(velib_prior_params.get_applicable_poi_rule(velib_tags)->poi_type_id, "velib");
    BOOST_CHECK_EQUAL(bss_prior_params.get_applicable_poi_rule(velib_tags)->poi_type_id, "amenity:bicycle_rental");
    BOOST_CHECK_EQUAL(velib_prior_params.get_applicable_poi_rule(bss_tags)->poi_type_id, "amenity:bicycle_rental");
    BOOST_CHECK_EQUAL(bss_prior_params.get_applicable_poi_rule(bss_tags)->poi_type_id, "amenity:bicycle_rental");
}

/*
 * `:` bug test (we can use an osm-tag key or osm-tag value that contains a colon)
 */
BOOST_AUTO_TEST_CASE(colon_tagging) {
    auto colon_json = R"(
        {
          "poi_types": [
            {"id": "amenity:bicycle_rental", "name": "Station VLS"},
            {"id": "amenity:parking", "name": "Parking"}
          ],
          "rules": [
            {
              "osm_tags_filters": [
                {"key": "amenity:bicycle_rental", "value": "true"}
              ],
              "poi_type_id": "amenity:bicycle_rental"
            },
            {
              "osm_tags_filters": [
                {"key": "amenity", "value": "parking:effia"}
              ],
              "poi_type_id": "amenity:parking"
            }
          ]
        })";
    const ed::connectors::PoiTypeParams colon_params(colon_json);

    Hove::Tags bss_tags = {{"amenity:bicycle_rental", "true"}};
    Hove::Tags effia_tags = {{"amenity", "parking:effia"}};

    BOOST_CHECK_EQUAL(colon_params.get_applicable_poi_rule(bss_tags)->poi_type_id, "amenity:bicycle_rental");
    BOOST_CHECK_EQUAL(colon_params.get_applicable_poi_rule(effia_tags)->poi_type_id, "amenity:parking");
}

BOOST_AUTO_TEST_CASE(highway_trunk_not_visible) {
    std::bitset<8> res = ed::connectors::parse_way_tags({{"highway", "trunk"}});
    BOOST_CHECK_EQUAL(res[ed::connectors::VISIBLE], false);
}

BOOST_AUTO_TEST_CASE(highway_trunk_link_not_visible) {
    std::bitset<8> res = ed::connectors::parse_way_tags({{"highway", "trunk_link"}});
    BOOST_CHECK_EQUAL(res[ed::connectors::VISIBLE], false);
}

BOOST_AUTO_TEST_CASE(highway_motorway_not_visible) {
    std::bitset<8> res = ed::connectors::parse_way_tags({{"highway", "motorway"}});
    BOOST_CHECK_EQUAL(res[ed::connectors::VISIBLE], false);
}

BOOST_AUTO_TEST_CASE(highway_motorway_link_not_visible) {
    std::bitset<8> res = ed::connectors::parse_way_tags({{"highway", "motorway_link"}});
    BOOST_CHECK_EQUAL(res[ed::connectors::VISIBLE], false);
}

BOOST_AUTO_TEST_CASE(highway_cycleway_visible) {
    std::bitset<8> res = ed::connectors::parse_way_tags({{"highway", "cycleway"}});
    BOOST_CHECK_EQUAL(res[ed::connectors::VISIBLE], true);
}

BOOST_AUTO_TEST_CASE(get_postal_code_from_tags_test) {
    BOOST_CHECK_EQUAL(ed::connectors::get_postal_code_from_tags({{"tag1", "plop"}, {"tag2", "plopi"}}), std::string());

    BOOST_CHECK_EQUAL(ed::connectors::get_postal_code_from_tags({{"tag1", "plop"}, {"addr:postcode", "42"}}),
                      std::string("42"));

    BOOST_CHECK_EQUAL(ed::connectors::get_postal_code_from_tags({{"tag1", "plop"}, {"postal_code", "67"}}),
                      std::string("67"));

    BOOST_CHECK_EQUAL(ed::connectors::get_postal_code_from_tags({{"addr:postcode", "23"}, {"postal_code", "168"}}),
                      std::string("23"));
}
