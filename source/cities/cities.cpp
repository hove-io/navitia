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

#include "cities.h"

#include "conf.h"
#include "ed/connectors/osm_tags_reader.h"
#include "utils/functions.h"
#include "utils/init.h"

#include <boost/dynamic_bitset.hpp>
#include <boost/geometry.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/reverse.hpp>

#include <cstdio>
#include <iomanip>
#include <iostream>
#include <queue>
#include <utility>

namespace po = boost::program_options;
namespace pt = boost::posix_time;

namespace navitia {
namespace cities {

std::string OSMNode::coord_to_string() const {
    std::stringstream geog;
    geog << std::setprecision(10) << lon() << " " << lat();
    return geog.str();
}

std::string OSMWay::coord_to_string() const {
    std::stringstream geog;
    geog << std::setprecision(10);
    for (auto node : nodes) {
        geog << node->second.coord_to_string();
    }
    return geog.str();
}

/*
 * Read relations
 * Stores admins relations and initializes nodes and ways associated to it.
 * Stores relations for associatedStreet, also initializes nodes and ways.
 */
void ReadRelationsVisitor::relation_callback(OSMId osm_id, const Hove::Tags& tags, const Hove::References& refs) {
    const auto boundary = tags.find("boundary");
    if (boundary == tags.end() || (boundary->second != "administrative" && boundary->second != "multipolygon")) {
        return;
    }
    const auto tmp_admin_level = tags.find("admin_level");
    if (tmp_admin_level != tags.end() && (tmp_admin_level->second == "8" || tmp_admin_level->second == "9")) {
        for (const Hove::Reference& ref : refs) {
            switch (ref.member_type) {
                case OSMPBF::Relation_MemberType::Relation_MemberType_WAY:
                    if (ref.role == "outer" || ref.role.empty() || ref.role == "exclave") {
                        cache.ways.insert(std::make_pair(ref.member_id, OSMWay()));
                    }
                    break;
                case OSMPBF::Relation_MemberType::Relation_MemberType_NODE:
                    if (ref.role == "admin_centre" || ref.role == "admin_center") {
                        cache.nodes.insert(std::make_pair(ref.member_id, OSMNode()));
                    }
                    break;
                case OSMPBF::Relation_MemberType::Relation_MemberType_RELATION:
                    continue;
            }
        }
        const std::string insee = find_or_default("ref:INSEE", tags);
        const std::string name = find_or_default("name", tags);
        const std::string postal_code = find_or_default("addr:postcode", tags);
        cache.relations.insert(std::make_pair(
            osm_id,
            OSMRelation(refs, insee, postal_code, name, boost::lexical_cast<uint32_t>(tmp_admin_level->second))));
    }
}

/*
 * We stores ways they are streets.
 * We store ids of needed nodes
 */
void ReadWaysVisitor::way_callback(OSMId osm_id, const Hove::Tags& /*unused*/, const std::vector<OSMId>& nodes_refs) {
    auto it_way = cache.ways.find(osm_id);
    if (it_way == cache.ways.end()) {
        return;
    }
    for (auto osm_id : nodes_refs) {
        auto v = cache.nodes.insert(std::make_pair(osm_id, OSMNode()));
        it_way->second.add_node(v.first);
    }
}

/*
 * We fill needed nodes with their coordinates
 */
void ReadNodesVisitor::node_callback(OSMId osm_id, double lon, double lat, const Hove::Tags& tags) {
    auto node_it = cache.nodes.find(osm_id);
    if (node_it != cache.nodes.end()) {
        node_it->second.set_coord(lon, lat);
        node_it->second.postal_code = find_or_default("addr:postcode", tags);
    }
}

/*
 *  Builds geometries of relations
 */
void OSMCache::build_relations_geometries() {
    for (auto& relation : relations) {
        relation.second.build_geometry(*this, relation.first);
        if (relation.second.polygon.empty()) {
            continue;
        }
        boost::geometry::model::box<point> box{};
        boost::geometry::envelope(relation.second.polygon, box);
        Rect r(box.min_corner().get<0>(), box.min_corner().get<1>(), box.max_corner().get<0>(),
               box.max_corner().get<1>());
        admin_tree.Insert(r.min, r.max, &relation.second);
    }
}

void OSMCache::build_postal_codes() {
    for (auto relation : relations) {
        if (relation.second.level != 9) {
            continue;
        }
        auto rel = match_coord_admin(relation.second.centre.get<0>(), relation.second.centre.get<1>());
        if (rel) {
            rel->postal_codes.insert(relation.second.postal_codes.begin(), relation.second.postal_codes.end());
        }
    }
}

OSMRelation* OSMCache::match_coord_admin(const double lon, const double lat) {
    Rect search_rect(lon, lat);
    const auto p = point(lon, lat);
    using relations = std::vector<OSMRelation*>;

    relations result;
    auto callback = [](OSMRelation* rel, void* c) -> bool {
        if (rel->level == 8) {  // we want to match only cities
            auto* context = reinterpret_cast<relations*>(c);
            context->push_back(rel);
        }
        return true;
    };
    admin_tree.Search(search_rect.min, search_rect.max, callback, &result);
    for (auto rel : result) {
        if (boost::geometry::within(p, rel->polygon)) {
            return rel;
        }
    }
    return nullptr;
}

/*
 * Insert relations into the database
 */
void OSMCache::insert_relations() {
    this->lotus.exec("TRUNCATE  administrative_regions CASCADE;");
    lotus.prepare_bulk_insert("administrative_regions",
                              {"id", "name", "post_code", "insee", "level", "coord", "boundary", "uri"});
    size_t nb_empty_polygons = 0;
    for (auto relation : relations) {
        if (!relation.second.polygon.empty()) {
            std::stringstream polygon_stream;
            polygon_stream << bg::wkt<mpolygon_type>(relation.second.polygon);
            std::string polygon_str = polygon_stream.str();
            const auto coord = "POINT(" + std::to_string(relation.second.centre.get<0>()) + " "
                               + std::to_string(relation.second.centre.get<1>()) + ")";

            auto uri = "admin:osm:relation:" + std::to_string(relation.first);
            if (!relation.second.insee.empty()) {
                uri = "admin:fr:" + relation.second.insee;
            }
            lotus.insert({std::to_string(relation.first), relation.second.name, relation.second.postal_code(),
                          relation.second.insee, std::to_string(relation.second.level), coord, polygon_str, uri});
        } else {
            ++nb_empty_polygons;
        }
    }
    lotus.finish_bulk_insert();
    auto logger = log4cplus::Logger::getInstance("log");
    LOG4CPLUS_INFO(logger,
                   "Ignored " << std::to_string(nb_empty_polygons) << " admins because their polygons were empty");
}

std::string OSMNode::to_geographic_point() const {
    std::stringstream geog;
    geog << std::setprecision(10) << "POINT(" << coord_to_string() << ")";
    return geog.str();
}

OSMRelation::OSMRelation(std::vector<Hove::Reference> refs,
                         std::string insee,
                         const std::string& postal_code,
                         std::string name,
                         const uint32_t level)
    : references(std::move(refs)), insee(std::move(insee)), name(std::move(name)), level(level) {
    this->add_postal_code(postal_code);
}

void OSMRelation::add_postal_code(const std::string& postal_code) {
    if (postal_code.empty()) {
        return;
    }
    if (postal_code.find(';', 0) == std::string::npos) {
        this->postal_codes.insert(postal_code);
    } else {
        boost::split(this->postal_codes, postal_code, boost::is_any_of(";"));
    }
}

std::string OSMRelation::postal_code() const {
    if (postal_codes.empty()) {
        return "";
    }
    if (postal_codes.size() == 1) {
        return *postal_codes.begin();
    }
    return *postal_codes.begin() + "-" + *postal_codes.rbegin();
}

/*
 * We build the polygon of the admin
 * Ways are supposed to be order, but they're not always.
 * Also we may have to reverse way before adding them into the polygon
 */
void OSMRelation::build_polygon(OSMCache& cache, OSMId osm_id) {
    std::set<u_int64_t> explored_ids;
    auto is_outer_way = [](const Hove::Reference& r) {
        return r.member_type == OSMPBF::Relation_MemberType::Relation_MemberType_WAY
               && contains({"outer", "enclave", ""}, r.role);
    };
    auto pickable_way = [&](const Hove::Reference& r) {
        return is_outer_way(r) && explored_ids.count(r.member_id) == 0;
    };
    // We need to explore every node because a boundary can be made in several parts
    while (explored_ids.size() != this->references.size()) {
        // We pickup one way
        auto ref = boost::find_if(references, pickable_way);
        if (ref == references.end()) {
            break;
        }
        auto it_first_way = cache.ways.find(ref->member_id);
        if (it_first_way == cache.ways.end() || it_first_way->second.nodes.empty()) {
            break;
        }
        auto first_node = it_first_way->second.nodes.front()->first;
        auto next_node = it_first_way->second.nodes.back();
        explored_ids.insert(ref->member_id);
        polygon_type tmp_polygon;
        for (auto node : it_first_way->second.nodes) {
            if (!node->second.is_defined()) {
                continue;
            }
            const auto p = point(node->second.lon(), node->second.lat());
            tmp_polygon.outer().push_back(p);
        }

        // We try to find a closed ring
        while (first_node != next_node->first) {
            // We look for a way that begin or end by the last node
            ref = boost::find_if(references, [&](Hove::Reference& r) {
                if (!pickable_way(r)) {
                    return false;
                }
                auto it = cache.ways.find(r.member_id);
                return it != cache.ways.end() && !it->second.nodes.empty()
                       && (it->second.nodes.front()->first == next_node->first
                           || it->second.nodes.back()->first == next_node->first);
            });
            if (ref == references.end()) {
                break;
            }
            explored_ids.insert(ref->member_id);
            auto next_way = cache.ways[ref->member_id];
            if (next_way.nodes.front()->first != next_node->first) {
                boost::reverse(next_way.nodes);
            }
            for (auto node : next_way.nodes) {
                if (!node->second.is_defined()) {
                    continue;
                }
                const auto p = point(node->second.lon(), node->second.lat());
                tmp_polygon.outer().push_back(p);
            }
            next_node = next_way.nodes.back();
        }
        auto log = log4cplus::Logger::getInstance("log");
        if (tmp_polygon.outer().size() < 2 || ref == references.end()) {
            // add some logs
            // some admins are at the boundary of the osm data, so their own boundary may be incomplete
            // some other admins have a split boundary and it is impossible to compute it.
            // to check if the boundary is likely to be split we look for a very near point
            // The admin can also be checked with: http://ra.osmsurround.org/index
            for (const auto& r : references) {
                if (!is_outer_way(r)) {
                    continue;
                }
                auto it_way = cache.ways.find(r.member_id);
                if (it_way == cache.ways.end()) {
                    continue;
                }
                for (const auto& node : it_way->second.nodes) {
                    if (node->first != next_node->first && node->second.almost_equal(next_node->second)) {
                        LOG4CPLUS_WARN(log, "Impossible to close the boundary of the admin "
                                                << name << " (osmid= " << osm_id << "). The end node " << node->first
                                                << " is almost the same as " << next_node->first
                                                << " it's likely that they are wrong duplicate");
                        break;
                    }
                }
            }
            break;
        }

        if (boost::geometry::area(tmp_polygon) < std::numeric_limits<double>::epsilon()) {
            // if area is negative, the polygon needs to be reversed to be valid in OGC norm
            boost::geometry::reverse(tmp_polygon);
            if (boost::geometry::area(tmp_polygon) < std::numeric_limits<double>::epsilon()) {
                // if area is null, it's not a valid shape (refused by PostGIS)
                LOG4CPLUS_WARN(log, "Part or all of the shape of admin " << name << " (insee = " << insee
                                                                         << ") is rejected. Cause: null area.");
                continue;
            }
        }

        const auto front = tmp_polygon.outer().front();
        const auto back = tmp_polygon.outer().back();

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        // This should not happen, but does some time :
        // coordinate values must be exactly equal for the shape
        if (front.get<0>() != back.get<0>() || front.get<1>() != back.get<1>()) {
            tmp_polygon.outer().push_back(tmp_polygon.outer().front());
        }
#pragma GCC diagnostic pop
        polygon.push_back(tmp_polygon);
    }
    if ((centre.get<0>() == 0.0 || centre.get<1>() == 0.0) && !polygon.empty()) {
        bg::centroid(polygon, centre);
    }
}

void OSMRelation::build_geometry(OSMCache& cache, OSMId osm_id) {
    for (Hove::Reference ref : references) {
        if (ref.member_type == OSMPBF::Relation_MemberType::Relation_MemberType_NODE) {
            auto node_it = cache.nodes.find(ref.member_id);
            if (node_it == cache.nodes.end()) {
                continue;
            }
            if (!node_it->second.is_defined()) {
                continue;
            }
            if (contains({"admin_centre", "admin_center"}, ref.role)) {
                set_centre(node_it->second.lon(), node_it->second.lat());
                this->add_postal_code(node_it->second.postal_code);
                break;
            }
        }
    }
    build_polygon(cache, osm_id);
}
}  // namespace cities
}  // namespace navitia

