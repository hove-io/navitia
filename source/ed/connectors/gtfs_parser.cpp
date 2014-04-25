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

#include "gtfs_parser.h"
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <fstream>
#include <iostream>
#include <set>
#include "utils/encoding_converter.h"
#include "utils/csv.h"
#include "utils/logger.h"

namespace nm = ed::types;
typedef boost::tokenizer< boost::escaped_list_separator<char> > Tokenizer;

namespace ed{ namespace connectors {


int time_to_int(const std::string & time) {
    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
    boost::char_separator<char> sep(":");
    tokenizer tokens(time, sep);
    std::vector<std::string> elts(tokens.begin(), tokens.end());
    int result = 0;
    if(elts.size() != 3)
        return -1;
    try {
        result = boost::lexical_cast<int>(elts[0]) * 3600;
        result += boost::lexical_cast<int>(elts[1]) * 60;
        result += boost::lexical_cast<int>(elts[2]);
    }
    catch(boost::bad_lexical_cast){
        return -1;
    }
    return result;
}

void AgencyGtfsHandler::init(Data&) {
    id_c = csv.get_pos_col("agency_id");
    name_c = csv.get_pos_col("agency_name");
}

ed::types::Network* AgencyGtfsHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    if(! is_first_line && ! has_col(id_c, row)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " + csv.filename +
                        + " file has more than one agency and no agency_id column");
        throw InvalidHeaders(csv.filename);
    }
    nm::Network * network = new nm::Network();

    if(has_col(id_c, row)) {
        network->uri = row[id_c];
    } else {
        network->uri = "default_agency";
    }
    network->external_code = network->uri;

    network->name = row[name_c];
    data.networks.push_back(network);

    gtfs_data.agency_map[network->uri] = network;
    return network;
}

void DefaultContributorHandler::init(Data& data) {
    nm::Contributor * contributor = new nm::Contributor();
    contributor->uri = "default_contributor";
    contributor->name = "default_contributor";
    contributor->idx = data.contributors.size() + 1;
    data.contributors.push_back(contributor);
    gtfs_data.contributor_map[contributor->uri] = contributor;
}

void StopsGtfsHandler::init(Data& data) {
    // we allocate with values sized for the Île-de-France
    data.stop_points.reserve(56000);
    data.stop_areas.reserve(13000);
    id_c = csv.get_pos_col("stop_id"),
            code_c = csv.get_pos_col("stop_code"),
            lat_c = csv.get_pos_col("stop_lat"),
            lon_c = csv.get_pos_col("stop_lon"),
            type_c = csv.get_pos_col("location_type"),
            parent_c = csv.get_pos_col("parent_station"),
            name_c = csv.get_pos_col("stop_name"),
            desc_c = csv.get_pos_col("stop_desc"),
            wheelchair_c = csv.get_pos_col("wheelchair_boarding");
    if (code_c == -1) {
        code_c = id_c;
    }
}

void StopsGtfsHandler::finish(Data& data) {
    // On reboucle pour récupérer les stop areas de tous les stop points
    for(auto sa_sps : gtfs_data.sa_spmap) {
        auto it = gtfs_data.stop_area_map.find(sa_sps.first);
        if(it != gtfs_data.stop_area_map.end()) {
            for(auto sp : sa_sps.second) {
                sp->stop_area = it->second;
            }
        } else {
            std::string error_message =
                    "the stop area " + sa_sps.first  + " has not been found for the stop points :  ";
            for(auto sp : sa_sps.second) {
                error_message += sp->uri;
                sp->stop_area = 0;
            }
            LOG4CPLUS_WARN(logger, error_message);
        }
    }

    //On va chercher l'accessibilité pour les stop points qui hérite de l'accessibilité de leur stop area
    for (auto sp : wheelchair_heritance) {
        if(sp->stop_area == nullptr)
            continue;
        if (sp->stop_area->property(navitia::type::hasProperties::WHEELCHAIR_BOARDING)) {
            sp->set_property(navitia::type::hasProperties::WHEELCHAIR_BOARDING);
        } else {
            LOG4CPLUS_WARN(logger, "Impossible to get the stop area accessibility value for the stop point " + sp->uri);
        }
    }

    handle_stop_point_without_area(data);

    LOG4CPLUS_TRACE(logger, data.stop_points.size() << " added stop points");;
    LOG4CPLUS_TRACE(logger, data.stop_areas.size() << " added stop areas");
    LOG4CPLUS_TRACE(logger, ignored << " points ignored because of dupplicates" );
}

