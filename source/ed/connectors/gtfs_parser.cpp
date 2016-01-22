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
#include "utils/encoding_converter.h"
#include "utils/csv.h"
#include "utils/logger.h"
#include <boost/geometry.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/range/algorithm/sort.hpp>
#include <boost/tokenizer.hpp>
#include <fstream>
#include <iostream>
#include <set>

namespace nm = ed::types;
typedef boost::tokenizer< boost::escaped_list_separator<char> > Tokenizer;

namespace ed{ namespace connectors {

nt::Type_e get_type_enum(const std::string& str) {
    if ("network" == str) {
        return nt::Type_e::Network;
    }
    if ("stop_area" == str) {
        return nt::Type_e::StopArea;
    }
    if ("stop_point" == str) {
        return nt::Type_e::StopPoint;
    }
    if ("line" == str) {
        return nt::Type_e::Line;
    }
    if ("route" == str) {
        return nt::Type_e::Route;
    }
    if ("trip" == str) {
        return nt::Type_e::VehicleJourney;
    }
    if ("stop_time" == str) {
        return nt::Type_e::StopTime;
    }
    LOG4CPLUS_WARN(log4cplus::Logger::getInstance("log"), "unkown navitia type: " << str);
    return nt::Type_e::Unknown;
}

nt::RTLevel get_rtlevel_enum(const std::string& str){
    if (str == "1"){
        return nt::RTLevel::Adapted;
    }
    if (str == "2"){
        return nt::RTLevel::RealTime;
    }
    return nt::RTLevel::Base;
}

/**
 * add a comment.
 * create an id for the comment if needed since the comments are handled by ID in fusio2ed
 */
template <typename T>
void add_gtfs_comment(GtfsData& gtfs_data, Data& data, const T& obj, const std::string& comment) {
    std::string& comment_id = gtfs_data.comments_id_map[comment];
    if (comment_id.empty()) {
        comment_id = "comment__" + boost::lexical_cast<std::string>(gtfs_data.comments_id_map.size());
    }
    data.add_pt_object_comment(obj, comment_id);

    data.comment_by_id[comment_id] = comment;
}

static int default_waiting_duration = 120;
static int default_connection_duration = 120;

std::string generate_unique_vj_uri(const GtfsData& gtfs_data, const std::string original_uri, int cpt_vj) {
    // we change the name of the vj since we had to split the original GTFS vj because of dst
    // WARNING: this code is uggly, but it's a quick fix.
    for (int i = 0; i < 100; ++i) {
        //for debugging purpose (since vj uri are useful only for dev purpose)
        //we store if the vj is in conflict
        const std::string separator = (i == 0 ? "dst" : "conflit");
        const std::string vj_uri = original_uri + "_" + separator + "_" + std::to_string(cpt_vj + i);
        //to avoid collision, we check if we find a vj with the name we want to create
        if (gtfs_data.vj_uri.find(vj_uri) == gtfs_data.vj_uri.end()) {
            return vj_uri;
        }
    }
    // we haven't found a unique uri...
    // If this case happens, we need to handle this differently.
    // read all vj beforehand to know how to avoid conflict ?
    throw navitia::exception("impossible to generate a unique uri for the vj " + original_uri +
                             "there are some problems with the dataset. The current code cannot handle that");
}

std::pair<std::string, boost::local_time::time_zone_ptr> TzHandler::get_tz(const std::string& tz_name) {
    if (! tz_name.empty()) {
        auto tz = tz_db.time_zone_from_region(tz_name);
        if (tz) {
            return {tz_name, tz};
        }
        LOG4CPLUS_WARN(log4cplus::Logger::getInstance("ed"), "cannot find " << tz_name << " in tz db");
    }
    //we fetch the default dataset timezone
    return default_timezone;
}


/*
 * we need the list of dst periods over the years of the validity period
 *
 *                                  validity period
 *                              [-----------------------]
 *                        2013                                  2014
 *       <------------------------------------><-------------------------------------->
 *[           non DST   )[  DST    )[        non DST     )[   DST     )[     non DST          )
 *
 * We thus create a partition of the time with all period with the UTC offset
 *
 *       [    +7h       )[  +8h    )[       +7h          )[   +8h     )[      +7h     )
 */
std::vector<PeriodWithUtcShift> get_dst_periods(const boost::gregorian::date_period& validity_period,
                                                const boost::local_time::time_zone_ptr& tz) {

    if (validity_period.is_null()) {
        return {};
    }
    std::vector<int> years;
    //we want to have all the overlapping year
    //so add each time 1 year and continue till the first day of the year is after the end of the period (cf gtfs_parser_test.boost_periods)
    for (boost::gregorian::year_iterator y_it(validity_period.begin()); boost::gregorian::date((*y_it).year(), 1, 1) < validity_period.end(); ++y_it) {
        years.push_back((*y_it).year());
    }

    BOOST_ASSERT(! years.empty());

    std::vector<PeriodWithUtcShift> res;
    for (int year: years) {
        if (! res.empty()) {
            //if res is not empty we add the additional period without the dst
            //from the previous end date to the beggining of the dst next year
            res.push_back({ {res.back().period.end(), tz->dst_local_start_time(year).date()},
                            tz->base_utc_offset() });
        } else {
            //for the first elt, we add a non dst
            auto first_day_of_year = boost::gregorian::date(year, 1, 1);
            if (tz->dst_local_start_time(year).date() != first_day_of_year) {
                res.push_back({ {first_day_of_year, tz->dst_local_start_time(year).date()},
                                tz->base_utc_offset() });
            }
        }
        res.push_back({ {tz->dst_local_start_time(year).date(), tz->dst_local_end_time(year).date()},
                        tz->base_utc_offset() + tz->dst_offset() });
    }
    //we add the last non DST period
    res.push_back({ {res.back().period.end(), boost::gregorian::date(years.back() + 1, 1, 1)},
                    tz->base_utc_offset() });

    //we want the shift in seconds, and it is in minute in the tzdb
    for (auto& p_shift: res) {
        p_shift.utc_shift *= 60;
    }
    return res;
}

nt::TimeZoneHandler::dst_periods
split_over_dst(const boost::gregorian::date_period& validity_period, const boost::local_time::time_zone_ptr& tz) {
    nt::TimeZoneHandler::dst_periods res;

    if (! tz) {
        LOG4CPLUS_FATAL(log4cplus::Logger::getInstance("log"), "no timezone available, cannot compute dst split");
        return res;
    }

    boost::posix_time::time_duration utc_offset = tz->base_utc_offset();

    if (! tz->has_dst()) {
        //no dst -> easy way out, no split, we just have to take the utc offset into account
        res[utc_offset.total_seconds() / 60].push_back(validity_period);
        return res;
    }

    std::vector<PeriodWithUtcShift> dst_periods = get_dst_periods(validity_period, tz);

    //we now compute all intersection between periods
    //to use again the example of get_dst_periods:
    //                                      validity period
    //                                  [----------------------------]
    //                            2013                                  2014
    //           <------------------------------------><-------------------------------------->
    //    [           non DST   )[  DST    )[        non DST     )[   DST     )[     non DST          )
    //
    // we create the periods:
    //
    //                                  [+8)[       +7h          )[+8h)
    //
    // all period_with_utc_shift are grouped by dst offsets.
    // ie in the previous example there are 2 period_with_utc_shift:
    //                        1/        [+8)                      [+8h)
    //                        2/            [       +7h          )
    for (const auto& dst_period: dst_periods) {

        if (! validity_period.intersects(dst_period.period)) {
            //no intersection, we don't consider it
            continue;
        }
        auto intersec = validity_period.intersection(dst_period.period);

        res[dst_period.utc_shift].push_back(intersec);
    }

    return res;
}


ed::types::Network* GtfsData::get_or_create_default_network(ed::Data& data) {
    if (! default_network) {
        LOG4CPLUS_INFO(log4cplus::Logger::getInstance("log"), "creating default network");
        default_network = new ed::types::Network();
        default_network->uri = "default_network";
        default_network->name = "default network";
        data.networks.push_back(default_network);
        network_map[default_network->uri] = default_network;

        //with the default agency comes the default timezone (only if none was provided before)
        if (tz.default_timezone.first.empty()) {
            LOG4CPLUS_INFO(log4cplus::Logger::getInstance("log"), "no time zone defined, we create a default one for paris");
            const std::string default_tz = UTC_TIMEZONE;//"Europe/Paris";
            auto timezone = tz.tz_db.time_zone_from_region(default_tz);
            BOOST_ASSERT(timezone);
            tz.default_timezone = {default_tz, timezone};
        }
    }
    return default_network;
}


ed::types::Company* GtfsData::get_or_create_default_company(Data & data) {
    if (! default_company) {
        // default compagny creation
        default_company = new ed::types::Company();
        default_company->uri = "default_company";
        default_company->name = "compagnie par défaut";
        data.companies.push_back(default_company);
        company_map[default_company->uri] = default_company;
    }
    return default_company;
}


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

void FeedInfoGtfsHandler::init(Data&) {
    feed_publisher_name_c = csv.get_pos_col("feed_publisher_name");
    feed_publisher_url_c = csv.get_pos_col("feed_publisher_url");
    feed_start_date_c = csv.get_pos_col("feed_start_date");
    feed_end_date_c = csv.get_pos_col("feed_end_date");
}

void FeedInfoGtfsHandler::handle_line(Data& data, const csv_row& row, bool) {
    if(has_col(feed_end_date_c, row)) {
        data.add_feed_info("feed_end_date", row[feed_end_date_c]);
    }

    if(has_col(feed_start_date_c, row)) {
        data.add_feed_info("feed_start_date", row[feed_start_date_c]);
    }

    if(has_col(feed_publisher_name_c, row)) {
        data.add_feed_info("feed_publisher_name", row[feed_publisher_name_c]);
    }

    if(has_col(feed_publisher_url_c, row)) {
        data.add_feed_info("feed_publisher_url", row[feed_publisher_url_c]);
    }
}

void AgencyGtfsHandler::init(Data&) {
    id_c = csv.get_pos_col("agency_id");
    name_c = csv.get_pos_col("agency_name");
    time_zone_c = csv.get_pos_col("agency_timezone");
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
        network->uri = "default_network";
        gtfs_data.default_network = network;
    }