int main(int argc, char** argv) {
    navitia::init_app();
    auto logger = log4cplus::Logger::getInstance("log");
    pt::ptime start;
    std::string input, connection_string;
    po::options_description desc("Allowed options");

    // clang-format off
    desc.add_options()
        ("version,v", "Show version")
        ("help,h", "Show this message")
        ("input,i", po::value<std::string>(&input)->required(), "Input OSM File")
        ("connection-string", po::value<std::string>(&connection_string)->required(),
         "Database connection parameters: host=localhost user=navitia dbname=navitia password=navitia");
    // clang-format on

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);

    if (vm.count("version")) {
        std::cout << argv[0] << " " << navitia::config::project_version << " " << navitia::config::navitia_build_type
                  << std::endl;
        return 0;
    }

    if (vm.count("help")) {
        std::cout << "This program import all cities of an OSM file into a database" << std::endl;
        std::cout << desc << std::endl;
        return 1;
    }

    start = pt::microsec_clock::local_time();
    po::notify(vm);

    navitia::cities::OSMCache cache(connection_string);
    navitia::cities::ReadRelationsVisitor relations_visitor(cache);
    Hove::read_osm_pbf(input, relations_visitor);
    navitia::cities::ReadWaysVisitor ways_visitor(cache);
    Hove::read_osm_pbf(input, ways_visitor);
    navitia::cities::ReadNodesVisitor node_visitor(cache);
    Hove::read_osm_pbf(input, node_visitor);
    cache.build_relations_geometries();
    cache.build_postal_codes();
    cache.insert_relations();
    return 0;
}