void StopsGtfsHandler::handle_stop_point_without_area(Data& data) {
    //we have to check if there was stop area in the file
    bool has_no_stop_areas = data.stop_areas.empty();
    if (has_no_stop_areas) {
        for (const auto sp : data.stop_points) {
            if (sp->stop_area) {
                has_no_stop_areas = false;
                break;
            }
        }
    }

    if (has_no_stop_areas) {
        //we artificialy create one stop_area by stop point
        for (const auto sp : data.stop_points) {
            auto sa = new nm::StopArea;

            sa->coord.set_lon(sp->coord.lon());
            sa->coord.set_lat(sp->coord.lat());
            sa->name = sp->name;
            sa->uri = sp->uri;
            if (sp->property(navitia::type::hasProperties::WHEELCHAIR_BOARDING))
                sa->set_property(navitia::type::hasProperties::WHEELCHAIR_BOARDING);

            gtfs_data.stop_area_map[sa->uri] = sa;
            data.stop_areas.push_back(sa);
            sp->stop_area = sa;
        }
        return;
    }

    //Deletion of the stop point without stop areas
    std::vector<size_t> erasest;
    for (int i = data.stop_points.size()-1; i >=0;--i) {
        if (data.stop_points[i]->stop_area == nullptr) {
            erasest.push_back(i);
        }
    }
    int num_elements = data.stop_points.size();
    for (size_t to_erase : erasest) {
        gtfs_data.stop_map.erase(data.stop_points[to_erase]->uri);
        delete data.stop_points[to_erase];
        data.stop_points[to_erase] = data.stop_points[num_elements - 1];
        num_elements--;
    }
    data.stop_points.resize(num_elements);
    LOG4CPLUS_INFO(logger, "Deletion of " << erasest.size() << " stop_point wihtout stop_area");
}

template <typename T>
bool StopsGtfsHandler::parse_common_data(const csv_row& row, T* stop) {
    try{
        stop->coord.set_lon(boost::lexical_cast<double>(row[lon_c]));
        stop->coord.set_lat(boost::lexical_cast<double>(row[lat_c]));
    }
    catch(boost::bad_lexical_cast ) {
        LOG4CPLUS_WARN(logger, "Impossible to parse the coordinate for "
                       + row[id_c] + " " + row[code_c] + " " + row[name_c]);
        return false;
    }
    if(!stop->coord.is_valid()) {
        LOG4CPLUS_WARN(logger, "The stop " + row[id_c] + " " + row[code_c] + " " + row[name_c] +
                       " has been ignored because the coordinates are not valid (" + row[lon_c] + ", " + row[lat_c] + ")");
        return false;
    }

    stop->name = row[name_c];
    stop->uri = row[id_c];
    stop->external_code = stop->uri;
    if (has_col(desc_c, row))
        stop->comment = row[desc_c];
    return true;
}

StopsGtfsHandler::stop_point_and_area StopsGtfsHandler::handle_line(Data& data, const csv_row& row, bool) {
    // In GTFS the file contains the stop_area and the stop_point
    // We test if it's a dupplicate
    if (gtfs_data.stop_map.find(row[id_c]) != gtfs_data.stop_map.end() ||
            gtfs_data.stop_area_map.find(row[id_c]) != gtfs_data.stop_area_map.end()) {
        LOG4CPLUS_WARN(logger, "The stop " + row[id_c] +" has been ignored");
        ignored++;
        return {};
    }

    stop_point_and_area return_wrapper {};
    // Si c'est un stopArea
    if (has_col(type_c, row) && row[type_c] == "1") {
        nm::StopArea * sa = new nm::StopArea();
        if (! parse_common_data(row, sa)) {
            delete sa; //don't forget to free the data
            return {};
        }

        if (has_col(wheelchair_c, row) && row[wheelchair_c] == "1")
            sa->set_property(navitia::type::hasProperties::WHEELCHAIR_BOARDING);
        gtfs_data.stop_area_map[sa->uri] = sa;
        data.stop_areas.push_back(sa);
        return_wrapper.second = sa;
    }
    // C'est un StopPoint
    else {
        nm::StopPoint* sp = new nm::StopPoint();
        if (! parse_common_data(row, sp)) {
            delete sp;
            return {};
        }

        if (has_col(wheelchair_c, row)) {
            if (row[wheelchair_c] == "0") {
                wheelchair_heritance.push_back(sp);
            } else if (row[wheelchair_c] == "1") {
                sp->set_property(navitia::type::hasProperties::WHEELCHAIR_BOARDING);
            }
        }
        gtfs_data.stop_map[sp->uri] = sp;
        data.stop_points.push_back(sp);
        if (has_col(parent_c, row) && row[parent_c] != "") {// we save the reference to the stop area
            auto it = gtfs_data.sa_spmap.find(row[parent_c]);
            if (it == gtfs_data.sa_spmap.end()) {
                it = gtfs_data.sa_spmap.insert(std::make_pair(row[parent_c], GtfsData::vector_sp())).first;
            }
            it->second.push_back(sp);
        }
        return_wrapper.first = sp;
    }
    return return_wrapper;
}

