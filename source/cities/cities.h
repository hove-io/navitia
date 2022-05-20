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
#include "utils/lotus.h"
#include "utils/logger.h"

#include <osmpbfreader/osmpbfreader.h>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>
#include <boost/geometry/core/cs.hpp>
#include <RTree/RTree.h>
#include <boost/algorithm/string.hpp>

#include <unordered_map>
#include <set>

namespace bg = boost::geometry;
using point = bg::model::point<double, 2, bg::cs::cartesian>;
using polygon_type = bg::model::polygon<point, false, false>;  // ccw, open polygon
using mpolygon_type = bg::model::multi_polygon<polygon_type>;

namespace navitia {
namespace cities {

using OSMId = uint64_t;

struct Rect {
    double min[2];
    double max[2];
    Rect() : min{0, 0}, max{0, 0} {}

    Rect(double lon, double lat) {
        min[0] = lon;
        min[1] = lat;
        max[0] = lon;
        max[1] = lat;
    }

    Rect(double a_minX, double a_minY, double a_maxX, double a_maxY) {
        min[0] = a_minX;
        min[1] = a_minY;

        max[0] = a_maxX;
        max[1] = a_maxY;
    }
};

struct OSMRelation;
struct OSMCache;

struct OSMNode {
    // We use int32_t to save memory, these are coordinates *  factor
    int32_t ilon = std::numeric_limits<int32_t>::max(), ilat = std::numeric_limits<int32_t>::max();
    std::string postal_code;
    static constexpr double factor = 1e6;

    bool is_defined() const {
        return ilon != std::numeric_limits<int32_t>::max() && ilat != std::numeric_limits<int32_t>::max();
    }

    void set_coord(double lon, double lat) {
        this->ilon = lon * factor;
        this->ilat = lat * factor;
    }

    double lon() const { return double(this->ilon) / factor; }

    double lat() const { return double(this->ilat) / factor; }

    bool almost_equal(const OSMNode& other) const {
        // check if the nodes are quite at the same location
        auto distance = 10;  // about 0.5m
        return std::abs(this->ilon - other.ilon) < distance && std::abs(this->ilat - other.ilat) < distance;
    }

    std::string coord_to_string() const;
    std::string to_geographic_point() const;
};

struct OSMRelation {
    Hove::References references;
    const std::string insee = "", name = "";
    const uint32_t level = std::numeric_limits<uint32_t>::max();

    std::set<std::string> postal_codes;

    mpolygon_type polygon;
    point centre = point(0.0, 0.0);

    OSMRelation(std::vector<Hove::Reference> refs,
                std::string insee,
                const std::string& postal_code,
                std::string name,
                const uint32_t level);

    std::string postal_code() const;
    void add_postal_code(const std::string& postal_code);

    void set_centre(double lon, double lat) { centre = point(lon, lat); }
    void build_geometry(OSMCache& cache, OSMId);
    void build_polygon(OSMCache& cache, OSMId);
};

struct OSMWay {
    /// Properties of a way : can we use it
    std::vector<std::unordered_map<OSMId, OSMNode>::const_iterator> nodes;

    void add_node(std::unordered_map<OSMId, OSMNode>::const_iterator node) { nodes.push_back(node); }

    std::string coord_to_string() const;
};

using it_way = std::set<OSMWay>::const_iterator;
using rel_ways = std::map<const OSMRelation*, std::vector<it_way> >;
using admin_type = std::set<OSMRelation>::const_iterator;
using admin_distance = std::pair<admin_type, double>;

struct OSMCache {
    std::unordered_map<OSMId, OSMRelation> relations;
    std::unordered_map<OSMId, OSMNode> nodes;
    std::unordered_map<OSMId, OSMWay> ways;
    RTree<OSMRelation*, double, 2> admin_tree;

    Lotus lotus;

    OSMCache(const std::string& connection_string) : lotus(connection_string) {}

    void build_relations_geometries();
    OSMRelation* match_coord_admin(const double lon, const double lat);
    void match_nodes_admin();
    void insert_relations();
    void build_postal_codes();
};

struct ReadRelationsVisitor {
    OSMCache& cache;
    ReadRelationsVisitor(OSMCache& cache) : cache(cache) {}

    void node_callback(OSMId, double, double, const Hove::Tags&) {}
    void relation_callback(OSMId, const Hove::Tags& tags, const Hove::References& refs);
    void way_callback(OSMId, const Hove::Tags&, const std::vector<OSMId>&) {}
};
struct ReadWaysVisitor {
    // Read references and set if a node is used by a way
    log4cplus::Logger logger = log4cplus::Logger::getInstance("log");
    OSMCache& cache;

    ReadWaysVisitor(OSMCache& cache) : cache(cache) {}

    void node_callback(OSMId, double, double, const Hove::Tags&) {}
    void relation_callback(OSMId, const Hove::Tags&, const Hove::References&) {}
    void way_callback(OSMId, const Hove::Tags& tags, const std::vector<OSMId>& nodes_refs);
};

struct ReadNodesVisitor {
    // Read references and set if a node is used by a way
    log4cplus::Logger logger = log4cplus::Logger::getInstance("log");
    OSMCache& cache;

    ReadNodesVisitor(OSMCache& cache) : cache(cache) {}

    void node_callback(uint64_t osm_id, double lon, double lat, const Hove::Tags& tags);
    void relation_callback(uint64_t, const Hove::Tags&, const Hove::References&) {}
    void way_callback(uint64_t, const Hove::Tags&, const std::vector<uint64_t>&) {}
};
}  // namespace cities
}  // namespace navitia
