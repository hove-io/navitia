/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
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

#include "osm_tags_reader.h"
#include "utils/functions.h"
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <boost/property_tree/json_parser.hpp>
#include "utils/exception.h"
#include "speed_parser.h"

namespace ed {
namespace connectors {

static void update_if_unknown(int& target, const int& source) {
    if (target == -1) {
        target = source;
    }
}

boost::optional<float> parse_way_speed(const std::map<std::string, std::string>& tags,
                                       const SpeedsParser& default_speed) {
    if (!tags.count("highway")) {
        return boost::none;
    }
    boost::optional<std::string> max_speed;
    std::string highway;
    for (std::pair<std::string, std::string> pair : tags) {
        std::string key = pair.first, val = pair.second;
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        std::transform(val.begin(), val.end(), val.begin(), ::tolower);

        if (key == "maxspeed") {
            max_speed = val;
        }

        if (key == "highway") {
            highway = val;
        }
    }

    return default_speed.get_speed(highway, max_speed);
}

std::bitset<8> parse_way_tags(const std::map<std::string, std::string>& tags) {
    // if no highway tag, we can do nothing on it
    if (!tags.count("highway")) {
        return {};
    }

    constexpr int unknown = -1;
    constexpr int foot_forbidden = 0;
    constexpr int foot_allowed = 1;

    constexpr int car_forbidden = 0;
    constexpr int car_residential = 1;
    constexpr int car_tertiary = 2;
    constexpr int car_secondary = 3;
    constexpr int car_primary = 4;
    constexpr int car_trunk = 5;
    constexpr int car_motorway = 6;

    constexpr int bike_forbidden = 0;
    constexpr int bike_allowed = 2;
    constexpr int bike_lane = 3;
    constexpr int bike_busway = 4;
    constexpr int bike_track = 5;

    int car_direct = unknown;
    int car_reverse = unknown;
    int bike_direct = unknown;
    int bike_reverse = unknown;
    int foot = unknown;
    bool visible = true;

    auto set_visible_false = [&]() { visible = false; };

    auto update_foot_allowed = [&]() { update_if_unknown(foot, foot_allowed); };
    auto set_foot_allowed = [&]() { foot = foot_allowed; };
    auto set_foot_forbidden = [&]() { foot = foot_forbidden; };

    auto update_bike_direct_track = [&]() { update_if_unknown(bike_direct, bike_track); };
    auto update_bike_direct_busway = [&]() { update_if_unknown(bike_direct, bike_busway); };
    auto update_bike_reverse_busway = [&]() { update_if_unknown(bike_reverse, bike_busway); };
    auto update_bike_reverse_forbidden = [&]() { update_if_unknown(bike_reverse, bike_forbidden); };
    auto set_bike_reverse_allowed = [&] { bike_reverse = bike_allowed; };
    auto set_bike_reverse_track = [&] { bike_reverse = bike_track; };
    auto set_bike_reverse_lane = [&] { bike_reverse = bike_lane; };
    auto set_bike_direct_allowed = [&]() { bike_direct = bike_allowed; };
    auto set_bike_direct_forbidden = [&]() { bike_direct = bike_forbidden; };
    auto set_bike_direct_track = [&] { bike_direct = bike_track; };
    auto set_bike_direct_lane = [&]() { bike_direct = bike_lane; };
    auto set_bike_direct_busway = [&]() { bike_direct = bike_busway; };

    auto update_car_direct_primary = [&]() {
        update_if_unknown(car_direct, car_primary);
        update_foot_allowed();
        update_if_unknown(bike_direct, bike_allowed);
    };
    auto update_car_direct_secondary = [&]() {
        update_if_unknown(car_direct, car_secondary);
        update_foot_allowed();
        update_if_unknown(bike_direct, bike_allowed);
    };
    auto update_car_direct_tertiary = [&]() {
        update_if_unknown(car_direct, car_tertiary);
        update_foot_allowed();
        update_if_unknown(bike_direct, bike_allowed);
    };
    auto update_car_direct_residentiary = [&]() {
        update_if_unknown(car_direct, car_residential);
        update_foot_allowed();
        update_if_unknown(bike_direct, bike_allowed);
    };
    auto update_car_direct_motorway = [&]() {
        update_if_unknown(car_direct, car_motorway);
        update_if_unknown(foot, foot_forbidden);
        update_if_unknown(bike_direct, bike_forbidden);
    };
    auto update_car_direct_trunk = [&]() {
        update_if_unknown(car_direct, car_trunk);
        update_if_unknown(foot, foot_forbidden);
        update_if_unknown(bike_direct, bike_forbidden);
    };
    auto set_car_reverse_forbidden = [&]() { car_reverse = car_forbidden; };

    for (std::pair<std::string, std::string> pair : tags) {
        std::unordered_map<std::string, std::unordered_map<std::string, std::function<void()>>> key_map = {
            {"highway",
             {
                 {"footway", update_foot_allowed},
                 {"pedestrian", update_foot_allowed},
                 {"cycleway",
                  [&]() {
                      update_bike_direct_track();
                      update_foot_allowed();
                  }},
                 {"path",
                  [&]() {
                      // http://www.cyclestreets.net/journey/help/osmconversion/#toc6
                      // highway = path => might mean lots of different things, so we allow bike and foot
                      update_bike_direct_track();
                      update_foot_allowed();
                  }},
                 {"steps", update_foot_allowed},
                 {"primary", update_car_direct_primary},
                 {"primary_link", update_car_direct_primary},
                 {"secondary", update_car_direct_secondary},
                 {"secondary_link", update_car_direct_secondary},
                 {"tertiary", update_car_direct_tertiary},
                 {"tertiary_link", update_car_direct_tertiary},
                 {"unclassified", update_car_direct_residentiary},
                 {"residential", update_car_direct_residentiary},
                 {"living_street", update_car_direct_residentiary},
                 {"road", update_car_direct_residentiary},
                 {"service", update_car_direct_residentiary},
                 {"track", update_car_direct_residentiary},
                 {"motorway",
                  [&]() {
                      update_car_direct_motorway();
                      set_visible_false();
                  }},
                 {"motorway_link",
                  [&]() {
                      update_car_direct_motorway();
                      set_visible_false();
                  }},
                 {"trunk",
                  [&]() {
                      update_car_direct_trunk();
                      set_visible_false();
                  }},
                 {"trunk_link",
                  [&]() {
                      update_car_direct_trunk();
                      set_visible_false();
                  }},
             }},

            {"pedestrian",
             {
                 {"yes", set_foot_allowed},
                 {"designated", set_foot_allowed},
                 {"permissive", set_foot_allowed},
                 {"lane", set_foot_allowed},
                 {"official", set_foot_allowed},
                 {"allowed", set_foot_allowed},
                 {"destination", set_foot_allowed},
                 {"no", set_foot_forbidden},
                 {"private", set_foot_forbidden},
             }},
            {"foot",
             {
                 {"yes", set_foot_allowed},
                 {"designated", set_foot_allowed},
                 {"permissive", set_foot_allowed},
                 {"lane", set_foot_allowed},
                 {"official", set_foot_allowed},
                 {"allowed", set_foot_allowed},
                 {"destination", set_foot_allowed},
                 {"no", set_foot_forbidden},
                 {"private", set_foot_forbidden},
             }},

            // http://wiki.openstreetmap.org/wiki/Cycleway
            // http://wiki.openstreetmap.org/wiki/Map_Features#Cycleway
            {"cycleway",
             {
                 {"lane", set_bike_direct_lane},
                 {"yes", set_bike_direct_lane},
                 {"true", set_bike_direct_lane},
                 {"lane_in_the_middle", set_bike_direct_lane},
                 {"track", set_bike_direct_track},
                 {"opposite_lane", set_bike_reverse_lane},
                 {"opposite_track", set_bike_reverse_track},
                 {"opposite", set_bike_reverse_allowed},
                 {"share_busway", set_bike_direct_busway},
                 {"lane_left", set_bike_reverse_lane},
                 {"_default", set_bike_direct_lane},
             }},

            {"bicycle",
             {
                 {"yes", set_bike_direct_allowed},
                 {"permissive", set_bike_direct_allowed},
                 {"destination", set_bike_direct_allowed},
                 {"designated", set_bike_direct_allowed},
                 {"private", set_bike_direct_allowed},
                 {"true", set_bike_direct_allowed},
                 {"allowed", set_bike_direct_allowed},
                 {"official", set_bike_direct_allowed},
                 {"no", set_bike_direct_forbidden},
                 {"dismount", set_bike_direct_forbidden},
                 {"VTT", set_bike_direct_forbidden},
                 {"share_busway", set_bike_direct_busway},
                 {"opposite_lane", set_bike_reverse_allowed},
                 {"opposite", set_bike_reverse_allowed},
             }},

            {"busway",
             {
                 {"yes", update_bike_direct_busway},
                 {"track", update_bike_direct_busway},
                 {"lane", update_bike_direct_busway},
                 {"opposite_lane", update_bike_reverse_busway},
                 {"opposite_track", update_bike_reverse_busway},
                 {"_default", update_bike_direct_busway},
             }},

            {"oneway",
             {
                 {"yes",
                  [&]() {
                      set_car_reverse_forbidden();
                      update_bike_reverse_forbidden();
                  }},
                 {"true",
                  [&]() {
                      set_car_reverse_forbidden();
                      update_bike_reverse_forbidden();
                  }},
                 {"1",
                  [&]() {
                      set_car_reverse_forbidden();
                      update_bike_reverse_forbidden();
                  }},
             }},

            {"junction",
             {
                 {"roundabout",
                  [&]() {
                      set_car_reverse_forbidden();
                      update_bike_reverse_forbidden();
                  }},
             }},

            {"access",
             {
                 {"yes", set_foot_allowed},
                 {"no", set_foot_forbidden},
             }},

            {"public_transport",
             {
                 {"platform", set_foot_allowed},
             }},

            {"railway",
             {
                 {"platform",
                  [&]() {
                      set_foot_allowed();
                      set_visible_false();
                  }},
             }},
        };

        std::string key = pair.first, val = pair.second;
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        std::transform(val.begin(), val.end(), val.begin(), ::tolower);

        // We do not want to autocomplete on public transport objects and tunnels
        if (key == "public_transport" || key == "tunnel") {
            set_visible_false();
        }

        try {
            key_map[key].at(val)();
        } catch (...) {
            try {
                key_map[key].at("_default")();
            } catch (...) {
                std::cerr << "I don't know what to do with: " << key << "=" << val << std::endl;
            }
        }
    }

    update_if_unknown(car_reverse, car_direct);
    update_if_unknown(bike_reverse, bike_direct);
    update_if_unknown(car_direct, car_forbidden);
    update_if_unknown(bike_direct, bike_forbidden);
    update_if_unknown(car_reverse, car_forbidden);
    update_if_unknown(bike_reverse, bike_forbidden);
    update_if_unknown(foot, foot_forbidden);

    std::bitset<8> result;
    result[CYCLE_FWD] = (bike_direct != bike_forbidden);
    result[CYCLE_BWD] = (bike_reverse != bike_forbidden);
    result[CAR_FWD] = (car_direct != car_forbidden);
    result[CAR_BWD] = (car_reverse != car_forbidden);
    result[FOOT_FWD] = (foot != foot_forbidden);
    result[FOOT_BWD] = (foot != foot_forbidden);
    if (result.any()) {  // only set visibility if the road is usable
        result[VISIBLE] = visible;
    }
    return result;
}

PoiTypeParams::PoiTypeParams(const std::string& json_params) {
    namespace pt = boost::property_tree;

    pt::ptree poi_root;
    std::istringstream iss(json_params);
    pt::read_json(iss, poi_root);

    for (const pt::ptree::value_type& json_poi_type : poi_root.get_child("poi_types")) {
        const auto& poi_type_id = json_poi_type.second.get<std::string>("id");
        const auto& poi_type_name = json_poi_type.second.get<std::string>("name");
        if (!poi_types.insert({poi_type_id, poi_type_name}).second) {
            throw std::invalid_argument("POI type id " + poi_type_id + " is defined multiple times");
        }
    }
    if (poi_types.find("amenity:parking") == poi_types.end()
        || poi_types.find("amenity:bicycle_rental") == poi_types.end()) {
        throw std::invalid_argument(
            "The 2 POI types id=amenity:parking and "
            "id=amenity:bicycle_rental must be defined");
    }

    for (const pt::ptree::value_type& json_rule : poi_root.get_child("rules")) {
        rules.emplace_back();
        auto& rule = rules.back();
        rule.poi_type_id = json_rule.second.get<std::string>("poi_type_id");
        if (poi_types.find(rule.poi_type_id) == poi_types.end()) {
            throw std::invalid_argument("Using an undefined POI type id (" + rule.poi_type_id + ") in rules");
        }
        for (const pt::ptree::value_type& json_filter : json_rule.second.get_child("osm_tags_filters")) {
            rule.osm_tag_filters[json_filter.second.get<std::string>("key")] =
                json_filter.second.get<std::string>("value");
        }
    }
}

/**
 * the first rule that matches OSM object decides POI-type
 * match : OSM object has all OSM tags required by rule
 */
const RuleOsmTag2PoiType* PoiTypeParams::get_applicable_poi_rule(const CanalTP::Tags& tags) const {
    auto poi_misses_a_tag = [&](const std::pair<const std::string, std::string>& osm_rule_tag) {
        const auto it_poi_tag = tags.find(osm_rule_tag.first);
        return (it_poi_tag == tags.end() || it_poi_tag->second != osm_rule_tag.second);
    };
    for (const auto& osm_rule : rules) {
        if (!navitia::contains_if(osm_rule.osm_tag_filters, poi_misses_a_tag)) {
            return &osm_rule;
        }
    }
    return nullptr;
}

const std::string get_postal_code_from_tags(const CanalTP::Tags& tags) {
    if (tags.find("addr:postcode") != tags.end()) {
        return tags.at("addr:postcode");
    }

    if (tags.find("postal_code") != tags.end()) {
        return tags.at("postal_code");
    }

    return std::string();
}

}  // namespace connectors
}  // namespace ed