void RouteGtfsHandler::init(Data&) {
    id_c = csv.get_pos_col("route_id"), short_name_c = csv.get_pos_col("route_short_name"),
            long_name_c = csv.get_pos_col("route_long_name"), type_c = csv.get_pos_col("route_type"),
            desc_c = csv.get_pos_col("route_desc"),
            color_c = csv.get_pos_col("route_color"), agency_c = csv.get_pos_col("agency_id");
}

nm::Line* RouteGtfsHandler::handle_line(Data& data, const csv_row& row, bool) {
    if(gtfs_data.line_map.find(row[id_c]) != gtfs_data.line_map.end()) {
        ignored++;
        LOG4CPLUS_WARN(logger, "dupplicate on route line " + row[id_c]);
        return nullptr;
    }

    nm::Line* line = new nm::Line();
    line->uri = row[id_c];
    line->external_code = line->uri;
    line->name = row[long_name_c];
    line->code = row[short_name_c];
    if ( has_col(desc_c, row) )
        line->comment = row[desc_c];

    if(has_col(color_c, row))
        line->color = row[color_c];
    line->additional_data = row[long_name_c];

    auto it_commercial_mode = gtfs_data.commercial_mode_map.find(row[type_c]);
    if (it_commercial_mode != gtfs_data.commercial_mode_map.end())
        line->commercial_mode = it_commercial_mode->second;
    else {
        LOG4CPLUS_ERROR(logger, "impossible to find route type " << row[type_c]
                        << " we ignore the route " << row[id_c]);
        ignored++;
        delete line;
        return nullptr;
    }

    if(has_col(agency_c, row)) {
        auto agency_it = gtfs_data.agency_map.find(row[agency_c]);
        if(agency_it != gtfs_data.agency_map.end())
            line->network = agency_it->second;
    }
    else {
        auto agency_it = gtfs_data.agency_map.find("default_agency");
        if(agency_it != gtfs_data.agency_map.end())
            line->network = agency_it->second;
    }

    gtfs_data.line_map[row[id_c]] = line;
    data.lines.push_back(line);
    return line;
}

void RouteGtfsHandler::finish(Data& data) {
    BOOST_ASSERT(data.lines.size() == gtfs_data.line_map.size());
    LOG4CPLUS_TRACE(logger, "Nb routes: " << data.lines.size());
    LOG4CPLUS_TRACE(logger, ignored << " routes have been ignored because of dupplicates");
}

void TransfersGtfsHandler::init(Data&) {
    from_c = csv.get_pos_col("from_stop_id"),
            to_c = csv.get_pos_col("to_stop_id"),
            time_c = csv.get_pos_col("min_transfer_time");
}

void TransfersGtfsHandler::finish(Data& data) {
    LOG4CPLUS_TRACE(logger, nblines << " correspondances prises en compte sur "
                    << data.stop_point_connections.size());
}

void TransfersGtfsHandler::fill_stop_point_connection(nm::StopPointConnection* connection, const csv_row& row) const {
    if(has_col(time_c, row)) {
        try{
            connection->display_duration = boost::lexical_cast<int>(row[time_c]);
        } catch (...) {
            connection->display_duration = 120;
        }
    } else {
        connection->display_duration = 120;
    }
}

void TransfersGtfsHandler::handle_line(Data& data, const csv_row& row, bool) {
    GtfsData::vector_sp departures, arrivals;
    auto it = gtfs_data.stop_map.find(row[from_c]);
    if(it == gtfs_data.stop_map.end()){
        auto it_sa = gtfs_data.sa_spmap.find(row[from_c]);
        if(it_sa == gtfs_data.sa_spmap.end()) {
            LOG4CPLUS_WARN(logger, "Impossible de find the stop point (from) " + row[from_c]);
            return;
        }
        departures = it_sa->second;
    } else {
        departures.push_back(it->second);
    }

    it = gtfs_data.stop_map.find(row[to_c]);
    if(it == gtfs_data.stop_map.end()){
        auto it_sa = gtfs_data.sa_spmap.find(row[to_c]);
        if(it_sa == gtfs_data.sa_spmap.end()) {
            LOG4CPLUS_WARN(logger, "Impossible de find the stop point (to) " + row[to_c]);
            return;
        }
        arrivals = it_sa->second;
    } else {
        arrivals.push_back(it->second);
    }

    for(auto from_sp : departures) {
        for(auto to_sp : arrivals) {
            nm::StopPointConnection * connection = new nm::StopPointConnection();
            connection->departure = from_sp;
            connection->destination  = to_sp;
            connection->uri = from_sp->uri + "=>"+ to_sp->uri;
            connection->connection_kind = types::ConnectionType::Walking;

            fill_stop_point_connection(connection, row);

            data.stop_point_connections.push_back(connection);
        }
    }
    nblines++;
}

