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

#pragma once
#include "type/type.h"
#include "third_party/osmpbfreader/osmpbfreader.h"
#include "utils/lotus.h"
#include "utils/logger.h"
#include <unordered_map>
#include <set>
#include "ed/types.h"
#include "ed_persistor.h"
#include "ed/connectors/osm_tags_reader.h"
#include <boost/geometry.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>
#include <boost/geometry/multi/geometries/multi_point.hpp>
#include "third_party/RTree/RTree.h"

namespace bg = boost::geometry;
typedef bg::model::point<double, 2, bg::cs::cartesian> point;
typedef bg::model::polygon<point, false, false> polygon_type; // ccw, open polygon
typedef bg::model::multi_polygon<polygon_type> mpolygon_type;
typedef bg::model::multi_point<point> mpoint_type;
typedef bg::model::linestring<point> ls_type;



namespace ed { namespace connectors {

struct OSMRelation;
struct OSMCache;

struct OSMNode {
    static const uint USED_MORE_THAN_ONCE = 0,
                      FIRST_OR_LAST = 1;
    uint64_t osm_id = std::numeric_limits<uint64_t>::max();
    // these attributes are mutable because this object would be use in a set, and
    // all object in a set are const, since these attributes are not used in the key we can modify them

    // We use int32_t to save memory, these are coordinates *  factor
    mutable int32_t ilon = std::numeric_limits<int32_t>::max(),
                    ilat = std::numeric_limits<int32_t>::max();
    mutable const OSMRelation* admin = nullptr;
    static constexpr double factor = 1e6;

    OSMNode(uint64_t osm_id) : osm_id(osm_id) {}

    bool operator<(const OSMNode& other) const {
        return this->osm_id < other.osm_id;
    }

    bool almost_equal(const OSMNode& other) const {
        // check if the nodes are quite at the same location
        auto distance = 10; // about 0.5m
        return std::abs(this->ilon - other.ilon) < distance && std::abs(this->ilat - other.ilat) < distance;
    }

    // Even if it's const, it modifies the variable used_more_than_once,
    // cause it's mutable, and doesn't affect the operator <
    void set_used_more_than_once() const {
        this->properties[USED_MORE_THAN_ONCE] = true;
    }

    bool is_defined() const {
        return ilon != std::numeric_limits<int32_t>::max() &&
            ilat != std::numeric_limits<int32_t>::max();
    }

    bool is_used_more_than_once() const {
        return this->properties[USED_MORE_THAN_ONCE];
    }

    bool is_used() const {
        return is_used_more_than_once() || is_first_or_last();
    }

    bool is_first_or_last() const {
        return this->properties[FIRST_OR_LAST];
    }

    void set_first_or_last() const {
        this->properties[FIRST_OR_LAST] = true;
    }

    void set_coord(double lon, double lat) const{
        this->ilon = lon * factor;
        this->ilat = lat * factor;
    }

    double lon() const {
        return double(this->ilon) / factor;
    }

    double lat() const {
        return double(this->ilat) / factor;
    }

    std::string coord_to_string() const {
        std::stringstream geog;
        geog << std::setprecision(10) << lon() << " " << lat();
        return geog.str();
    }
    std::string to_geographic_point() const;
private:
    mutable std::bitset<2> properties = 0;
};


struct OSMRelation {
    const u_int64_t osm_id;
    CanalTP::References references;
    const std::string insee = "", name = "";
    std::vector<std::string> postal_codes;
    const uint32_t level = std::numeric_limits<uint32_t>::max();

    // these attributes are mutable because this object would be use in a set, and
    // all object in a set are const, since these attributes are not used in the key we can modify them
    mutable mpolygon_type polygon;
    mutable point centre = point(0.0, 0.0);

    OSMRelation(const u_int64_t osm_id, const std::vector<CanalTP::Reference>& refs,
                const std::string& insee, const std::string& postal_code,
                const std::string& name, const uint32_t level);

    bool operator <(const OSMRelation& other) const{
        return this->osm_id < other.osm_id;
    }

    bool operator <(const u_int64_t other) const {
        return osm_id < other;
    }

    void set_centre(float lon, float lat) const {
        centre = point(lon, lat);
    }

