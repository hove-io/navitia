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

#include "fusio_parser.h"

#include <boost/geometry.hpp>

namespace ed { namespace connectors {

void AgencyFusioHandler::init(Data& data) {
    AgencyGtfsHandler::init(data);
    ext_code_c = csv.get_pos_col("external_code");
    sort_c = csv.get_pos_col("agency_sort");
    agency_url_c = csv.get_pos_col("agency_url");
}

void AgencyFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {

    if(! is_valid(id_c, row)){
        LOG4CPLUS_WARN(logger, "AgencyFusioHandler : Invalid agency id " << row[id_c]);
        return;
    }

    ed::types::Network * network = new ed::types::Network();
    network->uri = row[id_c];

    if (is_valid(ext_code_c, row)) {
        network->external_code = row[ext_code_c];
    }

    network->name = row[name_c];

    if (is_valid(sort_c, row)) {
        network->sort =  boost::lexical_cast<int>(row[sort_c]);
    }

    if (is_valid(agency_url_c, row)) {
        network->website = row[agency_url_c];
    }

    data.networks.push_back(network);
        gtfs_data.network_map[network->uri] = network;

    std::string timezone_name = row[time_zone_c];

    if (! is_first_line) {
        //we created a default agency, with a default time zone, but this will be overidden if we read at least one agency
        if (gtfs_data.tz.default_timezone.first != timezone_name) {
            LOG4CPLUS_WARN(logger, "Error while reading "<< csv.filename <<
                            " all the time zone are not equals, only the first one will be considered as the default timezone");
        }
        return;
    }

    if (timezone_name.empty()) {
        throw navitia::exception("Error while reading " + csv.filename +
                                 + " timezone is empty for agency " + network->uri);
    }

    auto tz = gtfs_data.tz.tz_db.time_zone_from_region(timezone_name);
    if (! tz) {
        throw navitia::exception("Error while reading " + csv.filename +
                                 + " timezone " + timezone_name + " is not valid for agency " + network->uri);
    }
    gtfs_data.tz.default_timezone = {timezone_name, tz};
    LOG4CPLUS_INFO(logger, "default agency tz " << gtfs_data.tz.default_timezone.first
                   << " -> " << gtfs_data.tz.default_timezone.second->std_zone_name());

}

void StopsFusioHandler::init(Data& data) {
        StopsGtfsHandler::init(data);
        ext_code_c = csv.get_pos_col("external_code");
        property_id_c = csv.get_pos_col("property_id");
        comment_id_c =  csv.get_pos_col("comment_id");
        visible_c =  csv.get_pos_col("visible");
}

//in fusio we want to delete all stop points without stop area
void StopsFusioHandler::handle_stop_point_without_area(Data& data) {
    //Deletion of the stop point without stop areas
    std::vector<size_t> erase_sp;
    for (int i = data.stop_points.size()-1; i >=0;--i) {
        if (data.stop_points[i]->stop_area == nullptr) {
            erase_sp.push_back(i);
        }
    }
    int num_elements = data.stop_points.size();
    for (size_t to_erase : erase_sp) {
        gtfs_data.stop_map.erase(data.stop_points[to_erase]->uri);
        delete data.stop_points[to_erase];
        data.stop_points[to_erase] = data.stop_points[num_elements - 1];
        num_elements--;
    }
    data.stop_points.resize(num_elements);
    LOG4CPLUS_INFO(logger, "Deletion of " << erase_sp.size() << " stop_point wihtout stop_area");
}

StopsGtfsHandler::stop_point_and_area StopsFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    auto return_wrapper = StopsGtfsHandler::handle_line(data, row, is_first_line);

    if (is_valid(ext_code_c, row)) {
        if ( return_wrapper.second != nullptr )
            return_wrapper.second->external_code = row[ext_code_c];
        else if ( return_wrapper.first != nullptr )
                return_wrapper.first->external_code = row[ext_code_c];
    }

    if (is_valid(property_id_c, row)) {
        auto it_property = gtfs_data.hasProperties_map.find(row[property_id_c]);
        if(it_property != gtfs_data.hasProperties_map.end()){
            if( return_wrapper.first != nullptr){
                return_wrapper.first->set_properties(it_property->second.properties());
            }
            if( return_wrapper.second != nullptr){
                return_wrapper.second->set_properties(it_property->second.properties());
            }
        }
    }

    if (is_valid(comment_id_c, row)) {
        auto it_comment = gtfs_data.comment_map.find(row[comment_id_c]);
        if(it_comment != gtfs_data.comment_map.end()){
            if( return_wrapper.first != nullptr){
                return_wrapper.first->comment = it_comment->second;
            }
            if( return_wrapper.second != nullptr){
                return_wrapper.second->comment = it_comment->second;
            }
        }
    }