    network->name = row[name_c];
    data.networks.push_back(network);

    gtfs_data.network_map[network->uri] = network;

    std::string timezone_name = row[time_zone_c];

    if (gtfs_data.tz.default_timezone.second) {
        if (gtfs_data.tz.default_timezone.first != timezone_name) {
            LOG4CPLUS_WARN(logger, "Error while reading "<< csv.filename <<
                            " all the time zone are not equals, only the first one will be considered as the default timezone");
        }
        return network;
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
            wheelchair_c = csv.get_pos_col("wheelchair_boarding"),
            platform_c = csv.get_pos_col("platform_code"),
            timezone_c = csv.get_pos_col("stop_timezone");
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

    //We fetch the accessibility for all stop points that inherit from their stop area's accessibility
    for (auto sp : wheelchair_heritance) {
        if(sp->stop_area == nullptr)
            continue;
        if (sp->stop_area->property(navitia::type::hasProperties::WHEELCHAIR_BOARDING)) {
            sp->set_property(navitia::type::hasProperties::WHEELCHAIR_BOARDING);
        }
    }

    handle_stop_point_without_area(data);

    LOG4CPLUS_TRACE(logger, data.stop_points.size() << " added stop points");;
    LOG4CPLUS_TRACE(logger, data.stop_areas.size() << " added stop areas");
    LOG4CPLUS_TRACE(logger, ignored << " points ignored because of dupplicates" );
}

void StopsGtfsHandler::handle_stop_point_without_area(Data& data) {
    int nb_added_sa(0);
    //we artificialy create one stop_area for stop point without one
    for (const auto sp : data.stop_points) {
        if (sp->stop_area) {
            continue;
        }
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

        //if the stoppoint had a tz, it becomes the stop area's, else we fetch the default timezone
        auto it_tz = gtfs_data.tz.stop_point_tz.find(sp);
        if (it_tz != gtfs_data.tz.stop_point_tz.end()) {
            sa->time_zone_with_name = gtfs_data.tz.get_tz(it_tz->second);
        } else {
            //we fetch the defautl dataset timezone
            sa->time_zone_with_name = gtfs_data.tz.default_timezone;
        }
        nb_added_sa ++;
    }

    if (nb_added_sa) {
        LOG4CPLUS_INFO(logger, nb_added_sa << " stop_points without stop_area. Creation of a stop_area for each of those stop_points");
    }
}

template <typename T>
bool StopsGtfsHandler::parse_common_data(const csv_row& row, T* stop) {
    try{
        stop->coord.set_lon(boost::lexical_cast<double>(row[lon_c]));
        stop->coord.set_lat(boost::lexical_cast<double>(row[lat_c]));
    }
    catch(boost::bad_lexical_cast) {
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

        if (has_col(timezone_c, row)) {
            auto tz_name = row[timezone_c];
            sa->time_zone_with_name = gtfs_data.tz.get_tz(tz_name);
        } else {
            sa->time_zone_with_name = gtfs_data.tz.default_timezone;
        }
        if (has_col(desc_c, row)) {
            add_gtfs_comment(gtfs_data, data, sa, row[desc_c]);
        }
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
        if (has_col(platform_c, row))
            sp->platform_code = row[platform_c];
        gtfs_data.stop_map[sp->uri] = sp;
        data.stop_points.push_back(sp);
        if (has_col(parent_c, row) && row[parent_c] != "") {// we save the reference to the stop area
            auto it = gtfs_data.sa_spmap.find(row[parent_c]);
            if (it == gtfs_data.sa_spmap.end()) {
                it = gtfs_data.sa_spmap.insert(std::make_pair(row[parent_c], GtfsData::vector_sp())).first;
            }
            it->second.push_back(sp);
        }

        if (has_col(desc_c, row)) {
            add_gtfs_comment(gtfs_data, data, sp, row[desc_c]);
        }

        //we save the tz in case the stop point is later promoted to stop area
        if (has_col(timezone_c, row) && ! row[timezone_c].empty()) {
            gtfs_data.tz.stop_point_tz[sp] = row[timezone_c];
        }
        return_wrapper.first = sp;
    }
    return return_wrapper;
}

void RouteGtfsHandler::init(Data&) {
    id_c = csv.get_pos_col("route_id"), short_name_c = csv.get_pos_col("route_short_name"),
            long_name_c = csv.get_pos_col("route_long_name"), type_c = csv.get_pos_col("route_type"),
            desc_c = csv.get_pos_col("route_desc"),
            color_c = csv.get_pos_col("route_color"), agency_c = csv.get_pos_col("agency_id"),
            text_color_c = csv.get_pos_col("route_text_color");
}

nm::Line* RouteGtfsHandler::handle_line(Data& data, const csv_row& row, bool) {
    if(gtfs_data.line_map.find(row[id_c]) != gtfs_data.line_map.end()) {
        ignored++;
        LOG4CPLUS_WARN(logger, "dupplicate on route line " + row[id_c]);
        return nullptr;
    }

    nm::Line* line = new nm::Line();
    line->uri = row[id_c];
    line->name = row[long_name_c];
    line->code = row[short_name_c];
    if (has_col(desc_c, row)) {
        add_gtfs_comment(gtfs_data, data, line, row[desc_c]);
    }

    if(has_col(color_c, row)) {
        line->color = row[color_c];
    }
    if(is_valid(text_color_c, row)) {
        line->text_color = row[text_color_c];
    }
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

    if(is_valid(agency_c, row)) {
        auto network_it = gtfs_data.network_map.find(row[agency_c]);
        if(network_it != gtfs_data.network_map.end()) {
            line->network = network_it->second;
        }
        else {
            throw navitia::exception("line " + line->uri + " has an unknown network: " + row[agency_c] + ", the dataset is not valid");
        }
    }
    else {
        //if the line has no network, we get_or_create the default one
        line->network = gtfs_data.get_or_create_default_network(data);
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
            //the GTFS transfers duration already take into account a tolerance so duration and display duration are equal
            connection->display_duration = boost::lexical_cast<int>(row[time_c]);
            connection->duration = connection->display_duration;
        } catch (const boost::bad_lexical_cast&) {
            //if no transfers time is given, we add an additional waiting time to the real duration
            //ie you want to walk for 2 mn for your transfert, and we add 2 more minutes to add robustness to your transfers
            connection->display_duration = default_connection_duration;
            connection->duration = connection->display_duration + default_waiting_duration;
        }
    } else {
        connection->display_duration = default_connection_duration;
        connection->duration = connection->display_duration + default_waiting_duration;
    }
}

void TransfersGtfsHandler::handle_line(Data& data, const csv_row& row, bool) {
    GtfsData::vector_sp departures, arrivals;
    // We don't want to have these connections, because it could lead to weird things
    // And also we don't want to show that to our users, this must be a data problem
    if (row[time_c].empty()) {
        ++data.count_empty_connections;
        return;
    }
    try {
        if (boost::lexical_cast<int>(row[time_c]) > 24*3600) {
            ++data.count_too_long_connections;
            return;
        }
    } catch (boost::bad_lexical_cast&) {
        LOG4CPLUS_WARN(logger, "Invalid connection time: " + row[time_c]);
        return;
    }
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
    if(it == gtfs_data.stop_map.end()) {
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
    LOG4CPLUS_TRACE(logger, "Nb validity patterns : " << data.validity_patterns.size() << " nb lines : " << nb_lines);
    BOOST_ASSERT(data.validity_patterns.size() == gtfs_data.tz.vp_by_name.size());
}

void CalendarGtfsHandler::handle_line(Data& data, const csv_row& row, bool) {
    if (row.empty())
        return;
    std::bitset<7> week;
    nb_lines ++;

    if (gtfs_data.tz.non_split_vp.find(row[id_c]) != gtfs_data.tz.non_split_vp.end()) {
        return;
    }
    //we only build temporary validity pattern that will be split after the CalendarDateParser has been called
    nt::ValidityPattern& vp = gtfs_data.tz.non_split_vp[row[id_c]];
    vp.beginning_date = data.meta.production_date.begin();
    vp.uri = row[id_c];

    //week day init (remember that sunday is the first day of week in the us)
    week[1] = (row[monday_c] == "1");
    week[2] = (row[tuesday_c] == "1");
    week[3] = (row[wednesday_c] == "1");
    week[4] = (row[thursday_c] == "1");
    week[5] = (row[friday_c] == "1");
    week[6] = (row[saturday_c] == "1");
    week[0] = (row[sunday_c] == "1");

    //Init the validity period
    boost::gregorian::date b_date = boost::gregorian::from_undelimited_string(row[start_date_c]);

    //we add one day to the last day of the period because end_date_c is included in the period
    //and as say the constructor of boost periods, the second args is "end" so it is not included
    boost::gregorian::date e_date = boost::gregorian::from_undelimited_string(row[end_date_c]) + 1_days;

    boost::gregorian::date_period full_period (b_date, e_date);
    auto period = full_period.intersection(data.meta.production_date);

    for (boost::gregorian::day_iterator it(period.begin()); it<period.end(); ++it) {
        if (week.test((*it).day_of_week())) {
            vp.add((*it));
        } else {
            vp.remove((*it));
        }
    }
}

static boost::gregorian::date_period compute_smallest_active_period(const nt::ValidityPattern& vp) {
    size_t beg(std::numeric_limits<size_t>::max()), end(std::numeric_limits<size_t>::min());

    for (size_t i = 0; i < vp.days.size(); ++i) {
        if (vp.check(i)) {
            if (i < beg) {
                beg = i;
            }
            if (i > end) {
                end = i;
            }
        }
    }

    if (beg == std::numeric_limits<size_t>::max()) {
        assert(end == std::numeric_limits<size_t>::min()); //if we did not find beg, end cannot be found too
        LOG4CPLUS_INFO(log4cplus::Logger::getInstance("log"), "the calendar " << vp.uri
                       << " has an empty validity period, we will ignore it");
        return boost::gregorian::date_period(vp.beginning_date, vp.beginning_date); //return null period
    }

    return boost::gregorian::date_period(vp.beginning_date + boost::gregorian::days(beg),
                                         vp.beginning_date + boost::gregorian::days(end + 1));
}

/*
 * Since all time have to be converted to UTC, we need to handle day saving time (DST) rules
 * we thus need to split all periods for them to be on only one DST
 * luckily in GTFS format all times are given in the same timezone (the default tz dataset) even if all stop areas are not on the same tz
 */
void split_validity_pattern_over_dst(Data& data, GtfsData& gtfs_data) {
    // we start by filling the global tz_handler
    auto splited_production_period = split_over_dst(data.meta.production_date,
                                                    gtfs_data.tz.default_timezone.second);
    data.tz_handler = nt::TimeZoneHandler(gtfs_data.tz.default_timezone.first,
                                          data.meta.production_date.begin(),
                                          splited_production_period);

    for (const auto& name_and_vp: gtfs_data.tz.non_split_vp) {
        const nt::ValidityPattern& original_vp = name_and_vp.second;

        boost::gregorian::date_period smallest_active_period = compute_smallest_active_period(original_vp);

        auto split_periods = split_over_dst(smallest_active_period, gtfs_data.tz.default_timezone.second);

        BOOST_ASSERT(! split_periods.empty() || smallest_active_period.is_null()); //by construction it cannot be empty if the validity period is not null

        size_t cpt(1);
        for (const auto& utc_shit_and_periods: split_periods) {
            nt::ValidityPattern* vp = new nt::ValidityPattern(data.meta.production_date.begin());

            for (const auto& period: utc_shit_and_periods.second) {
                for (boost::gregorian::day_iterator it(period.begin()); it < period.end(); ++it) {
                    if (original_vp.check(*it)) {
                        vp->add(*it);
                    }
                }
            }

            if (split_periods.size() == 1) {
                //we do not change the id if the period is not split
                vp->uri = original_vp.uri;
            } else {
                vp->uri = original_vp.uri + "_" + std::to_string(cpt);
            }
            gtfs_data.tz.vp_by_name.insert({original_vp.uri, vp});
            data.validity_patterns.push_back(vp);
            cpt++;
        }
    }

    LOG4CPLUS_TRACE(log4cplus::Logger::getInstance("log"), "Nb validity patterns : " << data.validity_patterns.size());
    BOOST_ASSERT(data.validity_patterns.size() == gtfs_data.tz.vp_by_name.size());
    if (data.validity_patterns.empty()) {
        LOG4CPLUS_FATAL(log4cplus::Logger::getInstance("log"), "No validity_patterns");
    }

}

void CalendarDatesGtfsHandler::init(Data&) {
    id_c = csv.get_pos_col("service_id"), date_c = csv.get_pos_col("date"),
            e_type_c = csv.get_pos_col("exception_type");
}

void CalendarDatesGtfsHandler::handle_line(Data& data, const csv_row& row, bool) {
    if(row.empty())
        return;

    bool new_vp = gtfs_data.tz.non_split_vp.find(row[id_c]) == gtfs_data.tz.non_split_vp.end();

    nt::ValidityPattern& vp = gtfs_data.tz.non_split_vp[row[id_c]];

    if (new_vp) {
        vp.beginning_date = data.meta.production_date.begin();
        vp.uri = row[id_c];
    }

    auto date = boost::gregorian::from_undelimited_string(row[date_c]);
    if(row[e_type_c] == "1")
        vp.add(date);
    else if(row[e_type_c] == "2")
        vp.remove(date);
    else
        LOG4CPLUS_WARN(logger, "Exception pour le service " << row[id_c] << " inconnue : " << row[e_type_c]);
}

void ShapesGtfsHandler::init(Data&) {
    shape_id_c = csv.get_pos_col("shape_id");
    shape_pt_lat_c = csv.get_pos_col("shape_pt_lat");
    shape_pt_lon_c = csv.get_pos_col("shape_pt_lon");
    shape_pt_sequence_c = csv.get_pos_col("shape_pt_sequence");
}

void ShapesGtfsHandler::handle_line(Data&, const csv_row& row, bool) {
    try {
        const navitia::type::GeographicalCoord coord(
            boost::lexical_cast<double>(row.at(shape_pt_lon_c)),
            boost::lexical_cast<double>(row.at(shape_pt_lat_c)));
        const int seq = boost::lexical_cast<int>(row.at(shape_pt_sequence_c));
        shapes[row.at(shape_id_c)][seq] = coord;
    } catch (std::exception &e) {
        LOG4CPLUS_WARN(logger, "Exception in shapes.txt: " << e.what());
    }
}

void ShapesGtfsHandler::finish(Data& data) {
    LOG4CPLUS_INFO(logger, "Nb shapes: " << shapes.size());
    for (const auto& shape: shapes) {
        navitia::type::LineString line;
        for (const auto& coord: shape.second) {
            line.push_back(coord.second);
        }
        data.shapes[shape.first] = {line};
    }
}

void TripsGtfsHandler::init(Data& data) {
    data.vehicle_journeys.reserve(350000);

    id_c = csv.get_pos_col("route_id");
    service_c = csv.get_pos_col("service_id");
    trip_c = csv.get_pos_col("trip_id");
    headsign_c = csv.get_pos_col("trip_headsign");
    block_id_c = csv.get_pos_col("block_id");
    wheelchair_c = csv.get_pos_col("wheelchair_accessible");
    bikes_c = csv.get_pos_col("bikes_allowed");
    shape_id_c = csv.get_pos_col("shape_id");
    direction_id_c = csv.get_pos_col("direction_id");
}

void TripsGtfsHandler::finish(Data& data) {
    BOOST_ASSERT(data.vehicle_journeys.size() == gtfs_data.tz.vj_by_name.size());
    LOG4CPLUS_TRACE(logger, "Nb vehicle journeys : " << data.vehicle_journeys.size());
    LOG4CPLUS_TRACE(logger, "Nb errors on service reference " << ignored);
    LOG4CPLUS_TRACE(logger, ignored_vj << " duplicated vehicule journey have been ignored");
}

/*
 * We create one route by line and direction_id
 * the direction_id field is usually a boolean, but can be used as a string
 * to create as many routes as wanted
 */
types::Route* TripsGtfsHandler::get_or_create_route(Data& data, const RouteId& route_id) {
    const auto it = routes.find(route_id);
    if (it != std::end(routes)) {
        return it->second;
    } else {
        types::Route* route = new types::Route();
        route->line = route_id.first;
        //uri is {line}:{direction}
        route->uri = route->line->uri + ":" + route_id.second;
        route->name = route->line->name;
        route->idx = data.routes.size();
        data.routes.push_back(route);
        routes[route_id] = route;
        return route;
    }
}

void TripsGtfsHandler::handle_line(Data& data, const csv_row& row, bool) {
    auto it = gtfs_data.line_map.find(row[id_c]);
    if (it == gtfs_data.line_map.end()) {
        LOG4CPLUS_WARN(logger, "Impossible to find the Gtfs line " << row[id_c]
                       << " referenced by trip " << row[trip_c]);
        ignored++;
        return;
    }

    nm::Line* line = it->second;

    nm::Route* route = get_or_create_route(data, {line, row[direction_id_c]});

    auto vp_range = gtfs_data.tz.vp_by_name.equal_range(row[service_c]);
    if(empty(vp_range)) {
        LOG4CPLUS_WARN(logger, "Impossible to find the Gtfs service " + row[service_c]
                       + " referenced by trip " + row[trip_c]);
        ignored++;
        return;
    }

    //we look in the meta vj table to see if we already have one such vj
    if (data.meta_vj_map.find(row[trip_c]) != data.meta_vj_map.end()) {
        LOG4CPLUS_DEBUG(logger, "vj " << row[trip_c] << " already read, we skip the second one");
        ignored_vj++;
        return;
    }

    types::MetaVehicleJourney& meta_vj = data.meta_vj_map[row[trip_c]]; //we get a ref on a newly created meta vj
    meta_vj.uri = row[trip_c];

    // get shape if possible
    const std::string &shape_id = has_col(shape_id_c, row) ? row.at(shape_id_c) : "";

    bool has_been_split = more_than_one_elt(vp_range); //check if the trip has been split over dst

    size_t cpt_vj(1);
    //the validity pattern may have been split because of DST, so we need to create one vj for each
    for (auto vp_it = vp_range.first; vp_it != vp_range.second; ++vp_it, cpt_vj++) {

        nt::ValidityPattern* vp_xx = vp_it->second;

        nm::VehicleJourney* vj = new nm::VehicleJourney();
        const std::string original_uri = row[trip_c];
        std::string vj_uri = original_uri;
        if (has_been_split) {
            vj_uri = generate_unique_vj_uri(gtfs_data, original_uri, cpt_vj);
        }

        vj->uri = vj_uri;
        if(has_col(headsign_c, row))
            vj->name = row[headsign_c];
        else
            vj->name = vj->uri;

        vj->validity_pattern = vp_xx;
        vj->adapted_validity_pattern = vp_xx;
        vj->route = route;
        if(has_col(block_id_c, row))
            vj->block_id = row[block_id_c];
        else
            vj->block_id = "";
        if(has_col(wheelchair_c, row) && row[wheelchair_c] == "1")
            vj->set_vehicle(navitia::type::hasVehicleProperties::WHEELCHAIR_ACCESSIBLE);
        if(has_col(bikes_c, row) && row[bikes_c] == "1")
            vj->set_vehicle(navitia::type::hasVehicleProperties::BIKE_ACCEPTED);

        auto itm = gtfs_data.physical_mode_map.find(line->commercial_mode->uri);
        if (itm != gtfs_data.physical_mode_map.end()){
            vj->physical_mode = itm->second;
        }

        vj->company = gtfs_data.get_or_create_default_company(data);

        gtfs_data.tz.vj_by_name.insert({original_uri, vj});

        data.vehicle_journeys.push_back(vj);
        //we add them on our meta vj
        meta_vj.theoric_vj.push_back(vj);
        vj->meta_vj_name = row[trip_c];
        vj->shape_id = shape_id;
    }
}

void StopTimeGtfsHandler::init(Data&) {
    LOG4CPLUS_INFO(logger, "reading stop times");
    id_c = csv.get_pos_col("trip_id"),
            arrival_c = csv.get_pos_col("arrival_time"),
            departure_c = csv.get_pos_col("departure_time"),
            stop_c = csv.get_pos_col("stop_id"),
            stop_seq_c = csv.get_pos_col("stop_sequence"),
            pickup_c = csv.get_pos_col("pickup_type"),
            drop_off_c = csv.get_pos_col("drop_off_type");
}

void StopTimeGtfsHandler::finish(Data& data) {
    LOG4CPLUS_INFO(logger, "sorting stoptimes of vehicle_journeys");
    for (auto *const vj: data.vehicle_journeys) {
        if (vj->stop_time_list.empty()) continue;

        boost::sort(vj->stop_time_list, [](const nm::StopTime* st1, const nm::StopTime* st2)->bool{
                if(st1->order == st2->order){
                    throw navitia::exception("two stoptime with the same order (" +
                        std::to_string(st1->order) + ") for vj: " + st1->vehicle_journey->uri);
                }
                return st1->order < st2->order;
            });

        for (size_t it_st = 0; it_st < vj->stop_time_list.size(); ++it_st) {
            vj->stop_time_list[it_st]->order =  it_st;
        }

        if (vj->stop_time_list.size() < 2) {
            continue;
        }
        //we check that the vj, once ordered by order are correctly sorted on the departure times
        auto it_st = vj->stop_time_list.begin();
        for (auto it_next = it_st + 1; it_next != vj->stop_time_list.end(); it_next++, it_st ++) {
            const auto* st1 = *it_st;
            const auto* st2 = *it_next;
            if (! (st1->arrival_time <= st1->departure_time &&
                  st1->departure_time <= st2->arrival_time &&
                  st2->arrival_time <= st2->departure_time)) {
                LOG4CPLUS_INFO(logger, "invalid vj " << vj->uri << ", the stop times "
                                        "are not correcly ordered "
                               << "stop time " << st1->order << " [" << st1->arrival_time
                               << ", " << st1->departure_time <<"]"
                               << " stop time " << st2->order << " [" << st2->arrival_time
                               << ", " << st2->departure_time << "]"
                               << " we erase the VJ");
                data.vj_to_erase.insert(vj);
                break;
            }
        }
    }
    LOG4CPLUS_INFO(logger, "Nb stop times: " << data.stops.size());
}

static int to_utc(const std::string& local_time, int utc_offset) {
    return time_to_int(local_time) - utc_offset;
}

std::vector<nm::StopTime*> StopTimeGtfsHandler::handle_line(Data& data, const csv_row& row, bool) {
    auto stop_it = gtfs_data.stop_map.find(row[stop_c]);
    if(stop_it == gtfs_data.stop_map.end()) {
        LOG4CPLUS_WARN(logger, "Impossible to find the stop_point " + row[stop_c] + "!");
        return {};
    }

    auto vj_it = gtfs_data.tz.vj_by_name.lower_bound(row[id_c]);
    if(vj_it == gtfs_data.tz.vj_by_name.end()) {
        LOG4CPLUS_WARN(logger, "Impossible to find the vehicle_journey '" << row[id_c] << "'");
        return {};
    }
    std::vector<nm::StopTime*> stop_times;

    //the validity pattern may have been split because of DST, so we need to create one vj for each
    for (auto vj_end_it = gtfs_data.tz.vj_by_name.upper_bound(row[id_c]); vj_it != vj_end_it; ++vj_it) {

        nm::StopTime* stop_time = new nm::StopTime();

        //we need to convert the stop times in UTC
        int utc_offset = data.tz_handler.get_first_utc_offset(*vj_it->second->validity_pattern);

        stop_time->arrival_time = to_utc(row[arrival_c], utc_offset);
        stop_time->departure_time = to_utc(row[departure_c], utc_offset);

        stop_time->stop_point = stop_it->second;
        stop_time->order = boost::lexical_cast<unsigned int>(row[stop_seq_c]);
        stop_time->vehicle_journey = vj_it->second;

        if(has_col(pickup_c, row) && has_col(drop_off_c, row))
            stop_time->ODT = (row[pickup_c] == "2" || row[drop_off_c] == "2");
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
        stop_time->idx = data.stops.size();
        data.stops.push_back(stop_time);
        count++;
        stop_times.push_back(stop_time);
    }
    return stop_times;
}

void FrequenciesGtfsHandler::init(Data&) {
    trip_id_c = csv.get_pos_col("trip_id"), start_time_c = csv.get_pos_col("start_time"),
    end_time_c = csv.get_pos_col("end_time"), headway_secs_c = csv.get_pos_col("headway_secs");
}

void FrequenciesGtfsHandler::handle_line(Data& data, const csv_row& row, bool) {
    if(row.empty())
        return;
    for (auto vj_end_it = gtfs_data.tz.vj_by_name.upper_bound(row[trip_id_c]),
         vj_it = gtfs_data.tz.vj_by_name.lower_bound(row[trip_id_c]);
         vj_it != vj_end_it; ++vj_it) {

        if (vj_it->second->stop_time_list.empty()) {
             LOG4CPLUS_WARN(logger, "vj " << row[trip_id_c] << " is empty cannot add stoptimes in frequencies");
             continue;
        }

        //we need to convert the stop times in UTC
        int utc_offset = data.tz_handler.get_first_utc_offset(*vj_it->second->validity_pattern);

        vj_it->second->start_time = to_utc(row[start_time_c], utc_offset);
        vj_it->second->end_time = to_utc(row[end_time_c], utc_offset);
        vj_it->second->headway_secs = boost::lexical_cast<int>(row[headway_secs_c]);
        int first_st_gap = vj_it->second->start_time - vj_it->second->stop_time_list.front()->arrival_time;
        for(auto st: vj_it->second->stop_time_list) {
            st->is_frequency = true;
            st->arrival_time += first_st_gap;
            st->departure_time += first_st_gap;
        }
    }
}

GenericGtfsParser::GenericGtfsParser(const std::string & path) : path(path){
    logger = log4cplus::Logger::getInstance("log");
}
GenericGtfsParser::~GenericGtfsParser() {}

void GenericGtfsParser::fill(Data& data, const std::string& beginning_date) {

    parse_files(data, beginning_date);

    normalize_extcodes(data);
}

void GenericGtfsParser::fill_default_modes(Data& data){

    // default commercial_mode and physical_modes
    // all modes are represented by a number in GTFS
    // see route_type in https://developers.google.com/transit/gtfs/reference?hl=fr-FR#routestxt
    ed::types::CommercialMode* commercial_mode = new ed::types::CommercialMode();
    commercial_mode->name = "Tram";
    commercial_mode->uri = commercial_mode->name;
    data.commercial_modes.push_back(commercial_mode);
    gtfs_data.commercial_mode_map["0"] = commercial_mode;

    commercial_mode = new ed::types::CommercialMode();
    commercial_mode->name = "Metro";
    commercial_mode->uri = commercial_mode->name;
    data.commercial_modes.push_back(commercial_mode);
    gtfs_data.commercial_mode_map["1"] = commercial_mode;

    commercial_mode = new ed::types::CommercialMode();
    commercial_mode->name = "Rail";
    commercial_mode->uri = commercial_mode->name;
    data.commercial_modes.push_back(commercial_mode);
    gtfs_data.commercial_mode_map["2"] = commercial_mode;

    commercial_mode = new ed::types::CommercialMode();
    commercial_mode->name = "Bus";
    commercial_mode->uri = commercial_mode->name;
    data.commercial_modes.push_back(commercial_mode);
    gtfs_data.commercial_mode_map["3"] = commercial_mode;

    commercial_mode = new ed::types::CommercialMode();
    commercial_mode->name = "Ferry";
    commercial_mode->uri = commercial_mode->name;
    data.commercial_modes.push_back(commercial_mode);
    gtfs_data.commercial_mode_map["4"] = commercial_mode;

    commercial_mode = new ed::types::CommercialMode();
    commercial_mode->name = "Cable car";
    commercial_mode->uri = "Cable_car";
    data.commercial_modes.push_back(commercial_mode);
    gtfs_data.commercial_mode_map["5"] = commercial_mode;

    commercial_mode = new ed::types::CommercialMode();
    commercial_mode->name = "Gondola";
    commercial_mode->uri = commercial_mode->name;
    data.commercial_modes.push_back(commercial_mode);
    gtfs_data.commercial_mode_map["6"] = commercial_mode;

    commercial_mode = new ed::types::CommercialMode();
    commercial_mode->name = "Funicular";
    commercial_mode->uri = commercial_mode->name;
    data.commercial_modes.push_back(commercial_mode);
    gtfs_data.commercial_mode_map["7"] = commercial_mode;

    for (ed::types::CommercialMode* mt : data.commercial_modes) {
        // we aggregate some GTFS modes
        if (in(mt->uri, {"Cable_car", "Gondola"})) {
            continue;
        }

        ed::types::PhysicalMode* mode = new ed::types::PhysicalMode();
        mode->name = mt->name;
        mode->uri = mt->uri;
        data.physical_modes.push_back(mode);
        //NOTE: physical mode don't need to be indexed by the GTFS code, since they don't exist in GTFS
        gtfs_data.physical_mode_map[mode->uri] = mode;
    }
    //for physical mode, cable car is tramway, gondola is funicular
    gtfs_data.physical_mode_map["Cable_car"] = gtfs_data.physical_mode_map.at("Tram");
    gtfs_data.physical_mode_map["Gondola"] = gtfs_data.physical_mode_map.at("Funicular");
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
                e_date(b_date + boost::gregorian::days(365 + 1));

        return boost::gregorian::date_period(b_date, e_date);
    }
}


boost::gregorian::date_period GenericGtfsParser::find_production_date(const std::string& beginning_date) {
    std::string filename = path + "/stop_times.txt";
    CsvReader csv(filename, ',' , true);
    if(!csv.is_open()) {
        LOG4CPLUS_FATAL(logger, "Impossible to find file " + filename);
        return basic_production_date(beginning_date);
    }

    std::vector<std::string> mandatory_headers = {"trip_id" , "arrival_time",
                                                  "departure_time", "stop_id", "stop_sequence"};
    if(!csv.validate(mandatory_headers)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " << filename << ". Columns missing: " 
                << csv.missing_headers(mandatory_headers));
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
        LOG4CPLUS_FATAL(logger, "Impossible to find file " << filename);
        return basic_production_date(beginning_date);
    }

