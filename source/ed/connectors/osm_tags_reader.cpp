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
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#include "osm_tags_reader.h"
#include "utils/functions.h"
#include <iostream>
#include <algorithm>

namespace ed { namespace connectors{

static void update_if_unknown(int& target, const int& source) {
    if (target == -1) {
        target = source;
    }
}

std::bitset<8> parse_way_tags(const std::map<std::string, std::string> & tags){
    // if no highway tag, we can do nothing on it
    if (! tags.count("highway")) { return {}; }

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

    for(std::pair<std::string, std::string> pair : tags){
        std::string key = pair.first, val = pair.second;
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        std::transform(val.begin(), val.end(), val.begin(), ::tolower);

        if (key == "highway") {
            if (in(val, {"footway", "pedestrian"})) {
                update_if_unknown(foot, foot_allowed);
            } else if (val == "cycleway") {
                update_if_unknown(bike_direct, bike_track);
                update_if_unknown(foot, foot_allowed);
            } else if (val == "path") {
                //http://www.cyclestreets.net/journey/help/osmconversion/#toc6
                //highway = path => might mean lots of different things, so we allow bike and foot
                update_if_unknown(bike_direct, bike_track);
                update_if_unknown(foot, foot_allowed);
            } else if (val == "steps") {
                update_if_unknown(foot, foot_allowed);
            } else if (in(val, {"primary", "primary_link"})) {
                update_if_unknown(car_direct, car_primary);
                update_if_unknown(foot, foot_allowed);
                update_if_unknown(bike_direct, bike_allowed);
            } else if (in(val, {"secondary", "secondary_link"})) {
                update_if_unknown(car_direct, car_secondary);
                update_if_unknown(foot, foot_allowed);
                update_if_unknown(bike_direct, bike_allowed);
            } else if (in(val, {"tertiary", "tertiary_link"})) {
                update_if_unknown(car_direct, car_tertiary);
                update_if_unknown(foot, foot_allowed);
                update_if_unknown(bike_direct, bike_allowed);
            } else if (in(val, {"unclassified", "residential", "living_street",
                            "road", "service", "track"})) {
                update_if_unknown(car_direct, car_residential);
                update_if_unknown(foot, foot_allowed);
                update_if_unknown(bike_direct, bike_allowed);
            } else if (in(val, {"motorway", "motorway_link"})) {
                update_if_unknown(car_direct, car_motorway);
                update_if_unknown(foot, foot_forbiden);
                update_if_unknown(bike_direct, bike_forbiden);
            } else if (in(val, {"trunk", "trunk_link"})) {
                update_if_unknown(car_direct, car_trunk);
                update_if_unknown(foot, foot_forbiden);
                update_if_unknown(bike_direct, bike_forbiden);
            }
        }

        else if (in(key, {"pedestrian", "foot"})) {
            if (in(val, {"yes", "designated", "permissive", "lane", "official",
                            "allowed", "destination"})) {
                foot = foot_allowed;
            } else if (in(val, {"no", "private"})) {
                foot = foot_forbiden;
            } else {
                std::cerr << "I don't know what to do with: " << key << "=" << val << std::endl;
            }
        }

        // http://wiki.openstreetmap.org/wiki/Cycleway
        // http://wiki.openstreetmap.org/wiki/Map_Features#Cycleway
        else if(key == "cycleway") {
            if(val == "lane" || val == "yes" || val == "true" || val == "lane_in_the_middle")
                bike_direct = bike_lane;
            else if(val == "track")
                bike_direct = bike_track;
            else if(val == "opposite_lane")
                bike_reverse = bike_lane;
            else if(val == "opposite_track")
                bike_reverse = bike_track;
            else if(val == "opposite")
                bike_reverse = bike_allowed;
            else if(val == "share_busway")
                bike_direct = bike_busway;
            else if(val == "lane_left")
                bike_reverse = bike_lane;
            else
                bike_direct = bike_lane;
        }
        else if(key == "bicycle") {
            if(val == "yes" || val == "permissive" || val == "destination" || val == "designated"
                    || val == "private" || val == "true" || val == "allowed"
                    || val == "official" || val == "destination")
                bike_direct = bike_allowed;
            else if(val == "no" || val == "dismount" || val == "VTT")
                bike_direct = bike_forbiden;
            else if(val == "share_busway")
                bike_direct = bike_busway;
            else if(val == "opposite_lane" || val == "opposite")
                bike_reverse = bike_allowed;
            else
                std::cerr << "I don't know what to do with: " << key << "=" << val << std::endl;
        }

        else if (key == "busway") {
            if (in(val, {"yes", "track", "lane"})) {
                update_if_unknown(bike_direct, bike_busway);
            } else if (in(val, {"opposite_lane", "opposite_track"})) {
                update_if_unknown(bike_reverse, bike_busway);
            } else {
                update_if_unknown(bike_direct, bike_busway);
            }
        }

        else if(key == "oneway") {
            if(val == "yes" || val == "true" || val == "1") {
                car_reverse = car_forbiden;
                update_if_unknown(bike_reverse, bike_forbiden);
            }
        }

        else if(key == "junction") {
            if(val == "roundabout") {
                car_reverse = car_forbiden;
                update_if_unknown(bike_reverse, bike_forbiden);
            }
        }

        else if(key == "access"){
            if(val == "yes"){
                foot = foot_allowed;
            } else if(val == "no"){
                foot = foot_forbiden;
            }
        }

        else if(key == "public_transport"){
            if(val == "platform"){
                foot = foot_allowed;
            }
        }

        else if(key == "railway"){
            if(val == "platform"){
                foot = foot_allowed;
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
    return result;
}

}} //namespace navitia::OSM