    if (return_wrapper.second != nullptr && is_valid(visible_c, row)) {
        return_wrapper.second->visible = (row[visible_c] == "1");
    }
    return return_wrapper;
}

void RouteFusioHandler::init(Data& ) {
    ext_code_c = csv.get_pos_col("external_code");
    route_id_c = csv.get_pos_col("route_id");
    route_name_c = csv.get_pos_col("route_name");
    is_forward_c = csv.get_pos_col("is_forward");
    line_id_c = csv.get_pos_col("line_id");
    comment_id_c = csv.get_pos_col("comment_id");
    contributor_id_c = csv.get_pos_col("contributor_id");
    geometry_id_c = csv.get_pos_col("geometry_id");
    ignored = 0;
}

void RouteFusioHandler::handle_line(Data& data, const csv_row& row, bool) {
    if(gtfs_data.route_map.find(row[route_id_c]) != gtfs_data.route_map.end()) {
        ignored++;
        LOG4CPLUS_WARN(logger, "dupplicate on route line " + row[route_id_c]);
        return;
    }
    ed::types::Line* ed_line = nullptr;
    auto it_line = gtfs_data.line_map.find(row[line_id_c]);
    if(it_line != gtfs_data.line_map.end()){
        ed_line = it_line->second;
    }else{
        ignored++;
        LOG4CPLUS_WARN(logger, "Route orphan " + row[route_id_c]);
        return;
    }
    ed::types::Route* ed_route = new ed::types::Route();
    ed_route->line = ed_line;
    ed_route->uri = row[route_id_c];

    if ( is_valid(ext_code_c, row) ){
        ed_route->external_code = row[ext_code_c];
    }

    ed_route->name = row[route_name_c];

    if ( is_valid(comment_id_c, row) ){
        auto it_comment = gtfs_data.comment_map.find(row[comment_id_c]);
        if(it_comment != gtfs_data.comment_map.end()){
            ed_route->comment = it_comment->second;
        }
    }
    if (is_valid(geometry_id_c, row))
        ed_route->shape = find_or_default(row.at(geometry_id_c), data.shapes);

    gtfs_data.route_map[row[route_id_c]] = ed_route;
    data.routes.push_back(ed_route);
}

void TransfersFusioHandler::init(Data& d) {
    TransfersGtfsHandler::init(d);
    real_time_c = csv.get_pos_col("real_min_transfer_time"),
            property_id_c = csv.get_pos_col("property_id");
}

void TransfersFusioHandler::fill_stop_point_connection(ed::types::StopPointConnection* connection, const csv_row& row) const {
    TransfersGtfsHandler::fill_stop_point_connection(connection, row);

    if(is_valid(property_id_c, row)) {
        auto it_property = gtfs_data.hasProperties_map.find(row[property_id_c]);
        if(it_property != gtfs_data.hasProperties_map.end()){
            connection->set_properties(it_property->second.properties());
        }
    }

    if(is_valid(real_time_c, row)) {
        try {
            connection->duration = boost::lexical_cast<int>(row[real_time_c]);
        } catch (const boost::bad_lexical_cast&) {
            LOG4CPLUS_INFO(logger, "impossible to parse real transfers time duration " << row[real_time_c]);
        }
    }
}

void StopTimeFusioHandler::init(Data& data) {
    StopTimeGtfsHandler::init(data);
    itl_c = csv.get_pos_col("stop_times_itl"),
               desc_c = csv.get_pos_col("stop_desc"),
            date_time_estimated_c = csv.get_pos_col("date_time_estimated");
}

void StopTimeFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    auto stop_times = StopTimeGtfsHandler::handle_line(data, row, is_first_line);
    //gtfs can return many stoptimes for one line because of DST periods
    if (stop_times.empty()) {
        return;
    }
    for (auto stop_time: stop_times) {
        if (is_valid(date_time_estimated_c, row))
            stop_time->date_time_estimated = (row[date_time_estimated_c] == "1");
        else
            stop_time->date_time_estimated = false;

        if ( is_valid(desc_c, row) ){
            auto it_comment = gtfs_data.comment_map.find(row[desc_c]);
            if(it_comment != gtfs_data.comment_map.end()){
                stop_time->comment = it_comment->second;
            }
        }

        if (is_valid(itl_c, row)) {
            uint16_t local_traffic_zone =  boost::lexical_cast<uint16_t>(row[itl_c]);
            if (local_traffic_zone > 0) {
                stop_time->local_traffic_zone = local_traffic_zone;
            }
        }
        else
            stop_time->local_traffic_zone = std::numeric_limits<uint16_t>::max();
    }
}

