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

#include "fusio_parser.h"
#include "type/address_from_ntfs.h"

#include <boost/geometry.hpp>
#include <boost/filesystem.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace ed {
namespace connectors {

namespace nm = ed::types;

template <typename C>
typename C::mapped_type get_object(const C& map, const std::string& obj_id, const std::string& property) {
    log4cplus::Logger logger = log4cplus::Logger::getInstance("log");
    const auto o = find_or_default(obj_id, map);
    if (!o) {
        LOG4CPLUS_INFO(logger, "impossible to find " << obj_id << ", cannot add the " << property << ", skipping line");
    }
    return o;
}
/*
Add default dataset for all contributor without any dataset
*/
static void default_datasets(GtfsData& gdata, Data& data) {
    for (auto* contributor : data.contributors) {
        bool exist = false;
        for (const auto dataset : data.datasets) {
            if (dataset->contributor->uri == contributor->uri) {
                exist = true;
                break;
            }
        }
        if (!exist) {
            auto dataset = std::make_unique<ed::types::Dataset>();
            dataset->contributor = contributor;
            dataset->uri = "default_dataset:" + contributor->uri;
            dataset->validation_period = data.meta.production_date;
            dataset->desc = "default dataset: " + contributor->name;
            dataset->idx = data.datasets.size() + 1;
            data.datasets.push_back(dataset.get());
            gdata.dataset_map[dataset->uri] = dataset.get();
            dataset.release();
        }
    }
}

static ed::types::Dataset* get_dataset(GtfsData& gdata, const std::string& dataset_id) {
    if (!dataset_id.empty()) {
        auto it_dataset = gdata.dataset_map.find(dataset_id);
        if (it_dataset != gdata.dataset_map.end()) {
            return it_dataset->second;
        }
    }
    return nullptr;
}

void FeedInfoFusioHandler::init(Data&) {
    feed_info_param_c = csv.get_pos_col("feed_info_param");
    feed_info_value_c = csv.get_pos_col("feed_info_value");
}

void FeedInfoFusioHandler::handle_line(Data& data, const csv_row& row, bool) {
    if (is_valid(feed_info_param_c, row) && has_col(feed_info_value_c, row)) {
        if (row[feed_info_param_c] == "feed_creation_date") {
            try {
                feed_creation_date =
                    boost::make_optional(boost::gregorian::from_undelimited_string(row[feed_info_value_c]));
            } catch (const std::out_of_range&) {
                LOG4CPLUS_INFO(logger, "feed_creation_date is not valid");
            }
        } else if (row[feed_info_param_c] == "feed_creation_time") {
            try {
                feed_creation_time =
                    boost::make_optional(boost::posix_time::duration_from_string(row[feed_info_value_c]));
            } catch (const std::out_of_range&) {
                LOG4CPLUS_INFO(logger, "feed_creation_time is not valid");
            }
        } else {
            data.add_feed_info(row[feed_info_param_c], row[feed_info_value_c]);
        }
    }
}

void FeedInfoFusioHandler::finish(Data& data) {
    if (feed_creation_date && feed_creation_time) {
        boost::posix_time::ptime creation_datetime(*feed_creation_date, *feed_creation_time);
        data.add_feed_info("feed_creation_datetime", boost::posix_time::to_iso_string(creation_datetime));
    }
}

void AgencyFusioHandler::init(Data& data) {
    AgencyGtfsHandler::init(data);
    if (id_c == -1) {
        id_c = csv.get_pos_col("network_id");
    }
    if (name_c == -1) {
        name_c = csv.get_pos_col("network_name");
    }
    if (time_zone_c == -1) {
        time_zone_c = csv.get_pos_col("network_timezone");
    }
    ext_code_c = csv.get_pos_col("external_code");
    if (ext_code_c == -1) {
        ext_code_c = csv.get_pos_col("network_external_code");
    }
    sort_c = csv.get_pos_col("agency_sort");
    if (sort_c == -1) {
        sort_c = csv.get_pos_col("network_sort");
    }
    if (sort_c == -1) {
        sort_c = csv.get_pos_col("network_sort_order");
    }
    agency_url_c = csv.get_pos_col("agency_url");
    if (agency_url_c == -1) {
        agency_url_c = csv.get_pos_col("network_url");
    }
}

void AgencyFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    if (!is_valid(id_c, row)) {
        LOG4CPLUS_WARN(logger, "AgencyFusioHandler : Invalid agency id " << row[id_c]);
        return;
    }

    auto* network = new ed::types::Network();
    network->uri = row[id_c];

    if (is_valid(ext_code_c, row)) {
        data.add_object_code(network, row[ext_code_c]);
    }

    network->name = row[name_c];

    if (is_valid(sort_c, row)) {
        network->sort = boost::lexical_cast<int>(row[sort_c]);
    }

    if (is_valid(agency_url_c, row)) {
        network->website = row[agency_url_c];
    }

    data.networks.push_back(network);
    gtfs_data.network_map[network->uri] = network;

    std::string timezone_name = row[time_zone_c];

    if (!is_first_line) {
        // we created a default agency, with a default time zone, but this will be overidden if we read at least one
        // agency
        if (data.tz_wrapper.tz_name != timezone_name) {
            LOG4CPLUS_WARN(logger, "Error while reading " << csv.filename
                                                          << " all the time zone are not equals, only the first one "
                                                             "will be considered as the default timezone");
        }
        return;
    }

    if (timezone_name.empty()) {
        throw navitia::exception("Error while reading " + csv.filename + +" timezone is empty for agency "
                                 + network->uri);
    }

    auto tz = gtfs_data.tz.tz_db.time_zone_from_region(timezone_name);
    if (!tz) {
        throw navitia::exception("Error while reading " + csv.filename + +" timezone " + timezone_name
                                 + " is not valid for agency " + network->uri);
    }
    data.tz_wrapper = ed::EdTZWrapper(timezone_name, tz);
    LOG4CPLUS_INFO(logger, "default agency tz " << data.tz_wrapper.tz_name << " -> " << tz->std_zone_name());
}

void StopsFusioHandler::init(Data& data) {
    StopsGtfsHandler::init(data);
    ext_code_c = csv.get_pos_col("external_code");
    // TODO equipment_id NTFSv0.4: remove property_id when we stop to support NTFSv0.3
    property_id_c = csv.get_pos_col("property_id");
    if (property_id_c == -1) {
        property_id_c = csv.get_pos_col("equipment_id");
    }
    comment_id_c = csv.get_pos_col("comment_id");
    visible_c = csv.get_pos_col("visible");
    geometry_id_c = csv.get_pos_col("geometry_id");
    // Since NTFS v0.9, using fare_zone_id (if absent, fallback to zone_id)
    zone_c = csv.get_pos_col("fare_zone_id");
    if (zone_c == UNKNOWN_COLUMN) {
        zone_c = csv.get_pos_col("zone_id");
    }
    address_id_c = csv.get_pos_col("address_id");
}

// in fusio we want to delete all stop points without stop area
void StopsFusioHandler::handle_stop_point_without_area(Data& data) {
    // Deletion of the stop point without stop areas
    std::vector<size_t> erase_sp;
    for (int i = data.stop_points.size() - 1; i >= 0; --i) {
        if (data.stop_points[i]->stop_area == nullptr) {
            erase_sp.push_back(i);
        }
    }
    int num_elements = data.stop_points.size();
    for (size_t to_erase : erase_sp) {
        gtfs_data.stop_point_map.erase(data.stop_points[to_erase]->uri);
        delete data.stop_points[to_erase];
        data.stop_points[to_erase] = data.stop_points[num_elements - 1];
        num_elements--;
    }
    data.stop_points.resize(num_elements);
    LOG4CPLUS_INFO(logger, "Deletion of " << erase_sp.size() << " stop_point wihtout stop_area");
}

nm::StopArea* StopsFusioHandler::build_stop_area(Data& data, const csv_row& row) {
    auto* sa = StopsGtfsHandler::build_stop_area(data, row);
    if (!sa) {
        return nullptr;
    }

    if (is_valid(ext_code_c, row)) {
        data.add_object_code(sa, row[ext_code_c]);
    }
    if (is_valid(property_id_c, row)) {
        auto it_property = gtfs_data.hasProperties_map.find(row[property_id_c]);
        if (it_property != gtfs_data.hasProperties_map.end()) {
            sa->set_properties(it_property->second.properties());
        }
    }

    if (is_valid(comment_id_c, row)) {
        auto it_comment = data.comment_by_id.find(row[comment_id_c]);
        if (it_comment != data.comment_by_id.end()) {
            data.add_pt_object_comment(sa, row[comment_id_c]);
        }
    }

    if (is_valid(visible_c, row)) {
        sa->visible = (row[visible_c] == "1");
    }
    return sa;
}

nm::StopPoint* StopsFusioHandler::build_stop_point(Data& data, const csv_row& row) {
    auto* sp = StopsGtfsHandler::build_stop_point(data, row);
    if (!sp) {
        return nullptr;
    }

    if (is_valid(ext_code_c, row)) {
        data.add_object_code(sp, row[ext_code_c]);
    }
    if (is_valid(property_id_c, row)) {
        auto it_property = gtfs_data.hasProperties_map.find(row[property_id_c]);
        if (it_property != gtfs_data.hasProperties_map.end()) {
            sp->set_properties(it_property->second.properties());
        }
    }

    if (is_valid(comment_id_c, row)) {
        auto it_comment = data.comment_by_id.find(row[comment_id_c]);
        if (it_comment != data.comment_by_id.end()) {
            data.add_pt_object_comment(sp, row[comment_id_c]);
        }
    }
    if (is_valid(address_id_c, row)) {
        sp->address_id = row[address_id_c];
    }

    if (has_col(type_c, row) && row[type_c] == "2") {
        sp->is_zonal = true;
    }
    if (is_valid(geometry_id_c, row)) {
        const auto search = data.areas.find(row.at(geometry_id_c));
        if (search != data.areas.end()) {
            sp->area = search->second;
        } else {
            LOG4CPLUS_WARN(logger, "geometry_id " << row.at(geometry_id_c) << " not found");
        }
    }

    return sp;
}