    void build_geometry(OSMCache& cache) const;
    void build_polygon(OSMCache& cache) const;
};

struct OSMWay {
    const static uint8_t CYCLE_FWD = 0;
    const static uint8_t CYCLE_BWD = 1;
    const static uint8_t CAR_FWD = 2;
    const static uint8_t CAR_BWD = 3;
    const static uint8_t FOOT_FWD = 4;
    const static uint8_t FOOT_BWD = 5;

    const u_int64_t osm_id;

    // these attributes are mutable because this object would be use in a set, and
    // all object in a set are const, since these attributes are not used in the key we can modify them

    /// Properties of a way : can we use it
    mutable std::bitset<8> properties;
    mutable std::string name = "";
    mutable std::vector<std::set<OSMNode>::const_iterator> nodes;
    mutable ls_type ls;
    mutable const OSMWay* way_ref = nullptr;

    OSMWay(const u_int64_t osm_id) : osm_id(osm_id) {}
    OSMWay(const u_int64_t osm_id, const std::bitset<8>& properties,
            const std::string& name) :
        osm_id(osm_id), properties(properties), name(name) {}

    void add_node(std::set<OSMNode>::const_iterator node) const {
        nodes.push_back(node);
        if (node->is_defined()) {
            ls.push_back(point(node->lon(), node->lat()));
        }
    }

    bool operator<(const OSMWay& other) const {
        return this->osm_id < other.osm_id;
    }

    void set_properties(const std::bitset<8>& properties) const {
        this->properties = properties;
    }

    void set_name(const std::string& name) const {
        this->name = name;
    }

    std::string coord_to_string() const {
        std::stringstream geog;
        geog << std::setprecision(10);
        for(auto node : nodes) {
            geog << node->coord_to_string();
        }
        return geog.str();
    }

    double distance(const double lon, const double lat) const {
        point p(lon, lat);
        return distance(p);
    }

    double distance(point p) const {
        if (ls.empty()) {
            return std::numeric_limits<double>::max();
        }
        return bg::distance(p, ls);
    }

    std::set<const OSMRelation*> admins() const {
        std::set<const OSMRelation*> result;
        for(auto node : nodes) {
            if (node->admin != nullptr) {
                result.insert(node->admin);
            }
        }
        return result;
    }

    bool is_used() const {
        return way_ref == nullptr || this == way_ref;
    }

    bool is_street() const{
        return this->properties.any();
    }
};

struct OSMHouseNumber {
    const size_t number;
    const double lon, lat;
    const OSMWay* way = nullptr;
    OSMHouseNumber(const size_t number, const double lon, const double lat,
            const OSMWay* way) : number(number), lon(lon), lat(lat), way(way) {
    }
};

struct AssociateStreetRelation {
    const uint64_t osm_id;
    const uint64_t way_id = std::numeric_limits<uint64_t>::max();
    const std::string streetname = "";

    AssociateStreetRelation(const uint64_t osm_id, const uint64_t way_id,
                            const std::string& streetname) :
        osm_id(osm_id), way_id(way_id), streetname(streetname) {}

    AssociateStreetRelation(const uint64_t osm_id) : osm_id(osm_id) {}


    bool operator<(const AssociateStreetRelation& other) const {
        return this->osm_id < other.osm_id;
    }
};


typedef std::set<OSMWay>::const_iterator it_way;
typedef std::map<std::set<const OSMRelation*>, std::set<it_way>> rel_ways;
typedef std::set<OSMRelation>::const_iterator admin_type;
typedef std::pair<admin_type, double> admin_distance;


struct OSMCache {
    std::set<OSMRelation> relations;
    std::set<OSMNode> nodes;
    std::set<OSMWay> ways;
    std::set<AssociateStreetRelation> associated_streets;
    std::unordered_map<std::string, rel_ways> way_admin_map;
    RTree<const OSMRelation*, double, 2> admin_tree;
    RTree<it_way, double, 2> way_tree;
    double max_search_distance = 0;
    size_t NB_PROJ = 0;

    Lotus lotus;

    OSMCache(const std::string& connection_string) : lotus(connection_string) {}

    void build_relations_geometries();
    const OSMRelation* match_coord_admin(const double lon, const double lat);
    void match_nodes_admin();
    void insert_nodes();
    void insert_ways();
    void insert_edges();
    void insert_relations();
    void insert_postal_codes();
    void insert_rel_way_admins();
    void build_way_map();
    void fusion_ways();
    void flag_nodes();
};

struct ReadRelationsVisitor {
    OSMCache& cache;
    ReadRelationsVisitor(OSMCache& cache) : cache(cache) {}