void GeometriesFusioHandler::init(Data&) {
    LOG4CPLUS_INFO(logger, "Reading geometries");
    geometry_id_c = csv.get_pos_col("geometry_id");
    geometry_wkt_c = csv.get_pos_col("geometry_wkt");
}
void GeometriesFusioHandler::handle_line(Data& data, const csv_row& row, bool) {
    try {
        nt::MultiLineString multiline;
        boost::geometry::read_wkt(row.at(geometry_wkt_c), multiline);
        data.shapes[row.at(geometry_id_c)] = multiline;
    } catch (const boost::geometry::read_wkt_exception&) {
        nt::LineString line;
        boost::geometry::read_wkt(row.at(geometry_wkt_c), line);
        data.shapes[row.at(geometry_id_c)] = {line};
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
    headsign_c = csv.get_pos_col("trip_headsign");
    block_id_c = csv.get_pos_col("block_id");
    comment_id_c = csv.get_pos_col("comment_id");
    trip_propertie_id_c = csv.get_pos_col("trip_property_id");
    odt_type_c = csv.get_pos_col("odt_type");
    company_id_c = csv.get_pos_col("company_id");
    odt_condition_id_c = csv.get_pos_col("odt_condition_id");
    physical_mode_c = csv.get_pos_col("physical_mode_id");
    ext_code_c = csv.get_pos_col("external_code");
    geometry_id_c = csv.get_pos_col("geometry_id");
}

std::vector<ed::types::VehicleJourney*> TripsFusioHandler::get_split_vj(Data& data, const csv_row& row, bool){
    auto it = gtfs_data.route_map.find(row[route_id_c]);
    if (it == gtfs_data.route_map.end()) {
        LOG4CPLUS_WARN(logger, "Impossible to find the route " + row[route_id_c]
                       + " referenced by trip " + row[trip_c]);
        ignored++;
        return {};
    }
    ed::types::Route* route = it->second;

    auto vp_range = gtfs_data.tz.vp_by_name.equal_range(row[service_c]);
    if(empty(vp_range)) {
        LOG4CPLUS_WARN(logger, "Impossible to find the service " + row[service_c]
                       + " referenced by trip " + row[trip_c]);
        ignored++;
        return {};
    }

    //we look in the meta vj table to see if we already have one such vj
    if(data.meta_vj_map.find(row[trip_c]) != data.meta_vj_map.end()) {
        LOG4CPLUS_DEBUG(logger, "a vj with trip id = " << row[trip_c] << " already read, we skip the other one");
        ignored_vj++;
        return {};
    }
    std::vector<ed::types::VehicleJourney*> res;

    types::MetaVehicleJourney& meta_vj = data.meta_vj_map[row[trip_c]]; //we get a ref on a newly created meta vj

    // get shape if possible
    const std::string &shape_id = has_col(geometry_id_c, row) ? row.at(geometry_id_c) : "";

    bool has_been_split = more_than_one_elt(vp_range); //check if the trip has been split over dst

    size_t cpt_vj(1);
    //the validity pattern may have been split because of DST, so we need to create one vj for each
    for (auto vp_it = vp_range.first; vp_it != vp_range.second; ++vp_it, cpt_vj++) {

        ed::types::ValidityPattern* vp_xx = vp_it->second;

        ed::types::VehicleJourney* vj = new ed::types::VehicleJourney();

        const std::string original_uri = row[trip_c];
        std::string vj_uri = original_uri;
        if (has_been_split) {
            //we change the name of the vj (all but the first one) since we had to split the original GTFS vj because of dst
            vj_uri = generate_unique_vj_uri(gtfs_data, original_uri, cpt_vj);
        }
        vj->uri = vj_uri;

        if(is_valid(ext_code_c, row)){
            vj->external_code = row[ext_code_c];
        }
        if(is_valid(headsign_c, row))
            vj->name = row[headsign_c];
        else
            vj->name = vj->uri;

        vj->validity_pattern = vp_xx;
        vj->adapted_validity_pattern = vp_xx;
        vj->journey_pattern = nullptr;
        vj->tmp_route = route;
        vj->tmp_line = vj->tmp_route->line;
        if(is_valid(block_id_c, row))
            vj->block_id = row[block_id_c];
        else
            vj->block_id = "";

        gtfs_data.tz.vj_by_name.insert({original_uri, vj});

        data.vehicle_journeys.push_back(vj);
        res.push_back(vj);
        meta_vj.theoric_vj.push_back(vj);
        vj->meta_vj_name = row[trip_c];
        vj->shape_id = shape_id;

        // we store the split vj utc shift
        auto utc_offset = gtfs_data.tz.offset_by_vp[vp_xx];
        vj->utc_to_local_offset = utc_offset;
    }

    return res;
}

void TripsFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    auto split_vj = TripsFusioHandler::get_split_vj(data, row, is_first_line);

    if (split_vj.empty()) {
        return;
    }

    //the vj might have been split over the dst,thus we loop over all split vj
    for (auto vj: split_vj) {
        if (is_valid(ext_code_c, row)) {
            vj->external_code = row[ext_code_c];
        }

        //if a physical_mode is given we override the value
        vj->physical_mode = nullptr;
        if (is_valid(physical_mode_c, row)){
            auto itm = gtfs_data.physical_mode_map.find(row[physical_mode_c]);
            if (itm == gtfs_data.physical_mode_map.end()) {
                LOG4CPLUS_WARN(logger, "TripsFusioHandler : Impossible to find the physical mode " << row[physical_mode_c]
                               << " referenced by trip " << row[trip_c]);
            } else {
                vj->physical_mode = itm->second;
            }
        }

        if (vj->physical_mode == nullptr) {
            vj->physical_mode = gtfs_data.get_or_create_default_physical_mode(data);
        }

        if (is_valid(odt_condition_id_c, row)){
            auto it_odt_condition = gtfs_data.odt_conditions_map.find(row[odt_condition_id_c]);
            if(it_odt_condition != gtfs_data.odt_conditions_map.end()){
                vj->odt_message = it_odt_condition->second;
            }
        }

        if (is_valid(trip_propertie_id_c, row)) {
            auto it_property = gtfs_data.hasVehicleProperties_map.find(row[trip_propertie_id_c]);
            if(it_property != gtfs_data.hasVehicleProperties_map.end()){
                vj->set_vehicles(it_property->second.vehicles());
            }
        }

        if (is_valid(comment_id_c, row)) {
            auto it_comment = gtfs_data.comment_map.find(row[comment_id_c]);
            if(it_comment != gtfs_data.comment_map.end()){
                vj->comment = it_comment->second;
            }
        }

        if(is_valid(odt_type_c, row)){
            vj->vehicle_journey_type = static_cast<nt::VehicleJourneyType>(boost::lexical_cast<int>(row[odt_type_c]));
        }

        vj->company = nullptr;
        if (is_valid(company_id_c, row)) {
            auto it_company = gtfs_data.company_map.find(row[company_id_c]);
            if(it_company == gtfs_data.company_map.end()){
                LOG4CPLUS_WARN(logger, "TripsFusioHandler : Impossible to find the company " << row[company_id_c]
                               << " referenced by trip " << row[trip_c]);
            } else {
                vj->company = it_company->second;
            }
        }

        if (! vj->company) {
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
}

void ContributorFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    if(! is_first_line && ! has_col(id_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename +
                        "  file has more than one contributor and no contributor_id column");
        throw InvalidHeaders(csv.filename);
    }
    ed::types::Contributor * contributor = new ed::types::Contributor();
    if (is_valid(id_c, row)) {
        contributor->uri = row[id_c];
    } else {
        contributor->uri = "default_contributor";
    }
    contributor->name = row[name_c];
    contributor->idx = data.contributors.size() + 1;
    data.contributors.push_back(contributor);
    gtfs_data.contributor_map[contributor->uri] = contributor;
}

void LineFusioHandler::init(Data &){
    id_c = csv.get_pos_col("line_id");
    name_c = csv.get_pos_col("line_name");
    external_code_c = csv.get_pos_col("external_code");
    code_c =  csv.get_pos_col("line_code");
    forward_name_c =  csv.get_pos_col("forward_line_name");
    backward_name_c =  csv.get_pos_col("backward_line_name");
    color_c = csv.get_pos_col("line_color");
    network_c = csv.get_pos_col("network_id");
    comment_c = csv.get_pos_col("comment_id");
    commercial_mode_c = csv.get_pos_col("commercial_mode_id");
    sort_c = csv.get_pos_col("line_sort");
    geometry_id_c = csv.get_pos_col("geometry_id");
    opening_c = csv.get_pos_col("line_opening_time");
    closing_c = csv.get_pos_col("line_closing_time");
}
void LineFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line){
    if(! is_first_line && ! has_col(id_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename +
                        "  file has more than one line and no line_id column");
        throw InvalidHeaders(csv.filename);
    }
    ed::types::Line * line = new ed::types::Line();
    line->uri = row[id_c];
    line->name = row[name_c];

    if (is_valid(external_code_c, row)) {
        line->external_code = row[external_code_c];
    }
    if (is_valid(code_c, row)) {
        line->code = row[code_c];
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
            throw navitia::exception("line " + line->uri + " has an unknown network: " + row[network_c] + ", the dataset is not valid");
        } else {
            line->network = itm->second;
        }
    }

    if (line->network == nullptr) {
        //if the line has no network data or an empty one, we get_or_create the default one
        line->network = gtfs_data.get_or_create_default_network(data);
    }

    if (is_valid(comment_c, row)) {
        auto itm = gtfs_data.comment_map.find(row[comment_c]);
        if (itm != gtfs_data.comment_map.end()) {
            line->comment = itm->second;
        }
    }

    line->commercial_mode = nullptr;
    if (is_valid(commercial_mode_c, row)) {
        auto itm = gtfs_data.commercial_mode_map.find(row[commercial_mode_c]);
        if(itm == gtfs_data.commercial_mode_map.end()){
            LOG4CPLUS_WARN(logger, "LineFusioHandler : Impossible to find the commercial_mode " << row[commercial_mode_c]
                           << " referenced by line " << row[id_c]);
        }else{
            line->commercial_mode = itm->second;
        }
    }

    if(line->commercial_mode == nullptr){
        line->commercial_mode = gtfs_data.get_or_create_default_commercial_mode(data);
    }
    if (is_valid(sort_c, row) && row[sort_c] != "-1") {
        //sort == -1 means no sort
        line->sort =  boost::lexical_cast<int>(row[sort_c]);
    }
    if (is_valid(opening_c, row)) {
        line->opening_time = boost::posix_time::duration_from_string(row[opening_c]);
    }
    if (is_valid(closing_c, row)) {
        line->closing_time = boost::posix_time::duration_from_string(row[closing_c]);
        while(line->closing_time->hours() >= 24) {
            line->closing_time = *line->closing_time - boost::posix_time::time_duration(24, 0, 0);
        }
    }

    data.lines.push_back(line);
    gtfs_data.line_map[line->uri] = line;
    gtfs_data.line_map_by_external_code[line->external_code] = line;
}