    mandatory_headers = {"trip_id" , "service_id"};
    if(!csv2.validate(mandatory_headers)) {
        LOG4CPLUS_FATAL(logger, "Error while reading " << filename << ". Columns missing: "
                << csv.missing_headers(mandatory_headers));

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
        LOG4CPLUS_WARN(logger, "impossible to find file " << filename);
    } else {
        calendar_txt_exists = true;
        mandatory_headers = {"start_date" , "end_date", "service_id"};
        if(!csv3.validate(mandatory_headers)) {
            LOG4CPLUS_FATAL(logger, "Error while reading " << filename << ". Columns missing: " 
                    << csv.missing_headers(mandatory_headers));
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
            LOG4CPLUS_WARN(logger, "impossible to find file " << filename);
        else
            LOG4CPLUS_WARN(logger, "impossible to find file " << filename << " and calendar.txt");
    } else {
        mandatory_headers = {"service_id" , "date", "exception_type"};
        if(!csv4.validate(mandatory_headers)) {
            LOG4CPLUS_FATAL(logger, "Error while reading " << filename << ". Columns missing: " 
                    << csv.missing_headers(mandatory_headers));
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
    return complete_production_date(beginning_date, start_date, end_date);
}

void GenericGtfsParser::manage_production_date(Data& data, const std::string& beginning_date){
    auto start_it = data.feed_infos.find("feed_start_date");
    auto end_it = data.feed_infos.find("feed_end_date");
    boost::gregorian::date start_date(boost::gregorian::not_a_date_time),
            end_date(boost::gregorian::not_a_date_time);
    if ((start_it != data.feed_infos.end() && start_it->second != "")
            && (end_it != data.feed_infos.end() && end_it->second != "")) {
        try{
            start_date = boost::gregorian::from_undelimited_string(start_it->second);
        }catch(const std::exception& e) {
           LOG4CPLUS_WARN(logger, "manage_production_date, Unable to parse start_date :"
                          << start_it->second << ", error : " << e.what());
        }
        try{
            end_date = boost::gregorian::from_undelimited_string(end_it->second);
        }catch(const std::exception& e) {
           LOG4CPLUS_WARN(logger, "manage_production_date, Unable to parse end_date :"
                          << end_it->second << ", error : " << e.what());
        }
    } else {
        LOG4CPLUS_INFO(logger, "Unable to find production date in add_feed_info.");
    }
    if ((start_date.is_not_a_date()) || (end_date.is_not_a_date())){
        try {
            data.meta.production_date = find_production_date(beginning_date);
        } catch (...) {
            if(beginning_date == "") {
                LOG4CPLUS_FATAL(logger, "Impossible to find the production date");
            }
        }
    } else {
        data.meta.production_date = complete_production_date(beginning_date, start_date, end_date);
    }
}

boost::gregorian::date_period GenericGtfsParser::complete_production_date(const std::string& beginning_date,
                                              boost::gregorian::date start_date,
                                              boost::gregorian::date end_date){

    boost::gregorian::date b_date(boost::gregorian::min_date_time);
    if(beginning_date != "")
        b_date = boost::gregorian::from_undelimited_string(beginning_date);

     boost::gregorian::date beginning = std::max(start_date, b_date);

     //technically we cannot handle more than one year of data
     boost::gregorian::date end = std::min(end_date, beginning + boost::gregorian::days(365));

    LOG4CPLUS_INFO(logger, "date de production: " +
                    boost::gregorian::to_simple_string(beginning)
                    + " - " + boost::gregorian::to_simple_string(end));
    //the end of a boost::gregorian::date_period is not in the period
    //since end_date is the last day is the data, we want the end to be the next day
    return boost::gregorian::date_period(beginning, end + boost::gregorian::days(1));
}

void GtfsParser::parse_files(Data& data, const std::string& beginning_date) {

    parse<FeedInfoGtfsHandler>(data, "feed_info.txt");

    manage_production_date(data, beginning_date);

    fill_default_modes(data);


    parse<ShapesGtfsHandler>(data, "shapes.txt");
    parse<AgencyGtfsHandler>(data, "agency.txt", true);
    parse<DefaultContributorHandler>(data);
    parse<StopsGtfsHandler>(data, "stops.txt", true);
    parse<RouteGtfsHandler>(data, "routes.txt", true);
    parse<TransfersGtfsHandler>(data, "transfers.txt");

    parse<CalendarGtfsHandler>(data, "calendar.txt");
    parse<CalendarDatesGtfsHandler>(data, "calendar_dates.txt");
    //after the calendar load, we need to split the validitypattern
    split_validity_pattern_over_dst(data, gtfs_data);

    parse<TripsGtfsHandler>(data, "trips.txt", true);
    parse<StopTimeGtfsHandler>(data, "stop_times.txt", true);
    parse<FrequenciesGtfsHandler>(data, "frequencies.txt");
}
}}