void CalendarGtfsHandler::init(Data& data) {
    data.validity_patterns.reserve(10000);

    id_c = csv.get_pos_col("service_id"), monday_c = csv.get_pos_col("monday"),
            tuesday_c = csv.get_pos_col("tuesday"), wednesday_c = csv.get_pos_col("wednesday"),
            thursday_c = csv.get_pos_col("thursday"), friday_c = csv.get_pos_col("friday"),
            saturday_c = csv.get_pos_col("saturday"), sunday_c = csv.get_pos_col("sunday"),
            start_date_c = csv.get_pos_col("start_date"), end_date_c = csv.get_pos_col("end_date");
}

void CalendarGtfsHandler::finish(Data& data) {
    LOG4CPLUS_TRACE(logger, "Nb validity patterns : " << data.validity_patterns.size() << " nb lines : " << nblignes);
    BOOST_ASSERT(data.validity_patterns.size() == gtfs_data.vp_map.size());
}

void CalendarGtfsHandler::handle_line(Data& data, const csv_row& row, bool) {
    if (row.empty())
        return;
    std::bitset<7> week;
    nblignes ++;

    if (gtfs_data.vp_map.find(row[id_c]) != gtfs_data.vp_map.end()) {
        return;
    }
    //On initialise la semaine
    week[1] = (row[monday_c] == "1");
    week[2] = (row[tuesday_c] == "1");
    week[3] = (row[wednesday_c] == "1");
    week[4] = (row[thursday_c] == "1");
    week[5] = (row[friday_c] == "1");
    week[6] = (row[saturday_c] == "1");
    week[0] = (row[sunday_c] == "1");

    nm::ValidityPattern * vp = new nm::ValidityPattern(gtfs_data.production_date.begin());

    for(unsigned int i = 0; i<366;++i)
        vp->remove(i);

    //Initialisation des jours de la date de départ jusqu'à la date de fin du service
    boost::gregorian::date b_date = boost::gregorian::from_undelimited_string(row[start_date_c]);
    boost::gregorian::date_period period = boost::gregorian::date_period(
                (b_date > gtfs_data.production_date.begin() ? b_date : gtfs_data.production_date.begin()), boost::gregorian::from_undelimited_string(row[end_date_c]));
    for(boost::gregorian::day_iterator it(period.begin()); it<period.end(); ++it) {
        if(week.test((*it).day_of_week())) {
            vp->add((*it));
        }
        else
            vp->remove((*it));
    }

    vp->uri = row[id_c];
    gtfs_data.vp_map[row[id_c]] = vp;
    data.validity_patterns.push_back(vp);
}

void CalendarDatesGtfsHandler::init(Data&) {
    id_c = csv.get_pos_col("service_id"), date_c = csv.get_pos_col("date"),
            e_type_c = csv.get_pos_col("exception_type");
}

void CalendarDatesGtfsHandler::finish(Data& data) {
    LOG4CPLUS_TRACE(logger, "Nb validity patterns : " << data.validity_patterns.size());
    BOOST_ASSERT(data.validity_patterns.size() == gtfs_data.vp_map.size());
    if (data.validity_patterns.empty())
        LOG4CPLUS_FATAL(logger, "No validity_patterns");
}

void CalendarDatesGtfsHandler::handle_line(Data& data, const csv_row& row, bool) {
    if(row.empty())
        return;
    nm::ValidityPattern* vp;
    std::unordered_map<std::string, nm::ValidityPattern*>::iterator it = gtfs_data.vp_map.find(row[id_c]);
    if(it == gtfs_data.vp_map.end()){
        vp = new nm::ValidityPattern(gtfs_data.production_date.begin());
        gtfs_data.vp_map[row[id_c]] = vp;
        data.validity_patterns.push_back(vp);
    }
    else {
        vp = it->second;
    }

    auto date = boost::gregorian::from_undelimited_string(row[date_c]);
    if(row[e_type_c] == "1")
        vp->add(date);
    else if(row[e_type_c] == "2")
        vp->remove(date);
    else
        LOG4CPLUS_WARN(logger, "Exception pour le service " << row[id_c] << " inconnue : " << row[e_type_c]);
}