nm::AccessPoint* StopsFusioHandler::build_access_points(Data& data, const csv_row& row) {
    auto* ap = StopsGtfsHandler::build_access_point(data, row);
    if (is_valid(visible_c, row)) {
        ap->visible = (row[visible_c] == "1");
    }
    // length
    if (has_col(code_c, row) && row[code_c] != "") {
        ap->stop_code = row[code_c];
    }
    return ap;
}

StopsGtfsHandler::stop_point_and_area StopsFusioHandler::handle_line(Data& data, const csv_row& row, bool) {
    stop_point_and_area return_wrapper{};

    if (is_duplicate(row)) {
        return return_wrapper;
    }
    // location_type == 1 => StopArea
    if (has_col(type_c, row) && row[type_c] == "1") {
        auto* sa = build_stop_area(data, row);
        if (sa != nullptr) {
            return_wrapper.second = sa;
        }
    } else if (has_col(type_c, row) && navitia::contains({"0", "2"}, row[type_c])) {
        // location_type == 0 => StopPoint
        // location_type == 2 => geographical zone, handled like stop points
        auto* sp = build_stop_point(data, row);
        if (sp) {
            return_wrapper.first = sp;
        }
        // Handle I/O case
    } else if (has_col(type_c, row) && row[type_c] == "3") {
        build_access_points(data, row);
        return {};
    } else {
        // we ignore pathways nodes
        return {};
    }

    return return_wrapper;
}

void PathWayFusioHandler::init(Data& data) {
    PathWayGtfsHandler::init(data);
}

void PathWayFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    PathWayGtfsHandler::handle_line(data, row, is_first_line);
}

void RouteFusioHandler::init(Data&) {
    ext_code_c = csv.get_pos_col("external_code");
    route_id_c = csv.get_pos_col("route_id");
    route_name_c = csv.get_pos_col("route_name");
    direction_type_c = csv.get_pos_col("direction_type");
    line_id_c = csv.get_pos_col("line_id");
    comment_id_c = csv.get_pos_col("comment_id");
    geometry_id_c = csv.get_pos_col("geometry_id");
    destination_id_c = csv.get_pos_col("destination_id");
    ignored = 0;
}

void RouteFusioHandler::handle_line(Data& data, const csv_row& row, bool) {
    if (gtfs_data.route_map.find(row[route_id_c]) != gtfs_data.route_map.end()) {
        ignored++;
        LOG4CPLUS_WARN(logger, "duplicate on route line " + row[route_id_c]);
        return;
    }
    ed::types::Line* ed_line = nullptr;
    auto it_line = gtfs_data.line_map.find(row[line_id_c]);
    if (it_line != gtfs_data.line_map.end()) {
        ed_line = it_line->second;
    } else {
        ignored++;
        LOG4CPLUS_WARN(logger, "Route orphan " + row[route_id_c]);
        return;
    }
    auto* ed_route = new ed::types::Route();
    ed_route->line = ed_line;
    ed_route->uri = row[route_id_c];

    if (is_valid(ext_code_c, row)) {
        data.add_object_code(ed_route, row[ext_code_c]);
    }

    ed_route->name = row[route_name_c];
    if (is_valid(direction_type_c, row)) {
        ed_route->direction_type = row[direction_type_c];
    }

    if (is_valid(comment_id_c, row)) {
        auto it_comment = data.comment_by_id.find(row[comment_id_c]);
        if (it_comment != data.comment_by_id.end()) {
            data.add_pt_object_comment(ed_route, row[comment_id_c]);
        }
    }
    if (is_valid(geometry_id_c, row))
        ed_route->shape = find_or_default(row.at(geometry_id_c), data.shapes);

    if (is_valid(destination_id_c, row)) {
        const auto search = gtfs_data.stop_area_map.find(row.at(destination_id_c));
        if (search != gtfs_data.stop_area_map.end()) {
            ed_route->destination = search->second;
        } else {
            LOG4CPLUS_WARN(logger, "impossible to find destination " << row.at(destination_id_c) << " for route "
                                                                     << row.at(route_id_c));
        }
    }

    gtfs_data.route_map[row[route_id_c]] = ed_route;
    data.routes.push_back(ed_route);
}

void TransfersFusioHandler::init(Data& d) {
    TransfersGtfsHandler::init(d);
    real_time_c = csv.get_pos_col("real_min_transfer_time");
    property_id_c = csv.get_pos_col("property_id");
    // TODO equipment_id NTFSv0.4: remove property_id when we stop to support NTFSv0.3
    if (property_id_c == -1) {
        property_id_c = csv.get_pos_col("equipment_id");
    }
}

void TransfersFusioHandler::fill_stop_point_connection(ed::types::StopPointConnection* connection,
                                                       const csv_row& row) const {
    TransfersGtfsHandler::fill_stop_point_connection(connection, row);

    if (is_valid(property_id_c, row)) {
        auto it_property = gtfs_data.hasProperties_map.find(row[property_id_c]);
        if (it_property != gtfs_data.hasProperties_map.end()) {
            connection->set_properties(it_property->second.properties());
        }
    }

    if (is_valid(real_time_c, row)) {
        try {
            connection->duration = boost::lexical_cast<int>(row[real_time_c]);
        } catch (const boost::bad_lexical_cast&) {
            LOG4CPLUS_INFO(logger, "impossible to parse real transfers time duration " << row[real_time_c]);
        }
    }
}

void StopTimeFusioHandler::init(Data& data) {
    StopTimeGtfsHandler::init(data);
    itl_c = csv.get_pos_col("local_zone_id");
    desc_c = csv.get_pos_col("stop_desc");
    date_time_estimated_c = csv.get_pos_col("stop_time_precision");
    // For backward compatibilty
    if (date_time_estimated_c == UNKNOWN_COLUMN) {
        is_stop_time_precision = false;
        date_time_estimated_c = csv.get_pos_col("date_time_estimated");
    }
    id_c = csv.get_pos_col("stop_time_id");
    stop_headsign_c = csv.get_pos_col("stop_headsign");
    trip_short_name_at_stop_c = csv.get_pos_col("trip_short_name_at_stop");
    boarding_duration_c = csv.get_pos_col("boarding_duration");
    alighting_duration_c = csv.get_pos_col("alighting_duration");
}

void StopTimeFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    auto stop_times = StopTimeGtfsHandler::handle_line(data, row, is_first_line);
    // gtfs can return many stoptimes for one line because of DST periods
    if (stop_times.empty()) {
        return;
    }
    for (auto stop_time : stop_times) {
        if (is_valid(date_time_estimated_c, row))
            // For backward retrocompatibility
            if (is_stop_time_precision) {
                stop_time->date_time_estimated = (row[date_time_estimated_c] == "2");
            } else {
                stop_time->date_time_estimated = (row[date_time_estimated_c] == "1");
            }
        else
            stop_time->date_time_estimated = false;

        if (is_valid(id_c, row)) {
            // if we have an id, we store the stoptime for futur use
            gtfs_data.stop_time_map[row[id_c]].push_back(stop_time);
        }

        if (is_valid(desc_c, row)) {
            auto it_comment = data.comment_by_id.find(row[desc_c]);
            if (it_comment != data.comment_by_id.end()) {
                data.add_pt_object_comment(stop_time, row[desc_c]);
            }
        }

        if (is_valid(itl_c, row)) {
            auto local_traffic_zone = boost::lexical_cast<uint16_t>(row[itl_c]);
            if (local_traffic_zone > 0) {
                stop_time->local_traffic_zone = local_traffic_zone;
            }
        }

        // stop_headsign value has priority over trip_short_name_at_stop
        if (is_valid(stop_headsign_c, row)) {
            stop_time->headsign = row[stop_headsign_c];
        } else if (is_valid(trip_short_name_at_stop_c, row)) {
            stop_time->headsign = row[trip_short_name_at_stop_c];
        }

        if (is_valid(boarding_duration_c, row)) {
            unsigned int boarding_duration(0);
            try {
                boarding_duration = boost::lexical_cast<unsigned int>(row[boarding_duration_c]);
            } catch (const boost::bad_lexical_cast&) {
                LOG4CPLUS_INFO(logger, "Impossible to parse boarding_duration for stop_time number "
                                           << stop_time->order << " on trip " << stop_time->vehicle_journey->uri
                                           << " : " << stop_time->departure_time << " / " << stop_time->arrival_time
                                           << ". Assuming 0 as boarding duration");
            }
            assert(stop_time->boarding_time == stop_time->departure_time);
            stop_time->boarding_time -= boarding_duration;
        }

        if (is_valid(alighting_duration_c, row)) {
            unsigned int alighting_duration(0);
            try {
                alighting_duration = boost::lexical_cast<unsigned int>(row[alighting_duration_c]);
            } catch (const boost::bad_lexical_cast&) {
                LOG4CPLUS_INFO(logger, "Impossible to parse boarding_duration for stop_time number "
                                           << stop_time->order << " on trip " << stop_time->vehicle_journey->uri
                                           << " : " << stop_time->departure_time << " / " << stop_time->arrival_time
                                           << ". Assuming 0 as boarding duration");
            }
            assert(stop_time->alighting_time == stop_time->arrival_time);
            stop_time->alighting_time += alighting_duration;
        }
    }
}

template <typename T>
static boost::optional<T> read_wkt(const std::string& s) {
    try {
        T g;
        boost::geometry::read_wkt(s, g);
        return g;
    } catch (const boost::geometry::read_wkt_exception&) {
        return boost::none;
    }
}
void GeometriesFusioHandler::init(Data&) {
    LOG4CPLUS_INFO(logger, "Reading geometries");
    geometry_id_c = csv.get_pos_col("geometry_id");
    geometry_wkt_c = csv.get_pos_col("geometry_wkt");
}
void GeometriesFusioHandler::handle_line(Data& data, const csv_row& row, bool) {
    try {
        if (const auto g = read_wkt<nt::MultiLineString>(row.at(geometry_wkt_c))) {
            data.shapes[row.at(geometry_id_c)] = *g;
        } else if (const auto g = read_wkt<nt::LineString>(row.at(geometry_wkt_c))) {
            data.shapes[row.at(geometry_id_c)] = {*g};
        } else if (const auto g = read_wkt<nt::MultiPolygon>(row.at(geometry_wkt_c))) {
            data.areas[row.at(geometry_id_c)] = *g;
        } else {
            LOG4CPLUS_WARN(logger,
                           "Geometry cannot be read as [Multi]Line or MultiPolygon: " << row.at(geometry_wkt_c));
        }
    } catch (const std::exception& e) {
        LOG4CPLUS_WARN(logger, "Geometry cannot be read: " << e.what());
    }
}
void GeometriesFusioHandler::finish(Data& data) {
    LOG4CPLUS_INFO(logger, "Nb shapes: " << data.shapes.size());
}

