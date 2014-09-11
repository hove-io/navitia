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

#include "osm2ed.h"
#include <stdio.h>
#include <queue>

#include <iostream>
#include <boost/program_options.hpp>
#include <boost/geometry.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/dynamic_bitset.hpp>

#include "ed/connectors/osm_tags_reader.h"
#include "ed/connectors/osm2ed_utils.h"
#include "ed_persistor.h"
#include "utils/functions.h"
#include "utils/init.h"

#include "config.h"

namespace po = boost::program_options;
namespace pt = boost::posix_time;

namespace ed { namespace connectors {

/*
 * Read relations
 * Stores admins relations and initializes nodes and ways associated to it.
 * Stores relations for associatedStreet, also initializes nodes and ways.
 */
void ReadRelationsVisitor::relation_callback(uint64_t osm_id, const CanalTP::Tags &tags, const CanalTP::References &refs) {
    auto logger = log4cplus::Logger::getInstance("log");
    const auto tmp_admin_level = tags.find("admin_level");

    std::string insee = "", postal_code = "", name = "";
    if(tmp_admin_level != tags.end()) {
        std::vector<std::string> accepted_levels{"8", "9", "10"};
        const auto it_level = std::find(accepted_levels.begin(), accepted_levels.end(),
                tmp_admin_level->second);
        if(it_level == accepted_levels.end()) {
            return;
        }
        for (const CanalTP::Reference& ref : refs) {
            switch(ref.member_type) {
            case OSMPBF::Relation_MemberType::Relation_MemberType_WAY:
                if (ref.role == "outer" || ref.role == "" || ref.role == "exclave") {
                    cache.ways.insert(OSMWay(ref.member_id));
                }
                break;
            case OSMPBF::Relation_MemberType::Relation_MemberType_NODE:
                if (ref.role == "admin_centre" || ref.role == "admin_center") {
                    cache.nodes.insert(OSMNode(ref.member_id));
                }
                break;
            case OSMPBF::Relation_MemberType::Relation_MemberType_RELATION:
                continue;
            }
        }
        if(tags.find("ref:INSEE") != tags.end()){
            insee = tags.at("ref:INSEE");
        }
        if(tags.find("name") != tags.end()){
            name = tags.at("name");
        }
        if(tags.find("addr:postcode") != tags.end()){
            postal_code = tags.at("addr:postcode");
        }
        cache.relations.insert(OSMRelation(osm_id, refs, insee, postal_code, name,
                boost::lexical_cast<uint32_t>(tmp_admin_level->second)));
    } else if(tags.find("type") != tags.end() && tags.at("type") == "associatedStreet") {
        uint64_t way_id = std::numeric_limits<uint64_t>::max();
        std::vector<uint64_t> osm_ids;
        for (const CanalTP::Reference& ref : refs) {
            switch(ref.member_type) {
            case OSMPBF::Relation_MemberType::Relation_MemberType_WAY:
                if(ref.role == "house" || ref.role == "addr:houselink" || ref.role == "address") {
                    osm_ids.push_back(ref.member_id);
                } else if(ref.role == "street" ) {
                    if (way_id == std::numeric_limits<uint64_t>::max()) {
                        way_id = ref.member_id;
                    } else {
                        LOG4CPLUS_TRACE(logger, "Reference " << std::to_string(osm_id)
                                << " has two ways with the role street, and that's WRONG");
                        return;
                    }
                }
                break;
            case OSMPBF::Relation_MemberType::Relation_MemberType_NODE:
                if (ref.role == "house" || ref.role == "addr:houselink" || ref.role == "address") {
                    osm_ids.push_back(ref.member_id);
                }
                break;
            case OSMPBF::Relation_MemberType::Relation_MemberType_RELATION:
                continue;
            }
        }
        if (way_id == std::numeric_limits<uint64_t>::max()) {
            return;
        }
        auto tag_name = tags.find("streetname");
        const std::string name = tag_name != tags.end() ? tag_name->second : "";
        for (const auto id : osm_ids) {
            cache.associated_streets.insert(AssociateStreetRelation(id, way_id, name));
        }
    }
}

/*
 * We stores ways they are streets.
 * We store ids of needed nodes
 */
void ReadWaysVisitor::way_callback(uint64_t osm_id, const CanalTP::Tags &tags, const std::vector<uint64_t> & nodes_refs) {
    const auto properties = parse_way_tags(tags);
    const auto name_it = tags.find("name");
    const bool is_street = properties.any();
    auto it_way = cache.ways.find(OSMWay(osm_id));
    const bool is_used_by_relation = it_way != cache.ways.end(),
               is_hn = tags.find("addr:housenumber") != tags.end(),
               is_poi = tags.find("amenity") != tags.end() || tags.find("leisure") != tags.end();

    if (!is_street && !is_used_by_relation && !is_hn && !is_poi) {
        return;
    }
    if (is_street) {
        const auto name = name_it != tags.end() ? name_it->second : "";
        it_way = cache.ways.insert(OSMWay(osm_id, properties, name)).first;
    } else if(is_used_by_relation) {
        it_way->set_properties(properties);
        if (name_it != tags.end()) {
            it_way->set_name(name_it->second);
        }
    }
    for (auto osm_id : nodes_refs) {
        auto v = cache.nodes.insert(OSMNode(osm_id));
        if (is_street && !v.second) {
            v.first->set_used_more_than_once();
        }
        if (it_way != cache.ways.end()) {
            it_way->add_node(v.first);
        }
    }
}

/*
 * We fill needed nodes with their coordinates
 */
void ReadNodesVisitor::node_callback(uint64_t osm_id, double lon, double lat,
        const CanalTP::Tags& ) {
    auto node_it = cache.nodes.find(OSMNode(osm_id));
    if (node_it != cache.nodes.end()) {
        node_it->set_coord(lon, lat);
    }
}

/*
 *  Builds geometries of relations
 */
void OSMCache::build_relations_geometries() {
    for (const auto& relation : relations) {
        relation.build_geometry(*this);
    }
}

/*
 * Find the admin of coordinates
 */
const OSMRelation* OSMCache::match_coord_admin(const double lon, const double lat) {
    Rect search_rect(lon, lat);
    const auto p = point(lon, lat);

    std::vector<OSMRelation*> result;
    auto callback = [](const OSMRelation* rel, void* vec)->bool{
        reinterpret_cast<std::vector<const OSMRelation*>*>(vec)->push_back(rel);
        return true;
    };
    admin_tree.Search(search_rect.min, search_rect.max, callback, &result);
    for(auto rel : result) {
        if (boost::geometry::within(p, rel->polygon)){
            return rel;
        }
    }
    return nullptr;
}

/*
 * Finds admin of a node
 */
void OSMCache::match_nodes_admin() {
    auto logger = log4cplus::Logger::getInstance("log");
    size_t count_matches = 0;
    for (const auto& node : nodes) {
        if (!node.is_defined() || node.admin) {
            continue;
        }
        node.admin = match_coord_admin(node.lon(), node.lat());
        if (node.admin != nullptr) {
            ++ count_matches;
        }
    }

    LOG4CPLUS_INFO(logger, "" << count_matches << "/" << nodes.size() << " nodes with an admin");
}

/*
 * Insert nodes into the database
 */
void OSMCache::insert_nodes() {
    auto logger = log4cplus::Logger::getInstance("log");
    this->lotus.prepare_bulk_insert("georef.node", {"id","coord"});
    size_t n_inserted = 0;
    const size_t max_n_inserted = 200000;
    for(const auto& node : nodes){
        if (!node.is_defined() || !node.is_used()) {
            continue;
        }
        this->lotus.insert({std::to_string(node.osm_id), node.to_geographic_point()});
        ++n_inserted;
        if ((n_inserted % max_n_inserted) == 0) {
            lotus.finish_bulk_insert();
            LOG4CPLUS_INFO(logger, n_inserted << "/" << nodes.size() << " nodes inserted" );
            this->lotus.prepare_bulk_insert("georef.node", {"id","coord"});
        }
     }
    lotus.finish_bulk_insert();
    LOG4CPLUS_INFO(logger, n_inserted << "/" << nodes.size() << " nodes inserted" );
}

/*
 * Insert ways into the database
 */
void OSMCache::insert_ways(){
    auto logger = log4cplus::Logger::getInstance("log");
    this->lotus.prepare_bulk_insert("georef.way", {"id", "name", "uri"});
    size_t n_inserted = 0;
    const size_t max_n_inserted = 50000;
    for (const auto& way : ways) {
        if (way.way_ref != nullptr && &way != way.way_ref) {
            continue;
        }
        std::vector<std::string> values;
        values.push_back(std::to_string(way.osm_id));
        values.push_back(way.name);
        values.push_back("way:"+std::to_string(way.osm_id));
        this->lotus.insert(values);
        ++n_inserted;
        if ((n_inserted % max_n_inserted) == 0) {
            this->lotus.finish_bulk_insert();
            LOG4CPLUS_INFO(logger, n_inserted << "/" << ways.size() << " ways inserted" );
            this->lotus.prepare_bulk_insert("georef.way", {"id", "name", "uri"});
        }
     }
    this->lotus.finish_bulk_insert();
    LOG4CPLUS_INFO(logger, n_inserted << "/" << ways.size() << " ways inserted" );
}

/*
 * Insert edges of the streetnetwork graph into the database.
 * For that we read ways, if a node of a way is used several times it's a node
 * of the streetnetwork graph, so there's a new edge between the last node and
 * this node.
 */
void OSMCache::insert_edges() {
    auto logger = log4cplus::Logger::getInstance("log");
    lotus.prepare_bulk_insert("georef.edge", {"source_node_id",
            "target_node_id", "way_id", "the_geog", "pedestrian_allowed",
            "cycles_allowed", "cars_allowed"});
    std::stringstream geog;
    geog << std::cout.precision(10);
    size_t n_inserted = 0;
    const size_t max_n_inserted = 20000;
    for (const auto& way : ways) {
        std::set<OSMNode>::iterator prev_node = nodes.end();
        const auto ref_way_id = way.way_ref == nullptr ? way.osm_id : way.way_ref->osm_id;
        for (const auto& node : way.nodes) {
            if (!node->is_defined()) {
                continue;
            }
            if (node->is_used_more_than_once() && prev_node != nodes.end()) {
                // If a node is used more than once, it is an intersection,
                // hence it's a node of the street network graph
                geog << node->coord_to_string() << ")";
                lotus.insert({std::to_string(prev_node->osm_id),
                        std::to_string(node->osm_id), std::to_string(ref_way_id),
                        geog.str(), std::to_string(way.properties[OSMWay::FOOT_FWD]),
                        std::to_string(way.properties[OSMWay::CYCLE_FWD]),
                        std::to_string(way.properties[OSMWay::CAR_FWD])});
                // In most of the case we need the reversal,
                // that'll be wrong for some in case in car
                // We need to work on it
                lotus.insert({std::to_string(node->osm_id),
                        std::to_string(prev_node->osm_id), std::to_string(ref_way_id),
                        geog.str(), std::to_string(way.properties[OSMWay::FOOT_BWD]),
                        std::to_string(way.properties[OSMWay::CYCLE_BWD]),
                        std::to_string(way.properties[OSMWay::CAR_BWD])});
                prev_node = nodes.end();
                n_inserted = n_inserted + 2;
            }
            if (prev_node == nodes.end()) {
                geog.str("");
                geog << "LINESTRING(";
                prev_node = node;
            }
            geog << node->coord_to_string() << ", ";
        }
        if ((n_inserted % max_n_inserted) == 0) {
            lotus.finish_bulk_insert();
            LOG4CPLUS_INFO(logger, n_inserted << " edges inserted" );
            lotus.prepare_bulk_insert("georef.edge", {"source_node_id",
                    "target_node_id", "way_id", "the_geog", "pedestrian_allowed",
                    "cycles_allowed", "cars_allowed"});
        }
    }
    lotus.finish_bulk_insert();
    LOG4CPLUS_INFO(logger, n_inserted << " edges inserted" );
}

/*
 * Insert relations into the database
 */
void OSMCache::insert_relations() {
    lotus.prepare_bulk_insert("georef.admin", {"id", "name", "post_code", "insee", "level", "coord", "boundary", "uri"});
    size_t nb_empty_polygons = 0 ;
    for (auto relation : relations) {
        if(!relation.polygon.empty()){
            std::stringstream polygon_stream;
            polygon_stream <<  bg::wkt<mpolygon_type>(relation.polygon);
            std::string polygon_str = polygon_stream.str();
            const auto coord = "POINT(" + std::to_string(relation.centre.get<0>())
                + " " + std::to_string(relation.centre.get<1>()) + ")";
            lotus.insert({std::to_string(relation.osm_id), relation.name,
                          relation.postal_code, relation.insee,
                          std::to_string(relation.level), coord, polygon_str,
                          "admin:"+std::to_string(relation.osm_id)});
        } else {
            ++nb_empty_polygons;
        }
    }
    lotus.finish_bulk_insert();
    auto logger = log4cplus::Logger::getInstance("log");
    LOG4CPLUS_INFO(logger, "Ignored " << std::to_string(nb_empty_polygons) << " admins because their polygons were empty");
}


std::string OSMNode::to_geographic_point() const{
    std::stringstream geog;
    geog << std::setprecision(10)<<"POINT("<< coord_to_string() <<")";
    return geog.str();
}

/*
 * We build a map that have for key the way name, and for value
 * a map admin->vector of way
 */
void OSMCache::build_way_map() {
    constexpr double max_double = std::numeric_limits<double>::max();
    for (auto way_it = ways.begin(); way_it != ways.end(); ++way_it) {
        double max_lon = max_double,
               max_lat = max_double,
               min_lon = max_double,
               min_lat = max_double;
        for (auto node : way_it->nodes) {
            if (!node->admin) {
                continue;
            }
            way_admin_map[way_it->name][node->admin].push_back(way_it);
            if (!node->is_defined()) {
                continue;
            }
            max_lon = max_lon == max_double ? node->lon() :
                                             std::max(max_lon, node->lon());
            max_lat = max_lat == max_double ? node->lat() :
                                             std::max(max_lat, node->lat());
            min_lon = std::min(min_lon, node->lon());
            min_lat = std::min(min_lat, node->lat());
        }
        bg::simplify(way_it->ls, way_it->ls, 0.5);
        if (max_lon == max_double || max_lat == max_double || min_lat == max_double
            || min_lon == max_double) {
            continue;
        }
        Rect r(min_lon, min_lat, max_lon, max_lat);
        way_tree.Insert(r.min, r.max, way_it);
    }
}

const OSMWay* get_way(const OSMWay* w) {
    while (w != nullptr && w->way_ref != nullptr && w->way_ref->osm_id != w->osm_id) {
        w = w->way_ref;
    }
    return w;
}

/*
 * We group together ways with the same name in the same admin
 */
void OSMCache::fusion_ways() {
    for (auto name_admin_ways : way_admin_map) {
        if (name_admin_ways.first == "") {
            continue;
        }
        for (auto admin_ways : name_admin_ways.second) {
            // This shouldn't happen, but... you know ...
            if (admin_ways.second.empty()) {
                continue;
            }
            auto it_ref = std::find_if(admin_ways.second.begin(), admin_ways.second.end(),
                    [](it_way w) { return w->way_ref != nullptr; });
            if (it_ref == admin_ways.second.end()) {
                it_ref = admin_ways.second.begin();
            }
            auto way_ref = (*it_ref)->way_ref == nullptr ? &(**it_ref) : (*it_ref)->way_ref;
            std::for_each(admin_ways.second.begin(), admin_ways.second.end(),
                    [&](it_way w) { w->way_ref = way_ref; });
        }
    }
    for (auto& way : ways) {
        way.way_ref = get_way(&way);
    }
}

/*
 * We build the polygon of the admin and insert it in the rtree
 */
void OSMRelation::build_geometry(OSMCache& cache) const {
    polygon_type tmp_polygon;
    for (CanalTP::Reference ref : references) {
        if (ref.member_type == OSMPBF::Relation_MemberType::Relation_MemberType_WAY) {
            auto it = cache.ways.find(OSMWay(ref.member_id));
            if (it == cache.ways.end()) {
                continue;
            }
            if(ref.role != "outer"  && ref.role != "enclave" && ref.role != "") {
                continue;
            }
            for (auto node : it->nodes) {
                if (!node->is_defined()) {
                    continue;
                }
                const auto p = point(float(node->lon()), float(node->lat()));
                tmp_polygon.outer().push_back(p);
            }
        } else if (ref.member_type == OSMPBF::Relation_MemberType::Relation_MemberType_NODE) {
            auto node_it = cache.nodes.find(OSMNode(ref.member_id));
            if (node_it == cache.nodes.end()) {
                continue;
            }
            if (!node_it->is_defined()) {
                continue;
            }
            if (ref.role == "admin_centre") {
                centre = point(float(node_it->lon()), float(node_it->lat()));
            }
        }
    }
    if (tmp_polygon.outer().size() < 2) {
        return;
    }
    if (centre.get<0>() == 0.0 || centre.get<1>() == 0.0) {
        bg::centroid(tmp_polygon, centre);
    }
    boost::geometry::model::box<point> envelope;
    bg::envelope(tmp_polygon, envelope);
    Rect r(envelope.min_corner().get<0>(), envelope.min_corner().get<1>(),
            envelope.max_corner().get<0>(), envelope.max_corner().get<1>());
    cache.admin_tree.Insert(r.min, r.max, this);
    cache.max_search_distance = std::max(cache.max_search_distance, bg::distance(centre, envelope.min_corner()));

    const auto front = tmp_polygon.outer().front();
    const auto back = tmp_polygon.outer().back();
    if (front.get<0>() != back.get<0>() || front.get<1>() != back.get<1>()) {
        tmp_polygon.outer().push_back(tmp_polygon.outer().front());
    }
    polygon.push_back(tmp_polygon);
}

/*
 * We read another time nodes to insert housenumbers and poi
 */
void PoiHouseNumberVisitor::node_callback(uint64_t osm_id, double lon, double lat, const CanalTP::Tags& tags) {
    this->fill_poi(osm_id, tags, lon, lat);
    this->fill_housenumber(osm_id, tags, lon, lat);
    auto logger = log4cplus::Logger::getInstance("log");
    if ((data.pois.size() + house_numbers.size()) >= max_inserts_without_bulk) {
        persistor.insert_pois(data);
        insert_house_numbers();
        LOG4CPLUS_INFO(logger, "" << std::to_string(data.pois.size()) << " pois inserted and " << std::to_string(house_numbers.size()) << " house numbers");
        n_inserted_pois += data.pois.size();
        n_inserted_house_numbers += house_numbers.size();
        house_numbers.clear();
        data.pois.clear();
    }
}


/*
 * We read another time ways to insert housenumbers and poi
 */
void PoiHouseNumberVisitor::way_callback(uint64_t osm_id, const CanalTP::Tags &tags, const std::vector<uint64_t> & refs) {
    if (tags.find("addr:housenumber") == tags.end() &&
            tags.find("leisure") == tags.end() &&
            tags.find("amenity") == tags.end()) {
        return;
    }
    polygon_type tmp_polygon;
    for (auto ref : refs) {
        auto node_it = cache.nodes.find(OSMNode(ref));
        if (node_it == cache.nodes.end() || !node_it->is_defined()) {
            continue;
        }
        const auto p = point(float(node_it->lon()), float(node_it->lat()));
        tmp_polygon.outer().push_back(p);
    }
    if (tmp_polygon.outer().size() <= 2) {
        for (auto ref_id : refs) {
            auto node_it = cache.nodes.find(OSMNode(ref_id));
            if (node_it != cache.nodes.end() && node_it->is_defined()) {
                this->fill_housenumber(osm_id, tags, node_it->lon(), node_it->lat());
                this->fill_poi(osm_id, tags, node_it->lon(), node_it->lat());
                break;
            }
        }
    } else {
        boost::geometry::model::box<point> envelope;
        point centre;
        bg::envelope(tmp_polygon, envelope);
        bg::centroid(tmp_polygon, centre);
        this->fill_housenumber(osm_id, tags, centre.get<0>(), centre.get<1>());
        this->fill_poi(osm_id, tags, centre.get<0>(), centre.get<1>());
    }
    auto logger = log4cplus::Logger::getInstance("log");
    if ((data.pois.size() + house_numbers.size()) >= max_inserts_without_bulk) {
        persistor.insert_pois(data);
        insert_house_numbers();
        LOG4CPLUS_INFO(logger, "" << std::to_string(data.pois.size()) << " pois inserted and " << std::to_string(house_numbers.size()) << " house numbers");
        n_inserted_pois += data.pois.size();
        n_inserted_house_numbers += house_numbers.size();
        house_numbers.clear();
        data.pois.clear();
    }
}

void PoiHouseNumberVisitor::finish() {
    persistor.insert_pois(data);
    persistor.insert_poi_properties(data);
    insert_house_numbers();
}
/*
 * Insertion of house numbers
 */
void PoiHouseNumberVisitor::insert_house_numbers() {
    persistor.lotus.prepare_bulk_insert("georef.house_number", {"coord", "number", "left_side", "way_id"});
    for(const auto& hn : house_numbers) {
        std::string way_id("NULL");
        if(hn.way != nullptr){
            way_id = hn.way->way_ref == nullptr ? std::to_string(hn.way->osm_id):
                                                  std::to_string(hn.way->way_ref->osm_id);
        }
        std::string str_point = "POINT(";
        str_point += std::to_string(hn.lon);
        str_point += " ";
        str_point += std::to_string(hn.lat);
        str_point += ")";
        persistor.lotus.insert({str_point, std::to_string(hn.number),
                std::to_string(hn.number % 2 == 0), way_id});
    }
    persistor.lotus.finish_bulk_insert();
}

/*
 * We find the admin of the node, and we try to attach the node to the closest
 * way
 */
const OSMWay* PoiHouseNumberVisitor::find_way_without_name(const double lon, const double lat) {
    const OSMWay* result = nullptr;
    const auto admin = cache.match_coord_admin(lon, lat);
    if (!admin) {
        return result;
    }
    double distance = 500;
    Rect search_rect(lon, lat);
    std::vector<it_way> res_tree;
    auto callback = [](it_way w, void* vec)->bool{
        reinterpret_cast<std::vector<it_way>*>(vec)->push_back(w);
        return true;
    };
    cache.way_tree.Search(search_rect.min, search_rect.max, callback, &res_tree);
    point p(lon, lat);
    for(auto way_it : res_tree) {
        auto admins = way_it->admins();
        if (admins.find(admin) != admins.end()) {
            const auto tmp_dist = way_it->distance(p);
            ++ cache.NB_PROJ;
            if (tmp_dist < distance) {
                result = way_it->way_ref==nullptr ? &*way_it : way_it->way_ref;
                distance = tmp_dist;
                if (distance <= 10) {
                     break;
                }
            }
        }
    }
    return result;
}

const OSMWay* PoiHouseNumberVisitor::find_way(const CanalTP::Tags& tags, const double lon,
        const double lat) {

    auto it_street = tags.find("addr:street");
    if (it_street == tags.end()) {
        return find_way_without_name(lon, lat);
    }
    auto it_ways = cache.way_admin_map.find(it_street->second);
    if (it_ways == cache.way_admin_map.end()) {
        return nullptr;
    }
    // If we have a postcode we try to find an admin with this postcode
    // and a way with the same name
    auto it_postcode = tags.find("addr:postcode");
    if (it_postcode != tags.end()) {
        if (it_ways == cache.way_admin_map.end()) {
            return nullptr;
        }
        auto ways_it = std::find_if(it_ways->second.begin(), it_ways->second.end(),
                [&](const std::pair<const OSMRelation*, std::vector<it_way>> r) {
                    return r.first->postal_code == it_postcode->second;
                }
        );
        if (ways_it != it_ways->second.end()) {
            auto way_it = std::find_if(ways_it->second.begin(), ways_it->second.end(),
                    [](const it_way w) { return w->way_ref->osm_id == w->osm_id;});
            if (way_it != ways_it->second.end()) {
                return &**way_it;
            }
        }
    }
    // Otherwize we try to match coords with an admin
    const auto admin = cache.match_coord_admin(lon, lat);
    if (admin) {
        auto it_admin = it_ways->second.find(admin);
        if (it_admin != it_ways->second.end()) {
            return &*it_admin->second.front();
        }
    }

    // If it didn't work we find the closest way to this point with the same name
    double min_distance = std::numeric_limits<double>::max();
    const OSMWay* candidate_way = nullptr;
    point p(lon, lat);
    for (const auto admin_ways : it_ways->second) {
        for (const auto way_it : admin_ways.second) {
            const auto& way = *way_it;
            const auto distance = way.distance(p);
            if (distance < min_distance) {
                min_distance = distance;
                candidate_way = &way;
            }
        }
    }
    return candidate_way;
}

void PoiHouseNumberVisitor::fill_housenumber(const uint64_t osm_id,
        const CanalTP::Tags& tags, const double lon, const double lat) {
    auto it_hn = tags.find("addr:housenumber");
    if (it_hn == tags.end()) {
        return ;
    }
    const OSMWay *candidate_way = nullptr;
    auto asso_it = cache.associated_streets.find(AssociateStreetRelation(osm_id)) ;
    if (asso_it != cache.associated_streets.end()) {
        auto it_cache = cache.ways.find(OSMWay(asso_it->way_id));
        if(it_cache != cache.ways.end()){
            candidate_way = &*it_cache;
        }
    } else {
        candidate_way = find_way(tags, lon, lat);
    }
    if (candidate_way == nullptr) {
        return;
    }
    house_numbers.push_back(OSMHouseNumber(str_to_int(it_hn->second), lon, lat, candidate_way->way_ref));
}

void PoiHouseNumberVisitor::fill_poi(const u_int64_t osm_id, const CanalTP::Tags& tags,
        const double lon, const double lat) {
    std::string ref_tag = "";
    if (tags.find("amenity") != tags.end()) {
            ref_tag = "amenity";
    }
    if (tags.find("leisure") != tags.end()) {
            ref_tag = "leisure";
    }
    if (ref_tag.empty()) {
        return;
    }
    auto poi_it = data.pois.find(std::to_string(osm_id));
    if (poi_it != data.pois.end()){
        return;
    }
    std::string value = tags.at(ref_tag);
    auto it = data.poi_types.find(value);
    if (it == data.poi_types.end()) {
        return;
    }
    ed::types::Poi poi;
    poi.poi_type = it->second;
    if(tags.find("name") != tags.end()){
        poi.name = tags.at("name");
    }
    for(auto st : tags){
        if(properties_to_ignore.find(st.first) != properties_to_ignore.end()){
            if(st.first == "addr:housenumber"){
                poi.address_number = st.second;
            }
            if(st.first == "addr:street"){
                poi.address_name = st.second;
            }
        }else{
            poi.properties[st.first] = st.second;
        }
    }
    poi.id = this->n_inserted_pois + data.pois.size();
    poi.coord.set_lon(lon);
    poi.coord.set_lat(lat);
    data.pois[std::to_string(osm_id)] = poi;
}

void OSMCache::flag_nodes() {
    for (const auto& way : ways) {
        for (const auto node: way.nodes) {
            if(node->is_defined()) {
                node->set_first_or_last();
                break;
            }
        }
        for (auto it=way.nodes.rbegin(); it!=way.nodes.rend(); ++it) {
            if ((*it)->is_defined()) {
                (*it)->set_first_or_last();
                break;
            }
        }
    }
}

}}