void TripsGtfsHandler::init(Data& data) {
    data.vehicle_journeys.reserve(350000);

    id_c = csv.get_pos_col("route_id"), service_c = csv.get_pos_col("service_id"),
            trip_c = csv.get_pos_col("trip_id"), headsign_c = csv.get_pos_col("trip_headsign"),
            block_id_c = csv.get_pos_col("block_id"), wheelchair_c = csv.get_pos_col("wheelchair_accessible");
}

void TripsGtfsHandler::finish(Data& data) {
    BOOST_ASSERT(data.vehicle_journeys.size() == gtfs_data.vj_map.size());
    LOG4CPLUS_TRACE(logger, "Nb vehicle journeys : " << data.vehicle_journeys.size());
    LOG4CPLUS_TRACE(logger, "Nb errors on service reference " << ignored);
    LOG4CPLUS_TRACE(logger, ignored_vj << " duplicated vehicule journey have been ignored");
}

nm::VehicleJourney* TripsGtfsHandler::handle_line(Data& data, const csv_row& row, bool) {
    auto it = gtfs_data.line_map.find(row[id_c]);
    if (it == gtfs_data.line_map.end()) {
        LOG4CPLUS_WARN(logger, "Impossible to find the Gtfs route " + row[id_c]
                       + " referenced by trip " + row[trip_c]);
        ignored++;
        return nullptr;
    }

    nm::Line* line = it->second;

    auto vp_it = gtfs_data.vp_map.find(row[service_c]);
    if(vp_it == gtfs_data.vp_map.end()) {
        LOG4CPLUS_WARN(logger, "Impossible to find the Gtfs service " + row[service_c]
                       + " referenced by trip " + row[trip_c]);
        ignored++;
        return nullptr;
    }
    nm::ValidityPattern* vp_xx = vp_it->second;

    auto vj_it = gtfs_data.vj_map.find(row[trip_c]);
    if(vj_it != gtfs_data.vj_map.end()) {
        ignored_vj++;
        return nullptr;
    }
    nm::VehicleJourney* vj = new nm::VehicleJourney();
    vj->uri = row[trip_c];
    vj->external_code = vj->uri;
    if(has_col(headsign_c, row))
        vj->name = row[headsign_c];
    else
        vj->name = vj->uri;

    vj->validity_pattern = vp_xx;
    vj->adapted_validity_pattern = vp_xx;
    vj->journey_pattern = 0;
    vj->tmp_line = line;
    if(has_col(block_id_c, row))
        vj->block_id = row[block_id_c];
    else
        vj->block_id = "";
    //                    if(has_col(wheelchair_c, row))
    //                        vj->wheelchair_boarding = row[wheelchair_c] == "1";
    if(has_col(wheelchair_c, row) && row[wheelchair_c] == "1")
        vj->set_vehicle(navitia::type::hasVehicleProperties::WHEELCHAIR_ACCESSIBLE);

    auto itm = gtfs_data.physical_mode_map.find(line->commercial_mode->uri);
    if (itm != gtfs_data.physical_mode_map.end()){
        vj->physical_mode = itm->second;
    }

    auto company_it = gtfs_data.company_map.find("default_company");
    if (company_it != gtfs_data.company_map.end()){
        vj->company = company_it->second;
    } else {
        //with no information, we take the first company attached to the line
        vj->company = line->company;
    }

    gtfs_data.vj_map[vj->uri] = vj;

    data.vehicle_journeys.push_back(vj);
    return vj;
}

void StopTimeGtfsHandler::init(Data&) {
    id_c = csv.get_pos_col("trip_id"),
            arrival_c = csv.get_pos_col("arrival_time"),
            departure_c = csv.get_pos_col("departure_time"),
            stop_c = csv.get_pos_col("stop_id"),
            stop_seq_c = csv.get_pos_col("stop_sequence"),
            pickup_c = csv.get_pos_col("pickup_type"),
            drop_off_c = csv.get_pos_col("drop_off_type");
}

void StopTimeGtfsHandler::finish(Data& data) {
    LOG4CPLUS_TRACE(logger, "Nombre d'horaires : " << data.stops.size());
}