void TripsFusioHandler::init(Data& d) {
    d.vehicle_journeys.reserve(350000);

    route_id_c = csv.get_pos_col("route_id");
    service_c = csv.get_pos_col("service_id");
    trip_c = csv.get_pos_col("trip_id");
    trip_headsign_c = csv.get_pos_col("trip_headsign");
    block_id_c = csv.get_pos_col("block_id");
    comment_id_c = csv.get_pos_col("comment_id");
    trip_propertie_id_c = csv.get_pos_col("trip_property_id");
    odt_type_c = csv.get_pos_col("odt_type");
    company_id_c = csv.get_pos_col("company_id");
    odt_condition_id_c = csv.get_pos_col("odt_condition_id");
    physical_mode_c = csv.get_pos_col("physical_mode_id");
    ext_code_c = csv.get_pos_col("external_code");
    geometry_id_c = csv.get_pos_col("geometry_id");
    contributor_id_c = csv.get_pos_col("contributor_id");
    dataset_id_c = csv.get_pos_col("dataset_id");
    // TODO remove frame_id when migrated
    if (dataset_id_c == -1) {
        dataset_id_c = csv.get_pos_col("frame_id");
    }
    trip_short_name_c = csv.get_pos_col("trip_short_name");
}

std::vector<ed::types::VehicleJourney*> TripsFusioHandler::get_split_vj(Data& data, const csv_row& row, bool) {
    auto it = gtfs_data.route_map.find(row[route_id_c]);
    if (it == gtfs_data.route_map.end()) {
        LOG4CPLUS_WARN(logger,
                       "Impossible to find the route " + row[route_id_c] + " referenced by trip " + row[trip_c]);
        ignored++;
        return {};
    }
    ed::types::Route* route = it->second;

    auto vp_range = gtfs_data.tz.vp_by_name.equal_range(row[service_c]);
    if (empty(vp_range)) {
        LOG4CPLUS_WARN(logger,
                       "Impossible to find the service " + row[service_c] + " referenced by trip " + row[trip_c]);
        ignored++;
        return {};
    }

    // we look in the meta vj table to see if we already have one such vj
    if (data.meta_vj_map.find(row[trip_c]) != data.meta_vj_map.end()) {
        LOG4CPLUS_DEBUG(logger, "a vj with trip id = " << row[trip_c] << " already read, we skip the other one");
        ignored_vj++;
        return {};
    }
    std::vector<ed::types::VehicleJourney*> res;

    types::MetaVehicleJourney& meta_vj = data.meta_vj_map[row[trip_c]];  // we get a ref on a newly created meta vj
    meta_vj.uri = row[trip_c];
    // Store the meta vj with its external code, only if this code exists
    if (is_valid(ext_code_c, row)) {
        gtfs_data.metavj_by_external_code.insert({row[ext_code_c], &meta_vj});
    }

    // get shape if possible
    const std::string& shape_id = has_col(geometry_id_c, row) ? row.at(geometry_id_c) : "";

    bool has_been_split = more_than_one_elt(vp_range);  // check if the trip has been split over dst

    size_t cpt_vj(1);
    // the validity pattern may have been split because of DST, so we need to create one vj for each
    for (auto vp_it = vp_range.first; vp_it != vp_range.second; ++vp_it, cpt_vj++) {
        navitia::type::ValidityPattern* vp_xx = vp_it->second;

        auto* vj = new ed::types::VehicleJourney();

        const std::string original_uri = row[trip_c];
        std::string vj_uri = original_uri;
        if (has_been_split) {
            // we change the name of the vj (all but the first one) since we had to split the original GTFS vj because
            // of dst
            vj_uri = generate_unique_vj_uri(gtfs_data, original_uri, cpt_vj);
        }
        vj->uri = vj_uri;

        if (is_valid(trip_headsign_c, row)) {
            vj->headsign = row[trip_headsign_c];
        } else {
            vj->headsign = vj->uri;
        }

        if (is_valid(trip_short_name_c, row)) {
            vj->name = row[trip_short_name_c];
        } else if (is_valid(trip_headsign_c, row)) {
            vj->name = row[trip_headsign_c];
        } else {
            vj->name = vj->uri;
        }

        vj->validity_pattern = vp_xx;
        vj->adapted_validity_pattern = vp_xx;
        vj->route = route;
        if (is_valid(block_id_c, row))
            vj->block_id = row[block_id_c];
        else
            vj->block_id = "";

        gtfs_data.tz.vj_by_name.insert({original_uri, vj});

        data.vehicle_journeys.push_back(vj);
        res.push_back(vj);
        meta_vj.theoric_vj.push_back(vj);
        vj->meta_vj_name = row[trip_c];
        vj->shape_id = shape_id;
    }

    return res;
}

void TripsFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    auto split_vj = TripsFusioHandler::get_split_vj(data, row, is_first_line);

    if (split_vj.empty()) {
        return;
    }

    ed::types::Dataset* dataset = nullptr;
    // with NTFS >= 0.7 dataset_id is required
    if (is_valid(dataset_id_c, row)) {
        dataset = get_dataset(gtfs_data, row[dataset_id_c]);
    }

    // the vj might have been split over the dst,thus we loop over all split vj
    for (auto vj : split_vj) {
        if (is_valid(ext_code_c, row)) {
            data.add_object_code(vj, row[ext_code_c]);
        }

        vj->dataset = dataset;
        // if a physical_mode is given we override the value
        vj->physical_mode = nullptr;
        if (is_valid(physical_mode_c, row)) {
            auto itm = gtfs_data.physical_mode_map.find(row[physical_mode_c]);
            if (itm == gtfs_data.physical_mode_map.end()) {
                LOG4CPLUS_WARN(logger, "TripsFusioHandler : Impossible to find the physical mode "
                                           << row[physical_mode_c] << " referenced by trip " << row[trip_c]);
            } else {
                vj->physical_mode = itm->second;
            }
        }

        if (vj->physical_mode == nullptr) {
            vj->physical_mode = gtfs_data.get_or_create_default_physical_mode(data);
        }

        if (is_valid(odt_condition_id_c, row)) {
            auto it_odt_condition = gtfs_data.odt_conditions_map.find(row[odt_condition_id_c]);
            if (it_odt_condition != gtfs_data.odt_conditions_map.end()) {
                vj->odt_message = it_odt_condition->second;
            }
        }

        if (is_valid(trip_propertie_id_c, row)) {
            auto it_property = gtfs_data.hasVehicleProperties_map.find(row[trip_propertie_id_c]);
            if (it_property != gtfs_data.hasVehicleProperties_map.end()) {
                vj->set_vehicles(it_property->second.vehicles());
            }
        }

        if (is_valid(comment_id_c, row)) {
            auto it_comment = data.comment_by_id.find(row[comment_id_c]);
            if (it_comment != data.comment_by_id.end()) {
                data.add_pt_object_comment(vj, row[comment_id_c]);
            }
        }

        if (is_valid(odt_type_c, row)) {
            vj->vehicle_journey_type = static_cast<nt::VehicleJourneyType>(boost::lexical_cast<int>(row[odt_type_c]));
        }

        vj->company = nullptr;
        if (is_valid(company_id_c, row)) {
            auto it_company = gtfs_data.company_map.find(row[company_id_c]);
            if (it_company == gtfs_data.company_map.end()) {
                LOG4CPLUS_WARN(logger, "TripsFusioHandler : Impossible to find the company "
                                           << row[company_id_c] << " referenced by trip " << row[trip_c]);
            } else {
                vj->company = it_company->second;
            }
        }

        if (!vj->company) {
            vj->company = gtfs_data.get_or_create_default_company(data);
        }
    }
}

void TripsFusioHandler::finish(Data&) {
    if (ignored_vj) {
        LOG4CPLUS_WARN(logger, "TripsFusioHandler:" << ignored_vj << " vehicle journeys ignored");
    }
}

void ContributorFusioHandler::init(Data&) {
    id_c = csv.get_pos_col("contributor_id");
    name_c = csv.get_pos_col("contributor_name");
    website_c = csv.get_pos_col("contributor_website");
    license_c = csv.get_pos_col("contributor_license");
}

void ContributorFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    if (!is_first_line && !has_col(id_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename
                                    + "  file has more than one contributor and no contributor_id column");
        throw InvalidHeaders(csv.filename);
    }
    auto* contributor = new ed::types::Contributor();
    if (is_valid(id_c, row)) {
        contributor->uri = row[id_c];
    } else {
        contributor->uri = "default_contributor";
    }
    contributor->name = row[name_c];

    if (is_valid(website_c, row)) {
        contributor->website = row[website_c];
    }

    if (is_valid(license_c, row)) {
        contributor->license = row[license_c];
    }

    contributor->idx = data.contributors.size() + 1;
    data.contributors.push_back(contributor);
    gtfs_data.contributor_map[contributor->uri] = contributor;
}

void FrameFusioHandler::init(Data&) {
    id_c = csv.get_pos_col("frame_id");
    contributor_c = csv.get_pos_col("contributor_id");
    start_date_c = csv.get_pos_col("frame_start_date");
    end_date_c = csv.get_pos_col("frame_end_date");
    type_c = csv.get_pos_col("frame_type");
    desc_c = csv.get_pos_col("frame_desc");
    system_c = csv.get_pos_col("frame_system");
}

void FrameFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    boost::gregorian::date start_date(boost::gregorian::not_a_date_time), end_date(boost::gregorian::not_a_date_time);
    std::string error;
    if (!is_first_line && !is_valid(id_c, row)) {
        LOG4CPLUS_FATAL(
            logger, "Error while reading " + csv.filename + "  file has more than one frame and no frame_id column");
        throw InvalidHeaders(csv.filename);
    }

    auto contributor = gtfs_data.contributor_map.find(row[contributor_c]);
    if (contributor == gtfs_data.contributor_map.end()) {
        error = "FrameFusioHandler, contributor_id invalid: " + row[contributor_c];
        LOG4CPLUS_FATAL(logger, error);
        throw navitia::exception(error);
    }

    try {
        start_date = boost::gregorian::from_undelimited_string(row[start_date_c]);
    } catch (const std::exception& e) {
        error =
            "FrameFusioHandler, frame_start_date invalid: " + row[start_date_c] + ", Error: " + std::string(e.what());
        LOG4CPLUS_FATAL(logger, error);
        throw navitia::exception(error);
    }

    try {
        end_date = boost::gregorian::from_undelimited_string(row[end_date_c]);
    } catch (const std::exception& e) {
        error = "FrameFusioHandler, frame_end_date invalid: " + row[end_date_c] + ", Error: " + std::string(e.what());
        LOG4CPLUS_FATAL(logger, error);
        throw navitia::exception(error);
    }

    auto* dataset = new ed::types::Dataset();
    dataset->contributor = contributor->second;
    dataset->uri = row[id_c];
    dataset->validation_period = boost::gregorian::date_period(start_date, end_date);

    if (is_valid(type_c, row)) {
        dataset->realtime_level = get_rtlevel_enum(row[type_c]);
    }

    if (is_valid(desc_c, row)) {
        dataset->desc = row[desc_c];
    }
    if (is_valid(system_c, row)) {
        dataset->system = row[system_c];
    }

    dataset->idx = data.datasets.size() + 1;
    data.datasets.push_back(dataset);
    gtfs_data.dataset_map[dataset->uri] = dataset;
}

void DatasetFusioHandler::init(Data&) {
    id_c = csv.get_pos_col("dataset_id");
    contributor_c = csv.get_pos_col("contributor_id");
    start_date_c = csv.get_pos_col("dataset_start_date");
    end_date_c = csv.get_pos_col("dataset_end_date");
    type_c = csv.get_pos_col("dataset_type");
    desc_c = csv.get_pos_col("dataset_desc");
    system_c = csv.get_pos_col("dataset_system");
}

void DatasetFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    boost::gregorian::date start_date(boost::gregorian::not_a_date_time), end_date(boost::gregorian::not_a_date_time);
    std::string error;
    if (!is_first_line && !is_valid(id_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename
                                    + "  file has more than one dataset and no dataset_id column");
        throw InvalidHeaders(csv.filename);
    }

    auto contributor = gtfs_data.contributor_map.find(row[contributor_c]);
    if (contributor == gtfs_data.contributor_map.end()) {
        error = "DatasetFusioHandler, contributor_id invalid: " + row[contributor_c];
        LOG4CPLUS_FATAL(logger, error);
        throw navitia::exception(error);
    }

    try {
        start_date = boost::gregorian::from_undelimited_string(row[start_date_c]);
    } catch (const std::exception& e) {
        error = "DatasetFusioHandler, dataset_start_date invalid: " + row[start_date_c]
                + ", Error: " + std::string(e.what());
        LOG4CPLUS_FATAL(logger, error);
        throw navitia::exception(error);
    }

    try {
        end_date = boost::gregorian::from_undelimited_string(row[end_date_c]);
    } catch (const std::exception& e) {
        error =
            "DatasetFusioHandler, dataset_end_date invalid: " + row[end_date_c] + ", Error: " + std::string(e.what());
        LOG4CPLUS_FATAL(logger, error);
        throw navitia::exception(error);
    }

    auto* dataset = new ed::types::Dataset();
    dataset->contributor = contributor->second;
    dataset->uri = row[id_c];
    dataset->validation_period = boost::gregorian::date_period(start_date, end_date);

    if (is_valid(type_c, row)) {
        dataset->realtime_level = get_rtlevel_enum(row[type_c]);
    }

    if (is_valid(desc_c, row)) {
        dataset->desc = row[desc_c];
    }
    if (is_valid(system_c, row)) {
        dataset->system = row[system_c];
    }

    dataset->idx = data.datasets.size() + 1;
    data.datasets.push_back(dataset);
    gtfs_data.dataset_map[dataset->uri] = dataset;
}

void LineFusioHandler::init(Data&) {
    id_c = csv.get_pos_col("line_id");
    external_code_c = csv.get_pos_col("external_code");
    name_c = csv.get_pos_col("line_name");
    code_c = csv.get_pos_col("line_code");
    forward_name_c = csv.get_pos_col("forward_line_name");
    backward_name_c = csv.get_pos_col("backward_line_name");
    color_c = csv.get_pos_col("line_color");
    network_c = csv.get_pos_col("network_id");
    comment_c = csv.get_pos_col("comment_id");
    commercial_mode_c = csv.get_pos_col("commercial_mode_id");
    sort_c = csv.get_pos_col("line_sort");
    if (sort_c == -1) {
        sort_c = csv.get_pos_col("line_sort_order");
    }
    geometry_id_c = csv.get_pos_col("geometry_id");
    opening_c = csv.get_pos_col("line_opening_time");
    closing_c = csv.get_pos_col("line_closing_time");
    text_color_c = csv.get_pos_col("line_text_color");
}

void LineFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    if (!is_first_line && !has_col(id_c, row)) {
        LOG4CPLUS_FATAL(logger,
                        "Error while reading " + csv.filename + "  file has more than one line and no line_id column");
        throw InvalidHeaders(csv.filename);
    }
    auto* line = new ed::types::Line();
    line->uri = row[id_c];
    line->name = row[name_c];

    if (is_valid(code_c, row)) {
        line->code = row[code_c];
    }
    if (is_valid(text_color_c, row)) {
        line->text_color = row[text_color_c];
    }
    if (is_valid(forward_name_c, row)) {
        line->forward_name = row[forward_name_c];
    }
    if (is_valid(backward_name_c, row)) {
        line->backward_name = row[backward_name_c];
    }
    if (is_valid(color_c, row)) {
        line->color = row[color_c];
    }
    if (is_valid(geometry_id_c, row))
        line->shape = find_or_default(row.at(geometry_id_c), data.shapes);

    line->network = nullptr;
    if (is_valid(network_c, row)) {
        auto itm = gtfs_data.network_map.find(row[network_c]);
        if (itm == gtfs_data.network_map.end()) {
            throw navitia::exception("line " + line->uri + " has an unknown network: " + row[network_c]
                                     + ", the dataset is not valid");
        } else {
            line->network = itm->second;
        }
    }

    if (line->network == nullptr) {
        // if the line has no network data or an empty one, we get_or_create the default one
        line->network = gtfs_data.get_or_create_default_network(data);
    }

    if (is_valid(comment_c, row)) {
        auto itm = data.comment_by_id.find(row[comment_c]);
        if (itm != data.comment_by_id.end()) {
            data.add_pt_object_comment(line, row[comment_c]);
        }
    }

    line->commercial_mode = nullptr;
    if (is_valid(commercial_mode_c, row)) {
        auto itm = gtfs_data.commercial_mode_map.find(row[commercial_mode_c]);
        if (itm == gtfs_data.commercial_mode_map.end()) {
            LOG4CPLUS_WARN(logger, "LineFusioHandler : Impossible to find the commercial_mode "
                                       << row[commercial_mode_c] << " referenced by line " << row[id_c]);
        } else {
            line->commercial_mode = itm->second;
        }
    }

    if (line->commercial_mode == nullptr) {
        line->commercial_mode = gtfs_data.get_or_create_default_commercial_mode(data);
    }
    if (is_valid(sort_c, row) && row[sort_c] != "-1") {
        // sort == -1 means no sort
        line->sort = boost::lexical_cast<int>(row[sort_c]);
    }
    if (is_valid(opening_c, row)) {
        line->opening_time = boost::posix_time::duration_from_string(row[opening_c]);
    }
    if (is_valid(closing_c, row)) {
        line->closing_time = boost::posix_time::duration_from_string(row[closing_c]);
        while (line->closing_time->hours() >= 24) {
            line->closing_time = *line->closing_time - boost::posix_time::time_duration(24, 0, 0);
        }
    }

    if (is_valid(external_code_c, row)) {
        data.add_object_code(line, row[external_code_c]);
        gtfs_data.line_map_by_external_code[row[external_code_c]] = line;
    }

    data.lines.push_back(line);
    gtfs_data.line_map[line->uri] = line;
}

void LineGroupFusioHandler::init(Data&) {
    id_c = csv.get_pos_col("line_group_id");
    name_c = csv.get_pos_col("line_group_name");
    main_line_id_c = csv.get_pos_col("main_line_id");
}

void LineGroupFusioHandler::handle_line(Data& data, const csv_row& row, bool) {
    if (!(has_col(id_c, row) && has_col(name_c, row) && has_col(main_line_id_c, row))) {
        LOG4CPLUS_WARN(logger, "LineGroupFusioHandler: Line ignored in "
                                   << csv.filename << " missing line_group_id, line_group_name or main_line_id column");
        throw InvalidHeaders(csv.filename);
    }
    auto line = gtfs_data.line_map.find(row[main_line_id_c]);
    if (line == gtfs_data.line_map.end()) {
        LOG4CPLUS_WARN(logger, "LineGroupFusioHandler: Impossible to find the line " << row[main_line_id_c]);
        return;
    }
    auto* line_group = new ed::types::LineGroup();
    line_group->uri = row[id_c];
    line_group->name = row[name_c];
    line_group->main_line = line->second;

    // Link main_line to line_group
    ed::types::LineGroupLink line_group_link;
    line_group_link.line_group = line_group;
    line_group_link.line = line->second;

    gtfs_data.linked_lines_by_line_group_uri[line_group->uri].insert(line->second->uri);

    data.line_groups.push_back(line_group);
    data.line_group_links.push_back(line_group_link);
    gtfs_data.line_group_map[line_group->uri] = line_group;
}

void LineGroupLinksFusioHandler::init(Data&) {
    line_group_id_c = csv.get_pos_col("line_group_id");
    line_id_c = csv.get_pos_col("line_id");
}

