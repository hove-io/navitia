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

#include "osm_tags_reader.h"
#include "utils/functions.h"
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <algorithm>
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
    constexpr int foot_forbiden = 0;
    constexpr int foot_allowed = 1;

    constexpr int car_forbiden = 0;
    constexpr int car_residential = 1;
    constexpr int car_tertiary = 2;
    constexpr int car_secondary = 3;
    constexpr int car_primary = 4;
    constexpr int car_trunk = 5;
    constexpr int car_motorway = 6;

    constexpr int bike_forbiden = 0;
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

    for (std::pair<std::string, std::string> pair : tags) {
        std::string key = pair.first, val = pair.second;
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        std::transform(val.begin(), val.end(), val.begin(), ::tolower);

        if (key == "highway") {
            if (navitia::contains({"footway", "pedestrian"}, val)) {
                update_if_unknown(foot, foot_allowed);
            } else if (val == "cycleway") {
                update_if_unknown(bike_direct, bike_track);
                update_if_unknown(foot, foot_allowed);
            } else if (val == "path") {
                // http://www.cyclestreets.net/journey/help/osmconversion/#toc6
                // highway = path => might mean lots of different things, so we allow bike and foot
                update_if_unknown(bike_direct, bike_track);
                update_if_unknown(foot, foot_allowed);
            } else if (val == "steps") {
                update_if_unknown(foot, foot_allowed);
            } else if (navitia::contains({"primary", "primary_link"}, val)) {
                update_if_unknown(car_direct, car_primary);
                update_if_unknown(foot, foot_allowed);
                update_if_unknown(bike_direct, bike_allowed);
            } else if (navitia::contains({"secondary", "secondary_link"}, val)) {
                update_if_unknown(car_direct, car_secondary);
                update_if_unknown(foot, foot_allowed);
                update_if_unknown(bike_direct, bike_allowed);
            } else if (navitia::contains({"tertiary", "tertiary_link"}, val)) {
                update_if_unknown(car_direct, car_tertiary);
                update_if_unknown(foot, foot_allowed);
                update_if_unknown(bike_direct, bike_allowed);
            } else if (navitia::contains({"unclassified", "residential", "living_street", "road", "service", "track"},
                                         val)) {
                update_if_unknown(car_direct, car_residential);
                update_if_unknown(foot, foot_allowed);
                update_if_unknown(bike_direct, bike_allowed);
            } else if (navitia::contains({"motorway", "motorway_link"}, val)) {
                update_if_unknown(car_direct, car_motorway);
                update_if_unknown(foot, foot_forbiden);
                update_if_unknown(bike_direct, bike_forbiden);
            } else if (navitia::contains({"trunk", "trunk_link"}, val)) {
                update_if_unknown(car_direct, car_trunk);
                update_if_unknown(foot, foot_forbiden);
                update_if_unknown(bike_direct, bike_forbiden);
            }
        }

        else if (navitia::contains({"pedestrian", "foot"}, key)) {
            if (navitia::contains({"yes", "designated", "permissive", "lane", "official", "allowed", "destination"},
                                  val)) {
                foot = foot_allowed;
            } else if (navitia::contains({"no", "private"}, val)) {
                foot = foot_forbiden;
            } else {
                std::cerr << "I don't know what to do with: " << key << "=" << val << std::endl;
            }
        }

        // http://wiki.openstreetmap.org/wiki/Cycleway
        // http://wiki.openstreetmap.org/wiki/Map_Features#Cycleway
        else if (key == "cycleway") {
            if (val == "lane" || val == "yes" || val == "true" || val == "lane_in_the_middle")
                bike_direct = bike_lane;
            else if (val == "track")
                bike_direct = bike_track;
            else if (val == "opposite_lane")
                bike_reverse = bike_lane;
            else if (val == "opposite_track")
                bike_reverse = bike_track;
            else if (val == "opposite")
                bike_reverse = bike_allowed;
            else if (val == "share_busway")
                bike_direct = bike_busway;
            else if (val == "lane_left")
                bike_reverse = bike_lane;
            else
                bike_direct = bike_lane;
        } else if (key == "bicycle") {
            if (val == "yes" || val == "permissive" || val == "destination" || val == "designated" || val == "private"
                || val == "true" || val == "allowed" || val == "official")
                bike_direct = bike_allowed;
            else if (val == "no" || val == "dismount" || val == "VTT")
                bike_direct = bike_forbiden;
            else if (val == "share_busway")
                bike_direct = bike_busway;
            else if (val == "opposite_lane" || val == "opposite")
                bike_reverse = bike_allowed;
            else
                std::cerr << "I don't know what to do with: " << key << "=" << val << std::endl;
        }

        else if (key == "busway") {
            if (navitia::contains({"yes", "track", "lane"}, val)) {
                update_if_unknown(bike_direct, bike_busway);
            } else if (navitia::contains({"opposite_lane", "opposite_track"}, val)) {
                update_if_unknown(bike_reverse, bike_busway);
            } else {
                update_if_unknown(bike_direct, bike_busway);
            }
        }

        else if (key == "oneway") {
            if (val == "yes" || val == "true" || val == "1") {
                car_reverse = car_forbiden;
                update_if_unknown(bike_reverse, bike_forbiden);
            }
        }

        else if (key == "junction") {
            if (val == "roundabout") {
                car_reverse = car_forbiden;
                update_if_unknown(bike_reverse, bike_forbiden);
            }
        }

        else if (key == "access") {
            if (val == "yes") {
                foot = foot_allowed;
            } else if (val == "no") {
                foot = foot_forbiden;
            }
        }

        else if (key == "public_transport") {
            if (val == "platform") {
                foot = foot_allowed;
            }
        }

        else if (key == "railway") {
            if (val == "platform") {
                foot = foot_allowed;
            }
        }

        // We do not want to autocomplete on public transport objects
        if (key == "public_transport") {
            visible = false;
        } else if (key == "railway") {
            if (val == "platform") {
                visible = false;
            }
        } else if (key == "highway") {
            if (val == "trunk" || val == "trunk_link" || val == "motorway" || val == "motorway_link") {
                visible = false;
            }
        }
    }

    update_if_unknown(car_reverse, car_direct);
    update_if_unknown(bike_reverse, bike_direct);
    update_if_unknown(car_direct, car_forbiden);
    update_if_unknown(bike_direct, bike_forbiden);
    update_if_unknown(car_reverse, car_forbiden);
    update_if_unknown(bike_reverse, bike_forbiden);
    update_if_unknown(foot, foot_forbiden);

    std::bitset<8> result;
    result[CYCLE_FWD] = (bike_direct != bike_forbiden);
    result[CYCLE_BWD] = (bike_reverse != bike_forbiden);
    result[CAR_FWD] = (car_direct != car_forbiden);
    result[CAR_BWD] = (car_reverse != car_forbiden);
    result[FOOT_FWD] = (foot != foot_forbiden);
    result[FOOT_BWD] = (foot != foot_forbiden);
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