void CompanyFusioHandler::init(Data&) {
    id_c = csv.get_pos_col("company_id"), name_c = csv.get_pos_col("company_name"),
            company_address_name_c = csv.get_pos_col("company_address_name"),
            company_address_number_c = csv.get_pos_col("company_address_number"),
            company_address_type_c = csv.get_pos_col("company_address_type"),
            company_url_c = csv.get_pos_col("company_url"),
            company_mail_c = csv.get_pos_col("company_mail"),
            company_phone_c = csv.get_pos_col("company_phone"),
            company_fax_c = csv.get_pos_col("company_fax");
}

void CompanyFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    if(! is_first_line && ! has_col(id_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename +
                        "  file has more than one company and no company_id column");
        throw InvalidHeaders(csv.filename);
    }
    ed::types::Company * company = new ed::types::Company();
    if(! is_valid(id_c, row)){
        LOG4CPLUS_WARN(logger, "CompanyFusioHandler : Invalid company id " << row[id_c]);
        return;
    }
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
    if(! is_first_line && ! has_col(id_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename +
                        "  file has more than one physical mode and no physical_mode_id column");
        throw InvalidHeaders(csv.filename);
    }
    ed::types::PhysicalMode* mode = new ed::types::PhysicalMode();
    double co2_emission;
    mode->name = row[name_c];
    mode->uri = row[id_c];
    if(has_col(co2_emission_c, row)) {
        try{
            co2_emission = boost::lexical_cast<double>(row[co2_emission_c]);
            if (co2_emission >= 0.){
                mode->co2_emission = co2_emission;
            }
        }
        catch(boost::bad_lexical_cast) {
            LOG4CPLUS_WARN(logger, "Impossible to parse the co2_emission for "
                           + mode->uri + " " + mode->name);
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
    if(! is_first_line && ! has_col(id_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename +
                        "  file has more than one commercial mode and no commercial_mode_id column");
        throw InvalidHeaders(csv.filename);
    }
    ed::types::CommercialMode* commercial_mode = new ed::types::CommercialMode();
    commercial_mode->name = row[name_c];
    commercial_mode->uri = row[id_c];
    data.commercial_modes.push_back(commercial_mode);
    gtfs_data.commercial_mode_map[commercial_mode->uri] = commercial_mode;
}

void CommentFusioHandler::init(Data&){
    id_c = csv.get_pos_col("comment_id");
    comment_c = csv.get_pos_col("comment_name");
}

void CommentFusioHandler::handle_line(Data&, const csv_row& row, bool is_first_line){
    if(! is_first_line && ! has_col(id_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename +
                        "  file has more than one comment and no comment_id column");
        throw InvalidHeaders(csv.filename);
    }
    if(! has_col(comment_c, row)) {
        LOG4CPLUS_INFO(logger, "Error while reading " + csv.filename +
                        "  row has column comment for the id : " + row[id_c]);
        return;
    }
    gtfs_data.comment_map[row[id_c]] = row[comment_c];
}


void OdtConditionsFusioHandler::init(Data&){
    odt_condition_id_c = csv.get_pos_col("odt_condition_id");
    odt_condition_c = csv.get_pos_col("odt_condition");
}

void OdtConditionsFusioHandler::handle_line(Data& , const csv_row& row, bool is_first_line){
    if(! is_first_line && ! has_col(odt_condition_id_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename +
                        "  file has more than one condition and no odt_condition_id_c column");
        throw InvalidHeaders(csv.filename);
    }
    gtfs_data.odt_conditions_map[row[odt_condition_id_c]] = row[odt_condition_c];
}

void StopPropertiesFusioHandler::init(Data&){
    id_c = csv.get_pos_col("property_id");
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

void StopPropertiesFusioHandler::handle_line(Data&, const csv_row& row, bool is_first_line){
    if(! is_first_line && ! has_col(id_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename +
                        "  file has more than one stop_properties and no stop_propertie_id column");
        throw InvalidHeaders(csv.filename);
    }
    auto itm = gtfs_data.hasProperties_map.find(row[id_c]);
    if(itm == gtfs_data.hasProperties_map.end()){
        navitia::type::hasProperties has_properties;
        if(is_active(wheelchair_boarding_c, row))
            has_properties.set_property(navitia::type::hasProperties::WHEELCHAIR_BOARDING);
        if(is_active(sheltered_c, row))
            has_properties.set_property(navitia::type::hasProperties::SHELTERED);
        if(is_active(elevator_c, row))
            has_properties.set_property(navitia::type::hasProperties::ELEVATOR);
        if(is_active(escalator_c, row))
            has_properties.set_property(navitia::type::hasProperties::ESCALATOR);
        if(is_active(bike_accepted_c, row))
            has_properties.set_property(navitia::type::hasProperties::BIKE_ACCEPTED);
        if(is_active(bike_depot_c, row))
            has_properties.set_property(navitia::type::hasProperties::BIKE_DEPOT);
        if(is_active(visual_announcement_c, row))
            has_properties.set_property(navitia::type::hasProperties::VISUAL_ANNOUNCEMENT);
        if(is_active(audible_announcement_c, row))
            has_properties.set_property(navitia::type::hasProperties::AUDIBLE_ANNOUNVEMENT);
        if(is_active(appropriate_escort_c, row))
            has_properties.set_property(navitia::type::hasProperties::APPOPRIATE_ESCORT);
        if(is_active(appropriate_signage_c, row))
            has_properties.set_property(navitia::type::hasProperties::APPOPRIATE_SIGNAGE);
        gtfs_data.hasProperties_map[row[id_c]] = has_properties;
    }
}
void TripPropertiesFusioHandler::init(Data &){
    id_c = csv.get_pos_col("trip_property_id");
    wheelchair_accessible_c = csv.get_pos_col("wheelchair_accessible");
    bike_accepted_c = csv.get_pos_col("bike_accepted");
    air_conditioned_c = csv.get_pos_col("air_conditioned");
    visual_announcement_c = csv.get_pos_col("visual_announcement");
    audible_announcement_c = csv.get_pos_col("audible_announcement");
    appropriate_escort_c = csv.get_pos_col("appropriate_escort");
    appropriate_signage_c = csv.get_pos_col("appropriate_signage");
    school_vehicle_c = csv.get_pos_col("school_vehicle");
}

void TripPropertiesFusioHandler::handle_line(Data&, const csv_row& row, bool is_first_line){
    if(! is_first_line && ! has_col(id_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename +
                        "  file has more than one stop_properties and no stop_propertie_id column");
        throw InvalidHeaders(csv.filename);
    }
    auto itm = gtfs_data.hasVehicleProperties_map.find(row[id_c]);
    if(itm == gtfs_data.hasVehicleProperties_map.end()){
        navitia::type::hasVehicleProperties has_properties;
        if(is_active(wheelchair_accessible_c, row))
            has_properties.set_vehicle(navitia::type::hasVehicleProperties::WHEELCHAIR_ACCESSIBLE);
        if(is_active(bike_accepted_c, row))
            has_properties.set_vehicle(navitia::type::hasVehicleProperties::BIKE_ACCEPTED);
        if(is_active(air_conditioned_c, row))
            has_properties.set_vehicle(navitia::type::hasVehicleProperties::AIR_CONDITIONED);
        if(is_active(visual_announcement_c, row))
            has_properties.set_vehicle(navitia::type::hasVehicleProperties::VISUAL_ANNOUNCEMENT);
        if(is_active(audible_announcement_c, row))
            has_properties.set_vehicle(navitia::type::hasVehicleProperties::AUDIBLE_ANNOUNCEMENT);
        if(is_active(appropriate_escort_c, row))
            has_properties.set_vehicle(navitia::type::hasVehicleProperties::APPOPRIATE_ESCORT);
        if(is_active(appropriate_signage_c, row))
            has_properties.set_vehicle(navitia::type::hasVehicleProperties::APPOPRIATE_SIGNAGE);
        if(is_active(school_vehicle_c, row))
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
    } catch(const boost::bad_lexical_cast& ) {
        LOG4CPLUS_ERROR(logger, "Impossible to parse the date for " << str);
    } catch(const boost::gregorian::bad_day_of_month&) {
        LOG4CPLUS_ERROR(logger, "bad_day_of_month : Impossible to parse the date for " << str);
    } catch(const boost::gregorian::bad_day_of_year&) {
        LOG4CPLUS_ERROR(logger, "bad_day_of_year : Impossible to parse the date for " << str);
    } catch(const boost::gregorian::bad_month&) {
        LOG4CPLUS_ERROR(logger, "bad_month : Impossible to parse the date for " << str);
    } catch(const boost::gregorian::bad_year&) {
        LOG4CPLUS_ERROR(logger, "bad_year : Impossible to parse the date for " << str);
    }
    return boost::gregorian::date(boost::gregorian::not_a_date_time);
}

namespace grid_calendar {

void PeriodFusioHandler::init(Data&) {
    calendar_c = csv.get_pos_col("calendar_id");
    begin_c = csv.get_pos_col("begin_date");
    end_c = csv.get_pos_col("end_date");
}

void PeriodFusioHandler::handle_line(Data&, const csv_row& row, bool is_first_line) {
    if(! is_first_line && ! has_col(calendar_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " << csv.filename <<
                        "  file has more than one period and no calendar_id column");
        throw InvalidHeaders(csv.filename);
    }
    auto cal = gtfs_data.calendars_map.find(row[calendar_c]);
    if (cal == gtfs_data.calendars_map.end()) {
        LOG4CPLUS_ERROR(logger, "GridCalPeriodFusioHandler : Impossible to find the calendar " << row[calendar_c]);
        return;
    }
    boost::gregorian::date begin_date, end_date;
    if(has_col(begin_c, row)) {
        begin_date = parse_date(row[begin_c]);
    }
    if(has_col(end_c, row)){
        end_date = parse_date(row[end_c]);
    }
    if (begin_date.is_not_a_date() || end_date.is_not_a_date()) {
        LOG4CPLUS_ERROR(logger, "period invalid, not added for calendar " << row[calendar_c]);
        return;
    }
    //the end of a gregorian period not in the period (it's the day after)
    //since we want the end to be in the period, we add one day to it
    boost::gregorian::date_period period(begin_date, end_date + boost::gregorian::days(1));
    cal->second->period_list.push_back(period);
}

void GridCalendarFusioHandler::init(Data&) {
    id_c = csv.get_pos_col("id");
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
    if(! is_first_line && ! has_col(id_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename +
                        "  file has more than one calendar and no id column");
        throw InvalidHeaders(csv.filename);
    }
    if (has_col(id_c, row) && row[id_c].empty()) {
        //we don't want empty named calendar as they often are empty lines in reality
        LOG4CPLUS_INFO(logger, "calendar name is empty, we skip it");
        return;
    }
    ed::types::Calendar* calendar = new ed::types::Calendar();
    calendar->uri = row[id_c];
    calendar->external_code = row[id_c];
    calendar->name =  row[name_c];
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

void ExceptionDatesFusioHandler::init(Data &){
    calendar_c = csv.get_pos_col("calendar_id");
    datetime_c = csv.get_pos_col("date");
    type_c = csv.get_pos_col("type");
}

void ExceptionDatesFusioHandler::handle_line(Data&, const csv_row& row, bool is_first_line){
    if(! is_first_line && ! has_col(calendar_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename +
                        "  file has more than one calendar_id and no id column");
        throw InvalidHeaders(csv.filename);
    }
    auto cal = gtfs_data.calendars_map.find(row[calendar_c]);
    if (cal == gtfs_data.calendars_map.end()) {
        LOG4CPLUS_WARN(logger, "ExceptionDatesFusioHandler : Impossible to find the calendar " << row[calendar_c]);
        return;
    }
    if(!has_col(type_c, row)) {
        LOG4CPLUS_WARN(logger, "ExceptionDatesFusioHandler: No column type for calendar " << row[calendar_c]);
        return;
    }
    if (row[type_c] != "0" && row[type_c] != "1") {
        LOG4CPLUS_WARN(logger, "ExceptionDatesFusioHandler : unknown type " << row[type_c]);
        return;
    }
    if(!has_col(datetime_c, row)) {
        LOG4CPLUS_WARN(logger, "ExceptionDatesFusioHandler: No column datetime for calendar " << row[calendar_c]);
        return;
    }
    boost::gregorian::date date(parse_date(row[datetime_c]));
    if(date.is_not_a_date()) {
        LOG4CPLUS_ERROR(logger, "date format not valid, we do not add the exception " <<
                       row[type_c] << " for " << row[calendar_c]);
        return;
    }
    navitia::type::ExceptionDate exception_date;
    exception_date.date = date;
    exception_date.type = static_cast<navitia::type::ExceptionDate::ExceptionType>(boost::lexical_cast<int>(row[type_c]));
    cal->second->exceptions.push_back(exception_date);
}

void CalendarLineFusioHandler::init(Data&){
    calendar_c = csv.get_pos_col("calendar_id");
    line_c = csv.get_pos_col("line_external_code");
}

void CalendarLineFusioHandler::handle_line(Data&, const csv_row& row, bool is_first_line){
    if(! is_first_line && ! has_col(calendar_c, row)) {
        LOG4CPLUS_FATAL(logger, "CalendarLineFusioHandler: Error while reading " + csv.filename +
                        "  file has more than one calendar_id and no id column");
        throw InvalidHeaders(csv.filename);
    }

    auto cal = gtfs_data.calendars_map.find(row[calendar_c]);
    if (cal == gtfs_data.calendars_map.end()) {
        LOG4CPLUS_ERROR(logger, "CalendarLineFusioHandler: Impossible to find the calendar " << row[calendar_c]);
        return;
    }
    if(!has_col(line_c, row)) {
        LOG4CPLUS_WARN(logger, "CalendarLineFusioHandler: No line column for calendar : " << row[calendar_c]);
        return;
    }
    auto it = gtfs_data.line_map_by_external_code.find(row[line_c]);

    if (it == gtfs_data.line_map_by_external_code.end()) {
        LOG4CPLUS_ERROR(logger, "CalendarLineFusioHandler: Impossible to find the line " << row[line_c]);
        return;
    }
    cal->second->line_list.push_back(it->second);
}
}

void AdminStopAreaFusioHandler::init(Data&){
    admin_c = csv.get_pos_col("admin_id");
    stop_area_c = csv.get_pos_col("station_id");

    for (const auto& stop_area : gtfs_data.stop_area_map){
        tmp_stop_area_map[stop_area.second->external_code] = stop_area.second;
    }
}

void AdminStopAreaFusioHandler::handle_line(Data& data, const csv_row& row, bool is_first_line){
    if(! is_first_line && ! has_col(stop_area_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename +
                        "  file has no stop_area_c column");
        throw InvalidHeaders(csv.filename);
    }

    auto sa = tmp_stop_area_map.find(row[stop_area_c]);
    if (sa == tmp_stop_area_map.end()) {
        LOG4CPLUS_ERROR(logger, "AdminStopAreaFusioHandler : Impossible to find the stop_area " << row[stop_area_c]);
        return;
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

    admin_stop_area->stop_area.push_back(sa->second);
}

ed::types::CommercialMode* GtfsData::get_or_create_default_commercial_mode(Data & data) {
    if (! default_commercial_mode) {
        default_commercial_mode = new ed::types::CommercialMode();
        default_commercial_mode->name = "mode commercial par défaut";
        default_commercial_mode->uri = "default_commercial_mode";
        data.commercial_modes.push_back(default_commercial_mode);
        commercial_mode_map[default_commercial_mode->uri] = default_commercial_mode;
    }
    return default_commercial_mode;
}

ed::types::PhysicalMode* GtfsData::get_or_create_default_physical_mode(Data & data) {
    if (! default_physical_mode ) {
        default_physical_mode = new ed::types::PhysicalMode();
        default_physical_mode->name = "mode physique par défaut";
        default_physical_mode->uri = "default_physical_mode";
        data.physical_modes.push_back(default_physical_mode);
        physical_mode_map[default_physical_mode->uri] = default_physical_mode;
    }
    return default_physical_mode;
}

void FusioParser::parse_files(Data& data) {
    parse<GeometriesFusioHandler>(data, "geometries.txt");
    parse<AgencyFusioHandler>(data, "agency.txt", true);
    parse<ContributorFusioHandler>(data, "contributors.txt");
    parse<CompanyFusioHandler>(data, "company.txt");
    parse<PhysicalModeFusioHandler>(data, "physical_modes.txt");
    parse<CommercialModeFusioHandler>(data, "commercial_modes.txt");
    parse<CommentFusioHandler>(data, "comments.txt");
    parse<LineFusioHandler>(data, "lines.txt");
    parse<StopPropertiesFusioHandler>(data, "stop_properties.txt");
    parse<StopsFusioHandler>(data, "stops.txt", true);
    parse<RouteFusioHandler>(data, "routes.txt", true);
    parse<TransfersFusioHandler>(data, "transfers.txt");

    parse<CalendarGtfsHandler>(data, "calendar.txt");
    parse<CalendarDatesGtfsHandler>(data, "calendar_dates.txt");
    //after the calendar load, we need to split the validitypattern
    split_validity_pattern_over_dst(data, gtfs_data);

    parse<TripPropertiesFusioHandler>(data, "trip_properties.txt");
    parse<OdtConditionsFusioHandler>(data, "odt_conditions.txt");
    parse<TripsFusioHandler>(data, "trips.txt", true);
    parse<StopTimeFusioHandler>(data, "stop_times.txt", true);
    parse<FrequenciesGtfsHandler>(data, "frequencies.txt");
    parse<grid_calendar::GridCalendarFusioHandler>(data, "grid_calendars.txt");
    parse<grid_calendar::PeriodFusioHandler>(data, "grid_periods.txt");
    parse<grid_calendar::ExceptionDatesFusioHandler>(data, "grid_exception_dates.txt");
    parse<grid_calendar::CalendarLineFusioHandler>(data, "grid_rel_calendar_line.txt");
    parse<AdminStopAreaFusioHandler>(data, "admin_stations.txt");
}
}
}