void LineGroupLinksFusioHandler::handle_line(Data& data, const csv_row& row, bool) {
    if (!(has_col(line_group_id_c, row) && has_col(line_id_c, row))) {
        LOG4CPLUS_WARN(logger, "LineGroupLinksFusioHandler: Line ignored in "
                                   << csv.filename << " missing line_group_id or line_id column");
        throw InvalidHeaders(csv.filename);
    }
    auto line_group = gtfs_data.line_group_map.find(row[line_group_id_c]);
    if (line_group == gtfs_data.line_group_map.end()) {
        LOG4CPLUS_WARN(logger,
                       "LineGroupLinksFusioHandler: Impossible to find the line_group " << row[line_group_id_c]);
        return;
    }

    auto line = gtfs_data.line_map.find(row[line_id_c]);
    if (line == gtfs_data.line_map.end()) {
        LOG4CPLUS_WARN(logger, "LineGroupLinksFusioHandler: Impossible to find the line " << row[line_id_c]);
        return;
    }

    auto linked_lines = gtfs_data.linked_lines_by_line_group_uri[line_group->second->uri];
    if (linked_lines.find(line->second->uri) != linked_lines.end()) {
        // Don't insert duplicates
        return;
    }

    ed::types::LineGroupLink line_group_link;
    line_group_link.line_group = line_group->second;
    line_group_link.line = line->second;
    gtfs_data.linked_lines_by_line_group_uri[line_group->second->uri].insert(line->second->uri);
    data.line_group_links.push_back(line_group_link);
}

void CompanyFusioHandler::init(Data&) {
    id_c = csv.get_pos_col("company_id");
    name_c = csv.get_pos_col("company_name");
    company_address_name_c = csv.get_pos_col("company_address_name");
    company_address_number_c = csv.get_pos_col("company_address_number");
    company_address_type_c = csv.get_pos_col("company_address_type");
    company_url_c = csv.get_pos_col("company_url");
    company_mail_c = csv.get_pos_col("company_mail");
    company_phone_c = csv.get_pos_col("company_phone");
    company_fax_c = csv.get_pos_col("company_fax");
}

void CompanyFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    if (!is_first_line && !has_col(id_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename
                                    + "  file has more than one company and no company_id column");
        throw InvalidHeaders(csv.filename);
    }
    if (!is_valid(id_c, row)) {
        LOG4CPLUS_WARN(logger, "CompanyFusioHandler : Invalid company id " << row[id_c]);
        return;
    }
    auto* company = new ed::types::Company();
    company->uri = row[id_c];
    company->name = row[name_c];
    if (is_valid(company_address_name_c, row))
        company->address_name = row[company_address_name_c];
    if (is_valid(company_address_number_c, row))
        company->address_number = row[company_address_number_c];
    if (is_valid(company_address_type_c, row))
        company->address_type_name = row[company_address_type_c];
    if (is_valid(company_url_c, row))
        company->website = row[company_url_c];
    if (is_valid(company_mail_c, row))
        company->mail = row[company_mail_c];
    if (is_valid(company_phone_c, row))
        company->phone_number = row[company_phone_c];
    if (is_valid(company_fax_c, row))
        company->fax = row[company_fax_c];
    data.companies.push_back(company);
    gtfs_data.company_map[company->uri] = company;
}

void PhysicalModeFusioHandler::init(Data&) {
    id_c = csv.get_pos_col("physical_mode_id");
    name_c = csv.get_pos_col("physical_mode_name");
    co2_emission_c = csv.get_pos_col("co2_emission");
}

void PhysicalModeFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    if (!is_first_line && !has_col(id_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename
                                    + "  file has more than one physical mode and no physical_mode_id column");
        throw InvalidHeaders(csv.filename);
    }
    auto* mode = new ed::types::PhysicalMode();
    double co2_emission;
    mode->name = row[name_c];
    mode->uri = row[id_c];
    if (has_col(co2_emission_c, row)) {
        try {
            co2_emission = boost::lexical_cast<double>(row[co2_emission_c]);
            if (co2_emission >= 0.) {
                mode->co2_emission = co2_emission;
            }
        } catch (const boost::bad_lexical_cast&) {
            LOG4CPLUS_WARN(logger, "Impossible to parse the co2_emission for " + mode->uri + " " + mode->name);
        }
    }
    data.physical_modes.push_back(mode);
    gtfs_data.physical_mode_map[mode->uri] = mode;
}

void CommercialModeFusioHandler::init(Data&) {
    id_c = csv.get_pos_col("commercial_mode_id");
    name_c = csv.get_pos_col("commercial_mode_name");
}

void CommercialModeFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    if (!is_first_line && !has_col(id_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename
                                    + "  file has more than one commercial mode and no commercial_mode_id column");
        throw InvalidHeaders(csv.filename);
    }
    if (!has_col(name_c, row)) {
        LOG4CPLUS_WARN(logger,
                       "invalid line on " << csv.filename << "  commercial mode " << row[id_c] << " is missing a name");
        return;
    }
    auto* commercial_mode = new ed::types::CommercialMode();
    commercial_mode->name = row[name_c];
    commercial_mode->uri = row[id_c];
    data.commercial_modes.push_back(commercial_mode);
    gtfs_data.commercial_mode_map[commercial_mode->uri] = commercial_mode;
}

void CommentFusioHandler::init(Data&) {
    id_c = csv.get_pos_col("comment_id");
    comment_c = csv.get_pos_col("comment_name");
    type_c = csv.get_pos_col("comment_type");
}

void CommentFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    if (!is_first_line && !has_col(id_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename
                                    + "  file has more than one comment and no comment_id column");
        throw InvalidHeaders(csv.filename);
    }
    if (!has_col(comment_c, row)) {
        LOG4CPLUS_INFO(logger,
                       "Error while reading " + csv.filename + "  row has column comment for the id : " + row[id_c]);
        return;
    }
    nt::Comment comment(row[comment_c]);
    if (has_col(type_c, row)) {
        auto type = row[type_c];
        // if type is empty we keep the default value
        if (!type.empty()) {
            comment.type = type;
        }
    }
    data.comment_by_id[row[id_c]] = comment;
}

void OdtConditionsFusioHandler::init(Data&) {
    odt_condition_id_c = csv.get_pos_col("odt_condition_id");
    odt_condition_c = csv.get_pos_col("odt_condition");
}

void OdtConditionsFusioHandler::handle_line(Data&, const csv_row& row, bool is_first_line) {
    if (!is_first_line && !has_col(odt_condition_id_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename
                                    + "  file has more than one condition and no odt_condition_id_c column");
        throw InvalidHeaders(csv.filename);
    }
    gtfs_data.odt_conditions_map[row[odt_condition_id_c]] = row[odt_condition_c];
}

void StopPropertiesFusioHandler::init(Data&) {
    id_c = csv.get_pos_col("property_id");
    // TODO equipment_id NTFSv0.4: remove property_id when we stop to support NTFSv0.3
    if (id_c == -1) {
        id_c = csv.get_pos_col("equipment_id");
    }
    wheelchair_boarding_c = csv.get_pos_col("wheelchair_boarding");
    sheltered_c = csv.get_pos_col("sheltered");
    elevator_c = csv.get_pos_col("elevator");
    escalator_c = csv.get_pos_col("escalator");
    bike_accepted_c = csv.get_pos_col("bike_accepted");
    bike_depot_c = csv.get_pos_col("bike_depot");
    visual_announcement_c = csv.get_pos_col("visual_announcement");
    audible_announcement_c = csv.get_pos_col("audible_announcement");
    appropriate_escort_c = csv.get_pos_col("appropriate_escort");
    appropriate_signage_c = csv.get_pos_col("appropriate_signage");
}

void StopPropertiesFusioHandler::handle_line(Data&, const csv_row& row, bool is_first_line) {
    if (!is_first_line && !has_col(id_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename
                                    + "  file has more than one stop_properties and no stop_propertie_id column");
        throw InvalidHeaders(csv.filename);
    }
    auto itm = gtfs_data.hasProperties_map.find(row[id_c]);
    if (itm == gtfs_data.hasProperties_map.end()) {
        navitia::type::hasProperties has_properties;
        if (is_active(wheelchair_boarding_c, row))
            has_properties.set_property(navitia::type::hasProperties::WHEELCHAIR_BOARDING);
        if (is_active(sheltered_c, row))
            has_properties.set_property(navitia::type::hasProperties::SHELTERED);
        if (is_active(elevator_c, row))
            has_properties.set_property(navitia::type::hasProperties::ELEVATOR);
        if (is_active(escalator_c, row))
            has_properties.set_property(navitia::type::hasProperties::ESCALATOR);
        if (is_active(bike_accepted_c, row))
            has_properties.set_property(navitia::type::hasProperties::BIKE_ACCEPTED);
        if (is_active(bike_depot_c, row))
            has_properties.set_property(navitia::type::hasProperties::BIKE_DEPOT);
        if (is_active(visual_announcement_c, row))
            has_properties.set_property(navitia::type::hasProperties::VISUAL_ANNOUNCEMENT);
        if (is_active(audible_announcement_c, row))
            has_properties.set_property(navitia::type::hasProperties::AUDIBLE_ANNOUNVEMENT);
        if (is_active(appropriate_escort_c, row))
            has_properties.set_property(navitia::type::hasProperties::APPOPRIATE_ESCORT);
        if (is_active(appropriate_signage_c, row))
            has_properties.set_property(navitia::type::hasProperties::APPOPRIATE_SIGNAGE);
        gtfs_data.hasProperties_map[row[id_c]] = has_properties;
    }
}

void ObjectPropertiesFusioHandler::init(Data&) {
    object_id_c = csv.get_pos_col("object_id");
    object_type_c = csv.get_pos_col("object_type");
    property_name_c = csv.get_pos_col("object_property_name");
    property_value_c = csv.get_pos_col("object_property_value");
}

void ObjectPropertiesFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    if (!is_first_line
        && !(has_col(object_id_c, row) && has_col(object_type_c, row) && has_col(property_name_c, row)
             && has_col(property_value_c, row))) {
        LOG4CPLUS_WARN(
            logger, "ObjectPropertiesFusioHandler: Line ignored in "
                        << csv.filename
                        << " missing object_id or object_type or object_property_name or object_property_value column");
        return;
    }
    const nt::Header* object;
    const auto& object_id = row[object_id_c];
    const auto enum_type = get_type_enum(row[object_type_c]);
    if (enum_type == nt::Type_e::Line) {
        object = get_object(gtfs_data.line_map, object_id, "object property");
        if (!object) {
            return;
        }
    } else if (enum_type == nt::Type_e::Route) {
        object = get_object(gtfs_data.route_map, object_id, "object property");
        if (!object) {
            return;
        }
    } else if (enum_type == nt::Type_e::StopArea) {
        object = get_object(gtfs_data.stop_area_map, object_id, "object property");
        if (!object) {
            return;
        }
    } else if (enum_type == nt::Type_e::StopPoint) {
        object = get_object(gtfs_data.stop_point_map, object_id, "object property");
        if (!object) {
            return;
        }
    } else {
        LOG4CPLUS_WARN(logger, "ObjectPropertiesFusioHandler: type '" << row[object_type_c] << "' not supported");
        return;
    }
    const auto key = row[property_name_c];
    const auto val = row[property_value_c];
    data.object_properties[{object, enum_type}][key] = val;
}