int main(int argc, char** argv) {
    navitia::init_app();
    auto logger = log4cplus::Logger::getInstance("log");
    pt::ptime start;
    std::string input, connection_string;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("version,v", "Affiche la version")
        ("help,h", "Affiche l'aide")
        ("input,i", po::value<std::string>(&input)->required(), "Fichier OSM à utiliser")
        ("connection-string", po::value<std::string>(&connection_string)->required(), "parametres de connexion à la base de données: host=localhost user=navitia dbname=navitia password=navitia");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);

    if(vm.count("version")){
        LOG4CPLUS_INFO(logger, argv[0] << " V" << KRAKEN_VERSION << " " << NAVITIA_BUILD_TYPE);
        return 0;
    }

    if(vm.count("help")) {
        LOG4CPLUS_INFO(logger, desc);
        return 1;
    }

    start = pt::microsec_clock::local_time();
    po::notify(vm);

    ed::EdPersistor persistor(connection_string);
    persistor.clean_georef();
    persistor.clean_poi();

    ed::connectors::OSMCache cache(connection_string);
    ed::connectors::ReadRelationsVisitor relations_visitor(cache);
    CanalTP::read_osm_pbf(input, relations_visitor);
    ed::connectors::ReadWaysVisitor ways_visitor(cache);
    CanalTP::read_osm_pbf(input, ways_visitor);
    ed::connectors::ReadNodesVisitor node_visitor(cache);
    CanalTP::read_osm_pbf(input, node_visitor);
    cache.build_relations_geometries();
    cache.match_nodes_admin();
    cache.build_way_map();
    cache.fusion_ways();
    cache.flag_nodes();
    cache.insert_nodes();
    cache.insert_ways();
    cache.insert_edges();
    cache.insert_relations();

    ed::Georef data;
    ed::connectors::PoiHouseNumberVisitor poi_visitor(persistor, cache, data);
    CanalTP::read_osm_pbf(input, poi_visitor);
    poi_visitor.finish();
    return 0;
}