nm::StopTime* StopTimeGtfsHandler::handle_line(Data& data, const csv_row& row, bool) {
    auto vj_it = gtfs_data.vj_map.find(row[id_c]);
    if(vj_it == gtfs_data.vj_map.end()) {
        LOG4CPLUS_WARN(logger, "Impossible to find the vehicle_journey " + row[id_c]);
        return nullptr;
    }
    auto stop_it = gtfs_data.stop_map.find(row[stop_c]);
    if(stop_it == gtfs_data.stop_map.end()){
        LOG4CPLUS_WARN(logger, "Impossible to find the stop_point " + row[stop_c] + "!");
        return nullptr;
    }
    nm::StopTime* stop_time = new nm::StopTime();
    stop_time->arrival_time = time_to_int(row[arrival_c]);
    stop_time->departure_time = time_to_int(row[departure_c]);
    stop_time->tmp_stop_point = stop_it->second;
    //stop_time->journey_pattern_point = journey_pattern_point;
    stop_time->order = boost::lexical_cast<int>(row[stop_seq_c]);
    stop_time->vehicle_journey = vj_it->second;

    if(has_col(pickup_c, row) && has_col(drop_off_c, row))
        stop_time->ODT = (row[pickup_c] == "2" && row[drop_off_c] == "2");
    else
        stop_time->ODT = false;
    if(has_col(pickup_c, row))
        stop_time->pick_up_allowed = row[pickup_c] != "1";
    else
        stop_time->pick_up_allowed = true;
    if(has_col(drop_off_c, row))
        stop_time->drop_off_allowed = row[drop_off_c] != "1";
    else
        stop_time->drop_off_allowed = true;


    stop_time->vehicle_journey->stop_time_list.push_back(stop_time);
    stop_time->wheelchair_boarding = stop_time->vehicle_journey->wheelchair_boarding;
    data.stops.push_back(stop_time);
    count++;
    return stop_time;
}

void FrequenciesGtfsHandler::init(Data&) {
    trip_id_c = csv.get_pos_col("trip_id"), start_time_c = csv.get_pos_col("start_time"),
    end_time_c = csv.get_pos_col("end_time"), headway_secs_c = csv.get_pos_col("headway_secs");
}

void FrequenciesGtfsHandler::handle_line(Data&, const csv_row& row, bool) {
    if(row.empty())
        return;
    auto vj_it = gtfs_data.vj_map.find(row[trip_id_c]);
    if(vj_it != gtfs_data.vj_map.end()) {
        int begin = vj_it->second->stop_time_list.front()->arrival_time;
        for(auto st_it = vj_it->second->stop_time_list.begin(); st_it != vj_it->second->stop_time_list.end(); ++st_it) {
            (*st_it)->start_time = time_to_int(row[start_time_c]) + (*st_it)->arrival_time - begin;
            (*st_it)->end_time = time_to_int(row[end_time_c]) + (*st_it)->arrival_time - begin;
            (*st_it)->headway_secs = boost::lexical_cast<int>(row[headway_secs_c]);
            (*st_it)->is_frequency = true;
        }
    }
}

GenericGtfsParser::GenericGtfsParser(const std::string & path) : path(path){
    logger = log4cplus::Logger::getInstance("log");
}

void GenericGtfsParser::fill(Data& data, const std::string beginning_date) {

    try {
        gtfs_data.production_date = find_production_date(beginning_date);
    } catch (...) {
        if(beginning_date == "") {
            LOG4CPLUS_FATAL(logger, "Impossible to find the production date");
            return;
        }
    }

    parse_files(data);

    normalize_extcodes(data);
}

void GenericGtfsParser::fill_default_company(Data & data){
    // création d'une compagnie par defaut
    ed::types::Company * company = new ed::types::Company();
    company->uri = "default_company";
    company->name = "compagnie par défaut";
    data.companies.push_back(company);
    gtfs_data.company_map[company->uri] = company;
}