    void node_callback(uint64_t , double , double , const CanalTP::Tags& ) {}
    void relation_callback(uint64_t osm_id, const CanalTP::Tags & tags, const CanalTP::References & refs);
    void way_callback(uint64_t , const CanalTP::Tags& , const std::vector<uint64_t>&) {}
};
struct ReadWaysVisitor {
    // Read references and set if a node is used by a way
    log4cplus::Logger logger = log4cplus::Logger::getInstance("log");
    OSMCache& cache;
    const PoiTypeParams poi_params;

    ReadWaysVisitor(OSMCache& cache, const PoiTypeParams& poi_params) :
        cache(cache), poi_params(poi_params) {}
    ~ReadWaysVisitor();

    void node_callback(uint64_t , double , double , const CanalTP::Tags& ) {}
    void relation_callback(uint64_t , const CanalTP::Tags& , const CanalTP::References& ) {}
    void way_callback(uint64_t osm_id, const CanalTP::Tags& tags, const std::vector<uint64_t>& nodes);

    size_t filtered_private_way = 0;
};


struct ReadNodesVisitor {
    // Read references and set if a node is used by a way
    log4cplus::Logger logger = log4cplus::Logger::getInstance("log");
    OSMCache& cache;

    ReadNodesVisitor(OSMCache& cache) : cache(cache) {}

    void node_callback(uint64_t osm_id, double lon, double lat, const CanalTP::Tags& tag);
    void relation_callback(uint64_t , const CanalTP::Tags& , const CanalTP::References& ) {}
    void way_callback(uint64_t , const CanalTP::Tags& , const std::vector<uint64_t>&) {}
};

struct Rect{
    double min[2];
    double max[2];
    Rect() : min{0,0}, max{0,0} {}

    Rect(double lon, double lat) {
        min[0] = lon;
        min[1] = lat;
        max[0] = lon;
        max[1] = lat;
    }

    Rect(double a_minX, double a_minY, double a_maxX, double a_maxY){
        min[0] = a_minX;
        min[1] = a_minY;

        max[0] = a_maxX;
        max[1] = a_maxY;
    }
};

enum class OsmObjectType {
    Node,
    Way
};

inline std::string to_string(OsmObjectType t) {
    switch (t) {
    case OsmObjectType::Node: return "n";
    case OsmObjectType::Way: return "w";
    default:
        throw navitia::exception("not implemented");
    }
}

struct PoiHouseNumberVisitor {
    const size_t max_inserts_without_bulk = 20000;
    ed::EdPersistor& persistor;
    /*const*/ OSMCache& cache;
    ed::Georef& data;
    bool parse_pois;
    std::vector<OSMHouseNumber> house_numbers;
    size_t n_inserted_pois = 0;
    size_t n_inserted_house_numbers = 0;
    const PoiTypeParams poi_params;

    PoiHouseNumberVisitor(EdPersistor& persistor, /*const*/ OSMCache& cache, Georef& data,
                          const bool parse_pois, const PoiTypeParams& poi_params) :
                              persistor(persistor), cache(cache), data(data),
                              parse_pois(parse_pois), poi_params(poi_params) {
        uint32_t idx = 0;
        for(const auto& type: poi_params.poi_types){
            data.poi_types[type.first] = new ed::types::PoiType(idx, type.second);
            ++idx;
        }

        persistor.insert_poi_types(data);
    }

    void node_callback(uint64_t osm_id, double lon, double lat, const CanalTP::Tags& tags);
    void relation_callback(uint64_t , const CanalTP::Tags &, const CanalTP::References &) {}
    void way_callback(uint64_t osm_id, const CanalTP::Tags &tags, const std::vector<uint64_t> & refs);
    const OSMWay* find_way_without_name(const double lon, const double lat);
    const OSMWay* find_way(const CanalTP::Tags& tags, const double lon, const double lat);
    void fill_poi(const u_int64_t osm_id, const CanalTP::Tags& tags, const double lon, const double lat, OsmObjectType t);
    void fill_housenumber(const u_int64_t osm_id, const CanalTP::Tags& tags, const double lon, const double lat);
    void insert_house_numbers();
    void insert_data();
    void finish();


};
inline bool operator<(const ed::connectors::it_way w1, const ed::connectors::it_way w2) {
    return w1->osm_id < w2->osm_id;
}

int osm2ed(int argc, const char** argv);

}}