void TripPropertiesFusioHandler::init(Data&) {
    id_c = csv.get_pos_col("trip_property_id");
    wheelchair_accessible_c = csv.get_pos_col("wheelchair_accessible");
    bike_accepted_c = csv.get_pos_col("bike_accepted");
    air_conditioned_c = csv.get_pos_col("air_conditioned");
    visual_announcement_c = csv.get_pos_col("visual_announcement");
    audible_announcement_c = csv.get_pos_col("audible_announcement");
    appropriate_escort_c = csv.get_pos_col("appropriate_escort");
    appropriate_signage_c = csv.get_pos_col("appropriate_signage");
    // TODO school_vehicle_type NTFSv0.5: remove school_vehicle when we stop to support NTFSv0.4
    school_vehicle_c = csv.get_pos_col("school_vehicle");
    if (school_vehicle_c == -1) {
        school_vehicle_c = csv.get_pos_col("school_vehicle_type");
    }
}

void TripPropertiesFusioHandler::handle_line(Data&, const csv_row& row, bool is_first_line) {
    if (!is_first_line && !has_col(id_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename
                                    + "  file has more than one stop_properties and no stop_propertie_id column");
        throw InvalidHeaders(csv.filename);
    }
    auto itm = gtfs_data.hasVehicleProperties_map.find(row[id_c]);
    if (itm == gtfs_data.hasVehicleProperties_map.end()) {
        navitia::type::hasVehicleProperties has_properties;
        if (is_active(wheelchair_accessible_c, row))
            has_properties.set_vehicle(navitia::type::hasVehicleProperties::WHEELCHAIR_ACCESSIBLE);
        if (is_active(bike_accepted_c, row))
            has_properties.set_vehicle(navitia::type::hasVehicleProperties::BIKE_ACCEPTED);
        if (is_active(air_conditioned_c, row))
            has_properties.set_vehicle(navitia::type::hasVehicleProperties::AIR_CONDITIONED);
        if (is_active(visual_announcement_c, row))
            has_properties.set_vehicle(navitia::type::hasVehicleProperties::VISUAL_ANNOUNCEMENT);
        if (is_active(audible_announcement_c, row))
            has_properties.set_vehicle(navitia::type::hasVehicleProperties::AUDIBLE_ANNOUNCEMENT);
        if (is_active(appropriate_escort_c, row))
            has_properties.set_vehicle(navitia::type::hasVehicleProperties::APPOPRIATE_ESCORT);
        if (is_active(appropriate_signage_c, row))
            has_properties.set_vehicle(navitia::type::hasVehicleProperties::APPOPRIATE_SIGNAGE);
        if (is_active(school_vehicle_c, row))
            has_properties.set_vehicle(navitia::type::hasVehicleProperties::SCHOOL_VEHICLE);
        gtfs_data.hasVehicleProperties_map[row[id_c]] = has_properties;
    }
}

static boost::gregorian::date parse_date(const std::string& str) {
    auto logger = log4cplus::Logger::getInstance("log");
    if (str.empty()) {
        LOG4CPLUS_ERROR(logger, "impossible to parse date, date is empty");
        return boost::gregorian::date(boost::gregorian::not_a_date_time);
    }
    try {
        return boost::gregorian::from_undelimited_string(str);
    } catch (const boost::bad_lexical_cast&) {
        LOG4CPLUS_ERROR(logger, "Impossible to parse the date for " << str);
    } catch (const boost::gregorian::bad_day_of_month&) {
        LOG4CPLUS_ERROR(logger, "bad_day_of_month : Impossible to parse the date for " << str);
    } catch (const boost::gregorian::bad_day_of_year&) {
        LOG4CPLUS_ERROR(logger, "bad_day_of_year : Impossible to parse the date for " << str);
    } catch (const boost::gregorian::bad_month&) {
        LOG4CPLUS_ERROR(logger, "bad_month : Impossible to parse the date for " << str);
    } catch (const boost::gregorian::bad_year&) {
        LOG4CPLUS_ERROR(logger, "bad_year : Impossible to parse the date for " << str);
    }
    return boost::gregorian::date(boost::gregorian::not_a_date_time);
}