void GenericGtfsParser::fill_default_modes(Data& data){

    // commercial_mode et physical_mode : par défaut
    ed::types::CommercialMode* commercial_mode = new ed::types::CommercialMode();
    commercial_mode->id = "0";
    commercial_mode->name = "Tram";
    commercial_mode->uri = "0";
    data.commercial_modes.push_back(commercial_mode);
    gtfs_data.commercial_mode_map[commercial_mode->id] = commercial_mode;

    commercial_mode = new ed::types::CommercialMode();
    commercial_mode->id = "1";
    commercial_mode->name = "Metro";
    commercial_mode->uri = "1";
    data.commercial_modes.push_back(commercial_mode);
    gtfs_data.commercial_mode_map[commercial_mode->id] = commercial_mode;

    commercial_mode = new ed::types::CommercialMode();
    commercial_mode->id = "2";
    commercial_mode->name = "Rail";
    commercial_mode->uri = "2";
    data.commercial_modes.push_back(commercial_mode);
    gtfs_data.commercial_mode_map[commercial_mode->id] = commercial_mode;

    commercial_mode = new ed::types::CommercialMode();
    commercial_mode->id = "3";
    commercial_mode->name = "Bus";
    commercial_mode->uri = "3";
    data.commercial_modes.push_back(commercial_mode);
    gtfs_data.commercial_mode_map[commercial_mode->id] = commercial_mode;

    commercial_mode = new ed::types::CommercialMode();
    commercial_mode->id = "4";
    commercial_mode->name = "Ferry";
    commercial_mode->uri = "4";
    data.commercial_modes.push_back(commercial_mode);
    gtfs_data.commercial_mode_map[commercial_mode->id] = commercial_mode;

    commercial_mode = new ed::types::CommercialMode();
    commercial_mode->id = "5";
    commercial_mode->name = "Cable car";
    commercial_mode->uri = "5";
    data.commercial_modes.push_back(commercial_mode);
    gtfs_data.commercial_mode_map[commercial_mode->id] = commercial_mode;

    commercial_mode = new ed::types::CommercialMode();
    commercial_mode->id = "6";
    commercial_mode->name = "Gondola";
    commercial_mode->uri = "6";
    data.commercial_modes.push_back(commercial_mode);
    gtfs_data.commercial_mode_map[commercial_mode->uri] = commercial_mode;

    commercial_mode = new ed::types::CommercialMode();
    commercial_mode->id = "7";
    commercial_mode->name = "Funicular";
    commercial_mode->uri = "7";
    data.commercial_modes.push_back(commercial_mode);
    gtfs_data.commercial_mode_map[commercial_mode->uri] = commercial_mode;

    for(ed::types::CommercialMode *mt : data.commercial_modes) {
        ed::types::PhysicalMode* mode = new ed::types::PhysicalMode();
        mode->id = mt->id;
        mode->name = mt->name;
        mode->uri = mt->uri;
        data.physical_modes.push_back(mode);
        gtfs_data.physical_mode_map[mode->uri] = mode;
    }
}

void normalize_extcodes(Data & data) {
    for(nm::StopArea * sa : data.stop_areas){
        boost::algorithm::replace_first(sa->uri, "StopArea:", "");
    }
    for(nm::StopPoint * sp : data.stop_points){
        boost::algorithm::replace_first(sp->uri, "StopPoint:", "");
    }
}

boost::gregorian::date_period GenericGtfsParser::basic_production_date(const std::string & beginning_date) {
    if(beginning_date == "") {
        throw UnableToFindProductionDateException();
    } else {
        boost::gregorian::date b_date(boost::gregorian::from_undelimited_string(beginning_date)),
                e_date(b_date + boost::gregorian::date_duration(365));

        return boost::gregorian::date_period(b_date, e_date);
    }
}

