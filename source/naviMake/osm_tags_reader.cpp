#include "osm_tags_reader.h"
#include <iostream>

namespace navitia { namespace georef{

std::bitset<8> parse_way_tags(const std::map<std::string, std::string> & tags){
    const int unknown = -1;
    const int foot_forbiden = 0;
    const int foot_allowed = 1;

    const int car_forbiden = 0;
    const int car_residential = 1;
    const int car_tertiary = 2;
    const int car_secondary = 3;
    const int car_primary = 4;
    const int car_trunk = 5;
    const int car_motorway = 6;

    const int bike_forbiden = 0;
    const int bike_allowed = 2;
    const int bike_lane = 3;
    const int bike_busway = 4;
    const int bike_track = 5;


    int car_direct = unknown;
    int car_reverse = unknown;
    int bike_direct = unknown;
    int bike_reverse = unknown;
    int foot = unknown;

    for(std::pair<std::string, std::string> pair : tags){
        std::string key = pair.first, val = pair.second;

        if(key == "highway") {
            if(val == "cycleway" || val == "path" || val == "footway" || val == "pedestrian"){
                bike_direct = bike_track;
                foot = foot_allowed;
            } if(val == "steps"){
                foot = foot_allowed;
            } else if(val == "primary" || val == "primary_link") {
                car_direct = car_primary;
                foot = foot_allowed;
                bike_direct = bike_allowed;
            } else if(val == "secondary") {
                car_direct = car_secondary;
                foot = foot_allowed;
                bike_direct = bike_allowed;
            } else if(val == "tertiary") {
                car_direct = car_tertiary;
                foot = foot_allowed;
                bike_direct = bike_allowed;
            } else if(val == "unclassified" || val == "residential" || val == "living_street" ||
                    val == "road" || val == "service" || val == "track") {
                car_direct = car_residential;
                foot = foot_allowed;
                bike_direct = bike_allowed;
            } else if(val == "motorway" || val == "motorway_link") {
                car_direct = car_motorway;
                foot = foot_forbiden;
                bike_direct = bike_forbiden;
            } else if(val == "trunk" || val == "trunk_link") {
                car_direct = car_trunk;
                foot = foot_forbiden;
                bike_direct = bike_forbiden;
            }
        }

        else if(key == "pedestrian" || key == "foot") {
            if(val == "yes" || val == "designated" || val == "permissive" || val == "lane")
                foot = foot_allowed;
            else if(val == "no" || val == "private")
                foot = foot_forbiden;
            else
                std::cerr << "I don't know what to do with: " << key << "=" << val << std::endl;
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
            if(val == "yes" || val == "permissive" || val == "destination" || val == "designated" || val == "private" || val == "true")
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

        else if(key == "busway") {
            if(val == "yes" || val == "track" || val == "lane")
                bike_direct = bike_busway;
            else if(val == "opposite_lane" || val == "opposite_track")
                bike_reverse = bike_busway;
            else
                bike_direct = bike_busway;
        }

        else if(key == "oneway") {
            if(val == "yes" || val == "true" || val == "1") {
                car_reverse = car_forbiden;
                if(bike_reverse == unknown)
                    bike_reverse = bike_forbiden;
            }
        }

        else if(key == "junction") {
            if(val == "roundabout") {
                car_reverse = car_forbiden;
                if(bike_reverse == unknown)
                    bike_reverse = bike_forbiden;
            }
        }

        else if(key == "access"){
            if(val == "yes"){
                foot = foot_allowed;
            } else if(val == "no"){
                foot = foot_forbiden;
            }
        }
    }

    if(car_reverse == unknown && car_direct != unknown)
        car_reverse = car_direct;
    if(bike_reverse == unknown && bike_direct != unknown)
        bike_reverse = bike_direct;
    if(car_direct == unknown) car_direct = car_forbiden;
    if(bike_direct == unknown) bike_direct = bike_forbiden;
    if(car_reverse == unknown) car_reverse = car_forbiden;
    if(bike_reverse == unknown) bike_reverse = bike_forbiden;
    if(foot == unknown) foot = foot_forbiden;

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