namespace grid_calendar {

void PeriodFusioHandler::init(Data&) {
    id_c = csv.get_pos_col("grid_calendar_id");
    if (id_c == UNKNOWN_COLUMN) {
        id_c = csv.get_pos_col("calendar_id");
    }
    start_c = csv.get_pos_col("start_date");
    if (start_c == UNKNOWN_COLUMN) {
        start_c = csv.get_pos_col("begin_date");
    }
    end_c = csv.get_pos_col("end_date");
}

void PeriodFusioHandler::handle_line(Data&, const csv_row& row, bool is_first_line) {
    if (!is_first_line && !has_col(id_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " << csv.filename << " column id : " << id_c << " not available");
        throw InvalidHeaders(csv.filename);
    }
    auto cal = gtfs_data.calendars_map.find(row[id_c]);
    if (cal == gtfs_data.calendars_map.end()) {
        LOG4CPLUS_ERROR(logger, "GridCalPeriodFusioHandler : Impossible to find the calendar " << row[id_c]);
        return;
    }
    boost::gregorian::date begin_date, end_date;
    if (has_col(start_c, row)) {
        begin_date = parse_date(row[start_c]);
    }
    if (has_col(end_c, row)) {
        end_date = parse_date(row[end_c]);
    }
    if (begin_date.is_not_a_date() || end_date.is_not_a_date()) {
        LOG4CPLUS_ERROR(logger, "period invalid, not added for calendar " << row[id_c]);
        return;
    }
    // the end of a gregorian period not in the period (it's the day after)
    // since we want the end to be in the period, we add one day to it
    boost::gregorian::date_period period(begin_date, end_date + boost::gregorian::days(1));
    cal->second->period_list.push_back(period);
}

void GridCalendarFusioHandler::init(Data&) {
    id_c = csv.get_pos_col("grid_calendar_id");
    if (id_c == UNKNOWN_COLUMN) {
        id_c = csv.get_pos_col("id");
    }
    name_c = csv.get_pos_col("name");
    monday_c = csv.get_pos_col("monday");
    tuesday_c = csv.get_pos_col("tuesday");
    wednesday_c = csv.get_pos_col("wednesday");
    thursday_c = csv.get_pos_col("thursday");
    friday_c = csv.get_pos_col("friday");
    saturday_c = csv.get_pos_col("saturday");
    sunday_c = csv.get_pos_col("sunday");
}

void GridCalendarFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    if (!is_first_line && !has_col(id_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " << csv.filename << " column id : " << id_c << " not available");
        throw InvalidHeaders(csv.filename);
    }
    if (has_col(id_c, row) && row[id_c].empty()) {
        // we don't want empty named calendar as they often are empty lines in reality
        LOG4CPLUS_INFO(logger, "calendar name is empty, we skip it");
        return;
    }
    auto* calendar = new ed::types::Calendar();
    calendar->uri = row[id_c];
    calendar->name = row[name_c];
    calendar->week_pattern[navitia::Monday] = is_active(monday_c, row);
    calendar->week_pattern[navitia::Tuesday] = is_active(tuesday_c, row);
    calendar->week_pattern[navitia::Wednesday] = is_active(wednesday_c, row);
    calendar->week_pattern[navitia::Thursday] = is_active(thursday_c, row);
    calendar->week_pattern[navitia::Friday] = is_active(friday_c, row);
    calendar->week_pattern[navitia::Saturday] = is_active(saturday_c, row);
    calendar->week_pattern[navitia::Sunday] = is_active(sunday_c, row);
    calendar->idx = data.calendars.size() + 1;
    data.calendars.push_back(calendar);
    gtfs_data.calendars_map[calendar->uri] = calendar;
}

void ExceptionDatesFusioHandler::init(Data&) {
    id_c = csv.get_pos_col("grid_calendar_id");
    if (id_c == UNKNOWN_COLUMN) {
        id_c = csv.get_pos_col("calendar_id");
    }
    datetime_c = csv.get_pos_col("date");
    type_c = csv.get_pos_col("type");
}

void ExceptionDatesFusioHandler::handle_line(Data&, const csv_row& row, bool is_first_line) {
    if (!is_first_line && !has_col(id_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " << csv.filename << " column id : " << id_c << " not available");
        throw InvalidHeaders(csv.filename);
    }
    auto cal = gtfs_data.calendars_map.find(row[id_c]);
    if (cal == gtfs_data.calendars_map.end()) {
        LOG4CPLUS_WARN(logger, "ExceptionDatesFusioHandler : Impossible to find the calendar " << row[id_c]);
        return;
    }
    if (!has_col(type_c, row)) {
        LOG4CPLUS_WARN(logger, "ExceptionDatesFusioHandler: No column type for calendar " << row[id_c]);
        return;
    }
    if (row[type_c] != "0" && row[type_c] != "1") {
        LOG4CPLUS_WARN(logger, "ExceptionDatesFusioHandler : unknown type " << row[type_c]);
        return;
    }
    if (!has_col(datetime_c, row)) {
        LOG4CPLUS_WARN(logger, "ExceptionDatesFusioHandler: No column datetime for calendar " << row[id_c]);
        return;
    }
    boost::gregorian::date date(parse_date(row[datetime_c]));
    if (date.is_not_a_date()) {
        LOG4CPLUS_ERROR(logger,
                        "date format not valid, we do not add the exception " << row[type_c] << " for " << row[id_c]);
        return;
    }
    navitia::type::ExceptionDate exception_date;
    exception_date.date = date;
    exception_date.type =
        static_cast<navitia::type::ExceptionDate::ExceptionType>(boost::lexical_cast<int>(row[type_c]));
    cal->second->exceptions.push_back(exception_date);
}

void CalendarLineFusioHandler::init(Data&) {
    id_c = csv.get_pos_col("grid_calendar_id");
    if (id_c == UNKNOWN_COLUMN) {
        id_c = csv.get_pos_col("calendar_id");
    }
    line_c = csv.get_pos_col("line_id");
    if (line_c == UNKNOWN_COLUMN) {
        line_id_is_present = false;
        line_c = csv.get_pos_col("line_external_code");
    }
}

void CalendarLineFusioHandler::handle_line(Data&, const csv_row& row, bool is_first_line) {
    if (!is_first_line && !has_col(id_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " << csv.filename << " column id : " << id_c << " not available");
        throw InvalidHeaders(csv.filename);
    }

    auto cal = gtfs_data.calendars_map.find(row[id_c]);
    if (cal == gtfs_data.calendars_map.end()) {
        LOG4CPLUS_ERROR(logger, "CalendarLineFusioHandler: Impossible to find the calendar " << row[id_c]);
        return;
    }
    if (!has_col(line_c, row)) {
        LOG4CPLUS_WARN(logger, "CalendarLineFusioHandler: No line column for calendar : " << row[id_c]);
        return;
    }
    std::unordered_map<std::string, ed::types::Line*>::iterator it;
    if (line_id_is_present) {
        it = gtfs_data.line_map.find(row[line_c]);
        if (it == gtfs_data.line_map.end()) {
            LOG4CPLUS_ERROR(logger, "CalendarLineFusioHandler: Impossible to find the line id" << row[line_c]);
            return;
        }
    } else {
        it = gtfs_data.line_map_by_external_code.find(row[line_c]);
        if (it == gtfs_data.line_map_by_external_code.end()) {
            LOG4CPLUS_ERROR(logger,
                            "CalendarLineFusioHandler: Impossible to find the line by external code " << row[line_c]);
            return;
        }
    }
    cal->second->line_list.push_back(it->second);
}

void CalendarTripFusioHandler::init(Data&) {
    calendar_c = csv.get_pos_col("calendar_id");
    trip_c = csv.get_pos_col("trip_external_code");
}

void CalendarTripFusioHandler::handle_line(Data& data, const csv_row& row, bool) {
    if (!has_col(calendar_c, row) || !(has_col(trip_c, row))) {
        LOG4CPLUS_FATAL(logger, "CalendarTripFusioHandler: Error while reading " + csv.filename
                                    + " file has no calendar_id or trip_external_code column");
        throw InvalidHeaders(csv.filename);
    }

    auto cal = gtfs_data.calendars_map.find(row[calendar_c]);
    if (cal == gtfs_data.calendars_map.end()) {
        LOG4CPLUS_ERROR(logger, "CalendarTripFusioHandler: Impossible to find the calendar " << row[calendar_c]);
        return;
    }

    auto meta_vj = gtfs_data.metavj_by_external_code.find(row[trip_c]);
    if (meta_vj == gtfs_data.metavj_by_external_code.end()) {
        LOG4CPLUS_ERROR(logger, "CalendarTripFusioHandler: Impossible to find the trip " << row[trip_c]);
        return;
    }
    auto associated_cal = new ed::types::AssociatedCalendar();
    associated_cal->idx = data.associated_calendars.size();
    data.associated_calendars.push_back(associated_cal);
    associated_cal->calendar = cal->second;
    meta_vj->second->associated_calendars.insert({cal->second->uri, associated_cal});
}

void GridCalendarTripExceptionDatesFusioHandler::init(Data&) {
    calendar_c = csv.get_pos_col("calendar_id");
    trip_c = csv.get_pos_col("trip_external_code");
    date_c = csv.get_pos_col("date");
    type_c = csv.get_pos_col("type");
}

void GridCalendarTripExceptionDatesFusioHandler::handle_line(Data&, const csv_row& row, bool) {
    if (!has_col(calendar_c, row) || !(has_col(trip_c, row))) {
        LOG4CPLUS_FATAL(logger, "GridCalendarTripExceptionDatesFusioHandler: Error while reading " + csv.filename
                                    + " file has no calendar_id or trip_external_code column");
        throw InvalidHeaders(csv.filename);
    }
    auto cal = gtfs_data.calendars_map.find(row[calendar_c]);
    if (cal == gtfs_data.calendars_map.end()) {
        LOG4CPLUS_WARN(
            logger, "GridCalendarTripExceptionDatesFusioHandler : Impossible to find the calendar " << row[calendar_c]);
        return;
    }
    if (!has_col(type_c, row)) {
        LOG4CPLUS_WARN(logger,
                       "GridCalendarTripExceptionDatesFusioHandler: No column type for calendar " << row[calendar_c]);
        return;
    }
    if (row[type_c] != "0" && row[type_c] != "1") {
        LOG4CPLUS_WARN(logger, "GridCalendarTripExceptionDatesFusioHandler : Unknown type " << row[type_c]);
        return;
    }
    if (!has_col(date_c, row)) {
        LOG4CPLUS_WARN(
            logger, "GridCalendarTripExceptionDatesFusioHandler: No column datetime for calendar " << row[calendar_c]);
        return;
    }
    auto meta_vj = gtfs_data.metavj_by_external_code.find(row[trip_c]);
    if (meta_vj == gtfs_data.metavj_by_external_code.end()) {
        LOG4CPLUS_WARN(logger, "CalendarTripFusioHandler: Impossible to find the trip " << row[trip_c]);
        return;
    }
    boost::gregorian::date date(parse_date(row[date_c]));
    if (date.is_not_a_date()) {
        LOG4CPLUS_WARN(logger,
                       "GridCalendarTripExceptionDatesFusioHandler: Date format not valid, we do not add the exception "
                           << row[type_c] << " for " << row[calendar_c]);
        return;
    }
    navitia::type::ExceptionDate exception_date;
    exception_date.date = date;
    exception_date.type =
        static_cast<navitia::type::ExceptionDate::ExceptionType>(boost::lexical_cast<int>(row[type_c]));
    meta_vj->second->associated_calendars[cal->second->uri]->exceptions.push_back(exception_date);
}
}  // namespace grid_calendar

void AdminStopAreaFusioHandler::init(Data& data) {
    admin_c = csv.get_pos_col("admin_id");

    stop_area_c = csv.get_pos_col("stop_id");

    // For retro compatibity
    // TODO : to remove after the data team update, it will become useless (NAVP-1285)
    if (stop_area_c == UNKNOWN_COLUMN) {
        stop_id_is_present = false;
        stop_area_c = csv.get_pos_col("station_id");
        for (const auto& object_code_map : data.object_codes) {
            for (auto& object_code : object_code_map.second) {
                if (object_code_map.first.type == nt::Type_e::StopArea && object_code.first == "external_code") {
                    const auto stop_area = gtfs_data.stop_area_map.find(object_code_map.first.pt_object->uri);
                    if (stop_area != gtfs_data.stop_area_map.end()) {
                        for (const auto& external_code : object_code.second) {
                            tmp_stop_area_map[external_code] = stop_area->second;
                        }
                    }
                }
            }
        }
    }
}

void AdminStopAreaFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    if (!is_first_line && !has_col(stop_area_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename + "  file has no stop_area_c column");
        throw InvalidHeaders(csv.filename);
    }

    std::unordered_map<std::string, ed::types::StopArea*>::iterator sa_it;
    // For retrocompatibity
    // TODO : to remove after the data team update, it will become useless (NAVP-1285)
    if (!stop_id_is_present) {
        sa_it = tmp_stop_area_map.find(row[stop_area_c]);
        if (sa_it == tmp_stop_area_map.end()) {
            LOG4CPLUS_ERROR(logger,
                            "AdminStopAreaFusioHandler : Impossible to find the stop_area " << row[stop_area_c]);
            return;
        }
    } else {
        sa_it = gtfs_data.stop_area_map.find(row[stop_area_c]);
        if (sa_it == gtfs_data.stop_area_map.end()) {
            LOG4CPLUS_ERROR(logger,
                            "AdminStopAreaFusioHandler : Impossible to find the stop_area " << row[stop_area_c]);
            return;
        }
    }

    ed::types::AdminStopArea* admin_stop_area{nullptr};
    auto admin_it = admin_stop_area_map.find(row[admin_c]);
    if (admin_it == admin_stop_area_map.end()) {
        admin_stop_area = new ed::types::AdminStopArea();
        admin_stop_area->admin = row[admin_c];
        admin_stop_area_map.insert({admin_stop_area->admin, admin_stop_area});
        data.admin_stop_areas.push_back(admin_stop_area);
    } else {
        admin_stop_area = admin_it->second;
    }

    admin_stop_area->stop_area.push_back(sa_it->second);
}

void CommentLinksFusioHandler::init(Data&) {
    object_id_c = csv.get_pos_col("object_id");
    object_type_c = csv.get_pos_col("object_type");
    comment_id_c = csv.get_pos_col("comment_id");
}

void CommentLinksFusioHandler::handle_line(Data& data, const csv_row& row, bool) {
    if (!has_col(object_id_c, row) || !has_col(object_type_c, row) || !has_col(comment_id_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename + "  impossible to find all needed fields");
        throw InvalidHeaders(csv.filename);
    }

    const auto object_id = row[object_id_c];
    const auto object_type = row[object_type_c];
    const auto comment_id = row[comment_id_c];

    // for coherence purpose we check that the comment exists
    nt::Comment comment;
    const auto comment_it = data.comment_by_id.find(comment_id);
    if (comment_it != data.comment_by_id.end()) {
        comment = comment_it->second;
    } else {
        LOG4CPLUS_INFO(logger, "impossible to find comment " << comment_id << ", skipping line for " << object_id
                                                             << " (" << object_type << ")");
        return;
    }

    // and we check that the object exists
    const nt::Type_e navitia_type = get_type_enum(object_type);
    if (navitia_type == nt::Type_e::StopArea) {
        const auto object = get_object(gtfs_data.stop_area_map, object_id, "comment");
        if (!object) {
            return;
        }
        data.add_pt_object_comment(object, comment_id);
    } else if (navitia_type == nt::Type_e::StopPoint) {
        const auto object = get_object(gtfs_data.stop_point_map, object_id, "comment");
        if (!object) {
            return;
        }
        data.add_pt_object_comment(object, comment_id);
    } else if (navitia_type == nt::Type_e::Line) {
        const auto object = get_object(gtfs_data.line_map, object_id, "comment");
        if (!object) {
            return;
        }
        data.add_pt_object_comment(object, comment_id);
    } else if (navitia_type == nt::Type_e::LineGroup) {
        const auto object = get_object(gtfs_data.line_group_map, object_id, "comment");
        if (!object) {
            return;
        }
        data.add_pt_object_comment(object, comment_id);
    } else if (navitia_type == nt::Type_e::Route) {
        const auto object = get_object(gtfs_data.route_map, object_id, "comment");
        if (!object) {
            return;
        }
        data.add_pt_object_comment(object, comment_id);
    } else if (navitia_type == nt::Type_e::VehicleJourney) {
        const auto range = gtfs_data.tz.vj_by_name.equal_range(object_id);
        if (empty(range)) {
            LOG4CPLUS_INFO(logger, "impossible to find " << object_id << ", cannot add the comment, skipping line");
            return;
        }
        // the trips are split by dst, we add the comment on each one
        for (auto vj_it = range.first; vj_it != range.second; ++vj_it) {
            data.add_pt_object_comment(vj_it->second, row[comment_id_c]);
        }
        return;
    } else if (navitia_type == nt::Type_e::StopTime) {
        // the stop time are also split by dst
        const auto& list_st = gtfs_data.stop_time_map[object_id];
        if (list_st.empty()) {
            LOG4CPLUS_INFO(
                logger, "impossible to find the stoptime " << object_id << ", cannot add the comment, skipping line");
            return;
        }

        // the stop times can be split by dst, so we add the comment on each stop time
        for (const auto* st : list_st) {
            data.add_pt_object_comment(st, row[comment_id_c]);
        }
        return;
    } else {
        LOG4CPLUS_INFO(logger, "Unhandled object type " << object_type << ", impossible to add a comment");
        return;
    }
}

void ObjectCodesFusioHandler::init(Data&) {
    object_uri_c = csv.get_pos_col("object_id");
    object_type_c = csv.get_pos_col("object_type");
    code_c = csv.get_pos_col("object_code");
    object_system_c = csv.get_pos_col("object_system");
}

void ObjectCodesFusioHandler::handle_line(Data& data, const csv_row& row, bool) {
    if (!has_col(object_uri_c, row) || !has_col(object_type_c, row) || !has_col(code_c, row)
        || !has_col(object_system_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename + "  impossible to find all needed fields");
        throw InvalidHeaders(csv.filename);
    }

    std::string key = row[object_system_c];
    std::string object_type = boost::algorithm::to_lower_copy(row[object_type_c]);

    if (boost::algorithm::to_lower_copy(key) == "navitia1") {
        key = "external_code";
    }

    if (object_type == "line") {
        const auto& it_object = gtfs_data.line_map.find(row[object_uri_c]);
        if (it_object != gtfs_data.line_map.end()) {
            data.add_object_code(it_object->second, row[code_c], key);
            if (key == "external_code") {
                gtfs_data.line_map_by_external_code[row[code_c]] = it_object->second;
            }
        }
    } else if (object_type == "route") {
        const auto& it_object = gtfs_data.route_map.find(row[object_uri_c]);
        if (it_object != gtfs_data.route_map.end()) {
            data.add_object_code(it_object->second, row[code_c], key);
        }
    } else if (object_type == "network") {
        const auto& it_object = gtfs_data.network_map.find(row[object_uri_c]);
        if (it_object != gtfs_data.network_map.end()) {
            data.add_object_code(it_object->second, row[code_c], key);
        }
    } else if (object_type == "trip") {
        const auto& it_object = data.meta_vj_map.find(row[object_uri_c]);
        if (it_object != data.meta_vj_map.end()) {
            for (const auto& vj : it_object->second.theoric_vj) {
                data.add_object_code(vj, row[code_c], key);
            }
        }
    } else if (object_type == "stop_area") {
        const auto& it_object = gtfs_data.stop_area_map.find(row[object_uri_c]);
        if (it_object != gtfs_data.stop_area_map.end()
            && !data.if_object_code_exist(it_object->second, row[code_c], key)) {
            data.add_object_code(it_object->second, row[code_c], key);
        }
    } else if (object_type == "stop_point") {
        const auto& it_object = gtfs_data.stop_point_map.find(row[object_uri_c]);
        if (it_object != gtfs_data.stop_point_map.end()
            && !data.if_object_code_exist(it_object->second, row[code_c], key)) {
            data.add_object_code(it_object->second, row[code_c], key);
        }
    } else if (object_type == "company") {
        const auto& it_object = gtfs_data.company_map.find(row[object_uri_c]);
        if (it_object != gtfs_data.company_map.end()) {
            data.add_object_code(it_object->second, row[code_c], key);
        }
    } else {
        LOG4CPLUS_DEBUG(logger, "unknown object type " << object_type << " skipping object code");
    }
}

ed::types::CommercialMode* GtfsData::get_or_create_default_commercial_mode(Data& data) {
    if (!default_commercial_mode) {
        default_commercial_mode = new ed::types::CommercialMode();
        default_commercial_mode->name = "mode commercial par défaut";
        default_commercial_mode->uri = "default_commercial_mode";
        data.commercial_modes.push_back(default_commercial_mode);
        commercial_mode_map[default_commercial_mode->uri] = default_commercial_mode;
    }
    return default_commercial_mode;
}

ed::types::PhysicalMode* GtfsData::get_or_create_default_physical_mode(Data& data) {
    if (!default_physical_mode) {
        default_physical_mode = new ed::types::PhysicalMode();
        default_physical_mode->name = "mode physique par défaut";
        default_physical_mode->uri = "default_physical_mode";
        data.physical_modes.push_back(default_physical_mode);
        physical_mode_map[default_physical_mode->uri] = default_physical_mode;
    }
    return default_physical_mode;
}

void AddressesFusioHandler::init(Data&) {
    address_id_c = csv.get_pos_col("address_id");
    street_name_c = csv.get_pos_col("street_name");
    house_number_c = csv.get_pos_col("house_number");
}

void AddressesFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    if (!is_first_line && !has_col(address_id_c, row) && !has_col(street_name_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename + "  impossible to find all needed fields");
        throw InvalidHeaders(csv.filename);
    }
    auto address = new navitia::type::Address();
    address->id = row[address_id_c];
    address->street_name = row[street_name_c];
    address->house_number = row[house_number_c];
    data.addresses_from_ntfs.push_back(address);
}

void FusioParser::parse_files(Data& data, const std::string& beginning_date) {
    parse<FeedInfoFusioHandler>(data, "feed_infos.txt", true);

    manage_production_date(data, beginning_date);

    parse<GeometriesFusioHandler>(data, "geometries.txt");

    if (!parse<AgencyFusioHandler>(data, "networks.txt")) {
        parse<AgencyFusioHandler>(data, "agency.txt", true);
    }

    parse<ContributorFusioHandler>(data, "contributors.txt");
    // TODO: remove frames.txt when all the components (export NTFS, fusio2ed, ...) are delivered.
    if (boost::filesystem::exists(this->path + "/datasets.txt")) {
        parse<DatasetFusioHandler>(data, "datasets.txt");
    } else {
        parse<FrameFusioHandler>(data, "frames.txt");
    }

    default_datasets(gtfs_data, data);

    if (!parse<CompanyFusioHandler>(data, "companies.txt")) {
        parse<CompanyFusioHandler>(data, "company.txt");
    }

    parse<PhysicalModeFusioHandler>(data, "physical_modes.txt");
    parse<CommercialModeFusioHandler>(data, "commercial_modes.txt");
    parse<CommentFusioHandler>(data, "comments.txt");
    parse<LineFusioHandler>(data, "lines.txt");
    parse<LineGroupFusioHandler>(data, "line_groups.txt");
    parse<LineGroupLinksFusioHandler>(data, "line_group_links.txt");

    // TODO equipments NTFSv0.4: remove stop_properties when we stop to support NTFSv0.3
    if (boost::filesystem::exists(this->path + "/equipments.txt")) {
        parse<StopPropertiesFusioHandler>(data, "equipments.txt");
    } else {
        parse<StopPropertiesFusioHandler>(data, "stop_properties.txt");
    }
    parse<StopsFusioHandler>(data, "stops.txt", true);
    if (boost::filesystem::exists(this->path + "/addresses.txt")) {
        parse<AddressesFusioHandler>(data, "addresses.txt");
    }
    if (boost::filesystem::exists(this->path + "/pathways.txt")) {
        parse<PathWayFusioHandler>(data, "pathways.txt", true);
    }
    parse<RouteFusioHandler>(data, "routes.txt", true);
    parse<TransfersFusioHandler>(data, "transfers.txt");

    parse<CalendarGtfsHandler>(data, "calendar.txt");
    parse<CalendarDatesGtfsHandler>(data, "calendar_dates.txt");
    // after the calendar load, we need to split the validitypattern
    split_validity_pattern_over_dst(data, gtfs_data);

    parse<TripPropertiesFusioHandler>(data, "trip_properties.txt");
    parse<OdtConditionsFusioHandler>(data, "odt_conditions.txt");
    parse<TripsFusioHandler>(data, "trips.txt", true);
    parse<StopTimeFusioHandler>(data, "stop_times.txt", true);
    parse<FrequenciesGtfsHandler>(data, "frequencies.txt");
    parse<ObjectCodesFusioHandler>(data, "object_codes.txt");
    parse<grid_calendar::GridCalendarFusioHandler>(data, "grid_calendars.txt");
    parse<grid_calendar::PeriodFusioHandler>(data, "grid_periods.txt");
    parse<grid_calendar::ExceptionDatesFusioHandler>(data, "grid_exception_dates.txt");
    parse<grid_calendar::CalendarLineFusioHandler>(data, "grid_rel_calendar_line.txt");
    parse<grid_calendar::CalendarTripFusioHandler>(data, "grid_rel_calendar_trip.txt");
    parse<grid_calendar::GridCalendarTripExceptionDatesFusioHandler>(data, "grid_calendar_trip_exception_dates.txt");
    parse<AdminStopAreaFusioHandler>(data, "admin_stations.txt");
    parse<ObjectPropertiesFusioHandler>(data, "object_properties.txt");
    parse<CommentLinksFusioHandler>(data, "comment_links.txt");
}
}  // namespace connectors
}  // namespace ed
