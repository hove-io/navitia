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
#include "ed/types.h"
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>

namespace bg = boost::geometry;
typedef bg::model::point<float, 2, bg::cs::cartesian> point;
typedef bg::model::polygon<point, false, false> polygon_type; // ccw, open polygon
typedef bg::model::multi_polygon<polygon_type> mpolygon_type;

namespace navitia { namespace cities {

struct OSMRelation;
struct OSMCache;

struct OSMNode {
    // We use int32_t to save memory, these are coordinates *  factor
    int32_t ilon = std::numeric_limits<int32_t>::max(),
            ilat = std::numeric_limits<int32_t>::max();
    static constexpr double factor = 1e6;

    bool is_defined() const {
        return ilon != std::numeric_limits<int32_t>::max() &&
            ilat != std::numeric_limits<int32_t>::max();
    }

    void set_coord(double lon, double lat) {
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
};


struct OSMRelation {
    CanalTP::References references;
    const std::string insee = "",
                      postal_code = "",
                      name = "";
    const uint32_t level = std::numeric_limits<uint32_t>::max();

    // these attributes are mutable because this object would be use in a set, and
    // all object in a set are const, since these attributes are not used in the key we can modify them
    mpolygon_type polygon;
    point centre = point(0.0, 0.0);

    OSMRelation(const std::vector<CanalTP::Reference>& refs,
                const std::string& insee, const std::string postal_code,
                const std::string& name, const uint32_t level) :
        references(refs), insee(insee),
        postal_code(postal_code), name(name), level(level) {}

    void set_centre(float lon, float lat) {
        centre = point(lon, lat);
    }
    void build_geometry(OSMCache& cache);
    void build_polygon(OSMCache& cache);
};

struct OSMWay {
    /// Properties of a way : can we use it
    std::vector<std::unordered_map<uint64_t, OSMNode>::const_iterator> nodes;

    void add_node(std::unordered_map<uint64_t, OSMNode>::const_iterator node) {
        nodes.push_back(node);
    }

    std::string coord_to_string() const {
        std::stringstream geog;
        geog << std::setprecision(10);
        for(auto node : nodes) {
            geog << node->second.coord_to_string();
        }
        return geog.str();
    }
};

typedef std::set<OSMWay>::const_iterator it_way;
typedef std::map<const OSMRelation*, std::vector<it_way>> rel_ways;
typedef std::set<OSMRelation>::const_iterator admin_type;
typedef std::pair<admin_type, double> admin_distance;

struct OSMCache {
    std::unordered_map<uint64_t, OSMRelation> relations;
    std::unordered_map<uint64_t, OSMNode> nodes;
    std::unordered_map<uint64_t, OSMWay> ways;

    Lotus lotus;

    OSMCache(const std::string& connection_string) : lotus(connection_string) {}

    void build_relations_geometries();
    const OSMRelation* match_coord_admin(const double lon, const double lat);
    void match_nodes_admin();
    void insert_relations();
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

    ReadWaysVisitor(OSMCache& cache) : cache(cache) {}

    void node_callback(uint64_t , double , double , const CanalTP::Tags& ) {}
    void relation_callback(uint64_t , const CanalTP::Tags& , const CanalTP::References& ) {}
    void way_callback(uint64_t osm_id, const CanalTP::Tags& tags, const std::vector<uint64_t>& nodes);
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
}}