boost::gregorian::date_period GenericGtfsParser::find_production_date(const std::string &beginning_date) {
    std::string filename = path + "/stop_times.txt";
    CsvReader csv(filename, ',' , true);
    if(!csv.is_open()) {
        LOG4CPLUS_WARN(logger, "Aucun fichier " + filename);
        return basic_production_date(beginning_date);
    }

    std::vector<std::string> mandatory_headers = {"trip_id" , "arrival_time",
                                                  "departure_time", "stop_id", "stop_sequence"};
    if(!csv.validate(mandatory_headers)) {
        LOG4CPLUS_FATAL(logger, "Erreur lors du parsing de " + filename
                        + ". Il manque les colonnes : "
                        + csv.missing_headers(mandatory_headers));
        return basic_production_date(beginning_date);
    }


    std::map<std::string, bool> trips;
    int trip_c = csv.get_pos_col("trip_id");
    while(!csv.eof()) {
        auto row = csv.next();
        if(!row.empty())
            trips.insert(std::make_pair(row[trip_c], true));
    }

    filename = path + "/trips.txt";
    CsvReader csv2(filename, ',' , true);
    if(!csv2.is_open()) {
        LOG4CPLUS_WARN(logger, "Aucun fichier " + filename);
        return basic_production_date(beginning_date);
    }

    mandatory_headers = {"trip_id" , "service_id"};
    if(!csv2.validate(mandatory_headers)) {
        LOG4CPLUS_WARN(logger, "Erreur lors du parsing de " + filename
                       + ". Il manque les colonnes : "
                       + csv2.missing_headers(mandatory_headers));
        return basic_production_date(beginning_date);
    }
    int service_c = csv2.get_pos_col("service_id");
    trip_c = csv2.get_pos_col("trip_id");
    std::map<std::string, bool> services;
    while(!csv2.eof()) {
        auto row = csv2.next();
        if(!row.empty() && trips.find(row[trip_c]) != trips.end())
            services.insert(std::make_pair(row[service_c], true));
    }
    boost::gregorian::date start_date(boost::gregorian::max_date_time), end_date(boost::gregorian::min_date_time);

    filename = path + "/calendar.txt";
    CsvReader csv3(filename, ',' , true);
    bool calendar_txt_exists = false;
    if(!csv3.is_open()) {
        LOG4CPLUS_WARN(logger, "Aucun fichier " + filename);
    } else {
        calendar_txt_exists = true;
        mandatory_headers = {"start_date" , "end_date", "service_id"};
        if(!csv3.validate(mandatory_headers)) {
            LOG4CPLUS_WARN(logger, "Erreur lors du parsing de " + filename
                           + ". Il manque les colonnes : "
                           + csv3.missing_headers(mandatory_headers));
            return basic_production_date(beginning_date);
        }

        int start_date_c = csv3.get_pos_col("start_date"), end_date_c = csv3.get_pos_col("end_date");
        service_c = csv3.get_pos_col("service_id");


        while(!csv3.eof()) {
            auto row = csv3.next();
            if(!row.empty()) {
                if(services.find(row[service_c]) != services.end()) {
                    boost::gregorian::date current_start_date = boost::gregorian::from_undelimited_string(row[start_date_c]);
                    boost::gregorian::date current_end_date = boost::gregorian::from_undelimited_string(row[end_date_c]);

                    if(current_start_date < start_date){
                        start_date = current_start_date;
                    }
                    if(current_end_date > end_date){
                        end_date = current_end_date;
                    }
                }
            }
        }
    }

    filename = path + "/calendar_dates.txt";
    CsvReader csv4(filename, ',' , true);
    if(!csv4.is_open()) {
        if(calendar_txt_exists)
            LOG4CPLUS_WARN(logger, "Aucun fichier " + filename);
        else
            LOG4CPLUS_FATAL(logger, "Aucun fichiers " + filename + " ni calendar.txt");
    } else {
        mandatory_headers = {"service_id" , "date", "exception_type"};
        if(!csv4.validate(mandatory_headers)) {
            LOG4CPLUS_WARN(logger, "Erreur lors du parsing de " + filename
                           + ". Il manque les colonnes : "
                           + csv4.missing_headers(mandatory_headers));
            return basic_production_date(beginning_date);
        }
        int date_c = csv4.get_pos_col("date");
        service_c = csv4.get_pos_col("service_id");
        while(!csv4.eof()) {
            auto row = csv4.next();
            if(!row.empty() && services.find(row[service_c]) != services.end()) {
                boost::gregorian::date current_date = boost::gregorian::from_undelimited_string(row[date_c]);
                if(current_date < start_date){
                    start_date = current_date;
                }
                if(current_date > end_date){
                    end_date = current_date;
                }
            }
        }

    }


    boost::gregorian::date b_date(boost::gregorian::min_date_time);
    if(beginning_date != "")
        b_date = boost::gregorian::from_undelimited_string(beginning_date);
    LOG4CPLUS_TRACE(logger, "date de production: " +
                    boost::gregorian::to_simple_string((start_date>b_date ? start_date : b_date))
                    + " - " + boost::gregorian::to_simple_string(end_date));
    return boost::gregorian::date_period((start_date>b_date ? start_date : b_date), end_date);
}

void GtfsParser::parse_files(Data& data) {
    fill_default_modes(data);
    fill_default_company(data);

    parse<AgencyGtfsHandler>(data, "agency.txt", true);
    parse<DefaultContributorHandler>(data);
    parse<StopsGtfsHandler>(data, "stops.txt", true);
    parse<RouteGtfsHandler>(data, "routes.txt", true);
    parse<TransfersGtfsHandler>(data, "transfers.txt");
    parse<CalendarGtfsHandler>(data, "calendar.txt");
    parse<CalendarDatesGtfsHandler>(data, "calendar_dates.txt");
    parse<TripsGtfsHandler>(data, "trips.txt", true);
    parse<StopTimeGtfsHandler>(data, "stop_times.txt", true);
    parse<FrequenciesGtfsHandler>(data, "frequencies.txt");
}
}}
