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
#include <utility>
#include <boost/filesystem.hpp>

namespace nm = ed::types;
using Tokenizer = boost::tokenizer<boost::escaped_list_separator<char>>;

namespace ed {
namespace connectors {

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

nt::RTLevel get_rtlevel_enum(const std::string& str) {
    if (str == "1") {
        return nt::RTLevel::Adapted;
    }
    if (str == "2") {
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
    data.comment_by_id[comment_id] = nt::Comment(comment);
}

static int default_waiting_duration = 120;
static int default_connection_duration = 120;

std::string generate_unique_vj_uri(const GtfsData& gtfs_data, const std::string original_uri, int cpt_vj) {
    // we change the name of the vj since we had to split the original GTFS vj because of dst
    // WARNING: this code is uggly, but it's a quick fix.
    for (int i = 0; i < 100; ++i) {
        // for debugging purpose (since vj uri are useful only for dev purpose)
        // we store if the vj is in conflict
        const std::string separator = (i == 0 ? "dst" : "conflit");
        const std::string vj_uri = original_uri + "_" + separator + "_" + std::to_string(cpt_vj + i);
        // to avoid collision, we check if we find a vj with the name we want to create
        if (gtfs_data.vj_uri.find(vj_uri) == gtfs_data.vj_uri.end()) {
            return vj_uri;
        }
    }
    // we haven't found a unique uri...
    // If this case happens, we need to handle this differently.
    // read all vj beforehand to know how to avoid conflict ?
    throw navitia::exception("impossible to generate a unique uri for the vj " + original_uri
                             + "there are some problems with the dataset. The current code cannot handle that");
}

std::pair<std::string, boost::local_time::time_zone_ptr> TzHandler::get_tz(const std::string& tz_name) {
    if (!tz_name.empty()) {
        auto tz = tz_db.time_zone_from_region(tz_name);
        if (tz) {
            return {tz_name, tz};
        }
        LOG4CPLUS_WARN(log4cplus::Logger::getInstance("ed"), "cannot find " << tz_name << " in tz db");
    }
    // we fetch the default dataset timezone
    return {"", boost::local_time::time_zone_ptr()};
}

ed::types::Network* GtfsData::get_or_create_default_network(ed::Data& data) {
    if (!default_network) {
        LOG4CPLUS_INFO(log4cplus::Logger::getInstance("log"), "creating default network");
        default_network = new ed::types::Network();
        default_network->uri = "default_network";
        default_network->name = "default network";
        data.networks.push_back(default_network);
        network_map[default_network->uri] = default_network;

        // with the default agency comes the default timezone (only if none was provided before)
        if (data.tz_wrapper.tz_name.empty()) {
            LOG4CPLUS_INFO(log4cplus::Logger::getInstance("log"),
                           "no time zone defined, we create a default one for paris");
            const std::string default_tz = UTC_TIMEZONE;  //"Europe/Paris";
            auto timezone = tz.tz_db.time_zone_from_region(default_tz);
            BOOST_ASSERT(timezone);

            data.tz_wrapper = ed::EdTZWrapper(default_tz, timezone);
        }
    }
    return default_network;
}

ed::types::Company* GtfsData::get_or_create_default_company(Data& data) {
    if (!default_company) {
        // default compagny creation
        default_company = new ed::types::Company();
        default_company->uri = "default_company";
        default_company->name = "compagnie par défaut";
        data.companies.push_back(default_company);
        company_map[default_company->uri] = default_company;
    }
    return default_company;
}

int time_to_int(const std::string& time) {
    using tokenizer = boost::tokenizer<boost::char_separator<char>>;
    boost::char_separator<char> sep(":");
    tokenizer tokens(time, sep);
    std::vector<std::string> elts(tokens.begin(), tokens.end());
    int result = 0;
    if (elts.size() != 3)
        return -1;
    try {
        result = boost::lexical_cast<int>(elts[0]) * 3600;
        result += boost::lexical_cast<int>(elts[1]) * 60;
        result += boost::lexical_cast<int>(elts[2]);
    } catch (const boost::bad_lexical_cast&) {
        return std::numeric_limits<int>::min();
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
    if (has_col(feed_end_date_c, row)) {
        data.add_feed_info("feed_end_date", row[feed_end_date_c]);
    }

    if (has_col(feed_start_date_c, row)) {
        data.add_feed_info("feed_start_date", row[feed_start_date_c]);
    }

    if (has_col(feed_publisher_name_c, row)) {
        data.add_feed_info("feed_publisher_name", row[feed_publisher_name_c]);
    }

    if (has_col(feed_publisher_url_c, row)) {
        data.add_feed_info("feed_publisher_url", row[feed_publisher_url_c]);
    }
}

void AgencyGtfsHandler::init(Data&) {
    id_c = csv.get_pos_col("agency_id");
    name_c = csv.get_pos_col("agency_name");
    time_zone_c = csv.get_pos_col("agency_timezone");
}

ed::types::Network* AgencyGtfsHandler::handle_line(Data& data, const csv_row& row, bool is_first_line) {
    if (!is_first_line && !has_col(id_c, row)) {
        LOG4CPLUS_FATAL(
            logger, "Error while reading " + csv.filename + +" file has more than one agency and no agency_id column");
        throw InvalidHeaders(csv.filename);
    }
    auto* network = new nm::Network();

    if (has_col(id_c, row)) {
        network->uri = row[id_c];
    } else {
        network->uri = "default_network";
        gtfs_data.default_network = network;
    }

    network->name = row[name_c];
    data.networks.push_back(network);

    gtfs_data.network_map[network->uri] = network;

    std::string timezone_name = row[time_zone_c];

    if (!data.tz_wrapper.tz_name.empty()) {
        if (data.tz_wrapper.tz_name != timezone_name) {
            LOG4CPLUS_WARN(logger, "Error while reading " << csv.filename
                                                          << " all the time zone are not equals, only the first one "
                                                             "will be considered as the default timezone");
        }
        return network;
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
    LOG4CPLUS_INFO(logger, "default agency tz " << timezone_name << " -> " << tz->std_zone_name());

    return network;
}

void DefaultContributorHandler::init(Data& data) {
    auto* contributor = new nm::Contributor();
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
    id_c = csv.get_pos_col("stop_id");
    code_c = csv.get_pos_col("stop_code");
    lat_c = csv.get_pos_col("stop_lat");
    lon_c = csv.get_pos_col("stop_lon");
    zone_c = csv.get_pos_col("zone_id");
    type_c = csv.get_pos_col("location_type");
    parent_c = csv.get_pos_col("parent_station");
    name_c = csv.get_pos_col("stop_name");
    desc_c = csv.get_pos_col("stop_desc");
    wheelchair_c = csv.get_pos_col("wheelchair_boarding");
    platform_c = csv.get_pos_col("platform_code");
    timezone_c = csv.get_pos_col("stop_timezone");
    if (code_c == UNKNOWN_COLUMN) {
        code_c = id_c;
    } else {
        stop_code_is_present = true;
    }
}

void StopsGtfsHandler::finish(Data& data) {
    // On reboucle pour récupérer les stop areas de tous les stop points
    for (auto sa_sps : gtfs_data.sa_spmap) {
        auto it = gtfs_data.stop_area_map.find(sa_sps.first);
        if (it != gtfs_data.stop_area_map.end()) {
            for (auto sp : sa_sps.second) {
                sp->stop_area = it->second;
            }
        } else {
            std::string error_message = "the stop area " + sa_sps.first + " has not been found for the stop points :  ";
            for (auto sp : sa_sps.second) {
                error_message += sp->uri;
                sp->stop_area = nullptr;
            }
            LOG4CPLUS_WARN(logger, error_message);
        }
    }

    // We fetch the accessibility for all stop points that inherit from their stop area's accessibility
    for (auto sp : wheelchair_heritance) {
        if (sp->stop_area == nullptr)
            continue;
        if (sp->stop_area->property(navitia::type::hasProperties::WHEELCHAIR_BOARDING)) {
            sp->set_property(navitia::type::hasProperties::WHEELCHAIR_BOARDING);
        }
    }

    handle_stop_point_without_area(data);

    LOG4CPLUS_TRACE(logger, data.stop_points.size() << " added stop points");
    ;
    LOG4CPLUS_TRACE(logger, data.stop_areas.size() << " added stop areas");
    LOG4CPLUS_TRACE(logger, ignored << " points ignored because of dupplicates");
}

void StopsGtfsHandler::handle_stop_point_without_area(Data& data) {
    int nb_added_sa(0);
    // we artificialy create one stop_area for stop point without one
    for (const auto sp : data.stop_points) {
        if (sp->stop_area) {
            continue;
        }
        auto sa = new nm::StopArea;

        sa->coord.set_lon(sp->coord.lon());
        sa->coord.set_lat(sp->coord.lat());
        sa->name = sp->name;
        sa->uri = "Navitia:" + sp->uri;
        if (sp->property(navitia::type::hasProperties::WHEELCHAIR_BOARDING))
            sa->set_property(navitia::type::hasProperties::WHEELCHAIR_BOARDING);

        gtfs_data.stop_area_map[sa->uri] = sa;
        data.stop_areas.push_back(sa);
        sp->stop_area = sa;

        // if the stoppoint had a tz, it becomes the stop area's, else we fetch the default timezone
        auto it_tz = gtfs_data.tz.stop_point_tz.find(sp);
        if (it_tz != gtfs_data.tz.stop_point_tz.end()) {
            sa->time_zone_with_name = gtfs_data.tz.get_tz(it_tz->second);
        } else {
            // we fetch the defautl dataset timezone
            sa->time_zone_with_name = {data.tz_wrapper.tz_name, data.tz_wrapper.boost_timezone};
        }
        nb_added_sa++;
    }

    if (nb_added_sa) {
        LOG4CPLUS_INFO(
            logger,
            nb_added_sa << " stop_points without stop_area. Creation of a stop_area for each of those stop_points");
    }
}

template <typename T>
bool StopsGtfsHandler::parse_common_data(const csv_row& row, T* stop) {
    try {
        stop->coord.set_lon(boost::lexical_cast<double>(row[lon_c]));
        stop->coord.set_lat(boost::lexical_cast<double>(row[lat_c]));
    } catch (const boost::bad_lexical_cast&) {
        LOG4CPLUS_WARN(logger,
                       "Impossible to parse the coordinate for " + row[id_c] + " " + row[code_c] + " " + row[name_c]);
        return false;
    }
    if (!stop->coord.is_valid()) {
        LOG4CPLUS_WARN(logger, "The stop " + row[id_c] + " " + row[code_c] + " " + row[name_c]
                                   + " has been ignored because the coordinates are not valid (" + row[lon_c] + ", "
                                   + row[lat_c] + ")");
        return false;
    }

    stop->name = row[name_c];
    stop->uri = row[id_c];

    return true;
}

nm::AccessPoint* StopsGtfsHandler::build_access_point(Data& data, const csv_row& row) {
    auto* ap = new nm::AccessPoint();
    if (!parse_common_data(row, ap)) {
        delete ap;
        return nullptr;
    }
    if (has_col(parent_c, row) && row[parent_c] != "") {
        ap->parent_station = row[parent_c];
    }
    gtfs_data.access_point_map[ap->uri] = ap;
    data.access_points.push_back(ap);

    return ap;
}

nm::StopArea* StopsGtfsHandler::build_stop_area(Data& data, const csv_row& row) {
    auto* sa = new nm::StopArea();
    if (!parse_common_data(row, sa)) {
        delete sa;  // don't forget to free the data
        return nullptr;
    }

    if (has_col(wheelchair_c, row) && row[wheelchair_c] == "1") {
        sa->set_property(navitia::type::hasProperties::WHEELCHAIR_BOARDING);
    }
    gtfs_data.stop_area_map[sa->uri] = sa;
    data.stop_areas.push_back(sa);
    if (stop_code_is_present) {
        add_gtfs_stop_code(data, sa, row[code_c]);
    }

    if (has_col(timezone_c, row)) {
        auto tz_name = row[timezone_c];
        sa->time_zone_with_name = gtfs_data.tz.get_tz(tz_name);
    }
    if (!sa->time_zone_with_name.second) {
        // if no timezone has been found, we set the default one
        sa->time_zone_with_name = {data.tz_wrapper.tz_name, data.tz_wrapper.boost_timezone};
    }
    if (has_col(desc_c, row)) {
        add_gtfs_comment(gtfs_data, data, sa, row[desc_c]);
    }
    return sa;
}

nm::StopPoint* StopsGtfsHandler::build_stop_point(Data& data, const csv_row& row) {
    auto* sp = new nm::StopPoint();
    if (!parse_common_data(row, sp)) {
        delete sp;
        return nullptr;
    }

    if (has_col(zone_c, row) && row[zone_c] != "") {
        sp->fare_zone = row[zone_c];
    }

    if (has_col(wheelchair_c, row)) {
        if (row[wheelchair_c] == "0") {
            wheelchair_heritance.push_back(sp);
        } else if (row[wheelchair_c] == "1") {
            sp->set_property(navitia::type::hasProperties::WHEELCHAIR_BOARDING);
        }
    }
    if (has_col(platform_c, row)) {
        sp->platform_code = row[platform_c];
    }
    gtfs_data.stop_point_map[sp->uri] = sp;
    data.stop_points.push_back(sp);
    if (stop_code_is_present) {
        add_gtfs_stop_code(data, sp, row[code_c]);
    }

    if (has_col(parent_c, row) && row[parent_c] != "") {  // we save the reference to the stop area
        auto it = gtfs_data.sa_spmap.find(row[parent_c]);
        if (it == gtfs_data.sa_spmap.end()) {
            it = gtfs_data.sa_spmap.insert(std::make_pair(row[parent_c], GtfsData::vector_sp())).first;
        }
        it->second.push_back(sp);
    }

    if (has_col(desc_c, row)) {
        add_gtfs_comment(gtfs_data, data, sp, row[desc_c]);
    }

    // we save the tz in case the stop point is later promoted to stop area
    if (has_col(timezone_c, row) && !row[timezone_c].empty()) {
        gtfs_data.tz.stop_point_tz[sp] = row[timezone_c];
    }
    return sp;
}

bool StopsGtfsHandler::is_duplicate(const csv_row& row) {
    // In GTFS the file contains the stop_area and the stop_point
    // We test if it's a duplicate
    if (gtfs_data.stop_point_map.find(row[id_c]) != gtfs_data.stop_point_map.end()
        || gtfs_data.stop_area_map.find(row[id_c]) != gtfs_data.stop_area_map.end()) {
        LOG4CPLUS_WARN(logger, "The stop " + row[id_c] + " has been ignored");
        ignored++;
        return true;
    }
    return false;
}

StopsGtfsHandler::stop_point_and_area StopsGtfsHandler::handle_line(Data& data, const csv_row& row, bool) {
    stop_point_and_area return_wrapper{};

    if (is_duplicate(row)) {
        return return_wrapper;
    }

    // location_type == 1 => StopArea
    if (has_col(type_c, row) && row[type_c] == "1") {
        auto* sa = build_stop_area(data, row);
        if (sa) {
            return_wrapper.second = sa;
        }
        return return_wrapper;

    } else if ((has_col(type_c, row) && row[type_c] == "0") || !has_col(type_c, row)) {
        // location_type == 0 => StopPoint
        auto* sp = build_stop_point(data, row);
        if (sp) {
            return_wrapper.first = sp;
        }
        return return_wrapper;
        // Handle I/O case
    } else if (has_col(type_c, row) && row[type_c] == "3") {
        auto* sp = build_access_point(data, row);
        if (sp) {
            return {};
        }
        return return_wrapper;
    } else {
        // we ignore pathways nodes
        return {};
    }
}

void PathWayGtfsHandler::init(Data&) {
    pathway_id_c = csv.get_pos_col("pathway_id");
    from_stop_id_c = csv.get_pos_col("from_stop_id");
    to_stop_id_c = csv.get_pos_col("to_stop_id");
    pathway_mode_c = csv.get_pos_col("pathway_mode");
    is_bidirectional_c = csv.get_pos_col("is_bidirectional");
    length_c = csv.get_pos_col("length");
    traversal_time_c = csv.get_pos_col("traversal_time");
    stair_count_c = csv.get_pos_col("stair_count");
    max_slope_c = csv.get_pos_col("max_slope");
    min_width_c = csv.get_pos_col("min_width");
    signposted_as_c = csv.get_pos_col("signposted_as");
    reversed_signposted_as_c = csv.get_pos_col("reversed_signposted_as");
}

int PathWayGtfsHandler::fill_pathway_field(const csv_row& row, const int key, const std::string column_name) {
    int pw_value = DEFAULT_PATHWAYS_VALUE;
    if (has_col(key, row) && row[key] != "") {
        try {
            return (int)boost::lexical_cast<float>(row[key]);
        } catch (boost::bad_lexical_cast const& e) {
            LOG4CPLUS_ERROR(log4cplus::Logger::getInstance("log"),
                            "impossible to parse " << column_name << ": " << row[key]);
        }
    }
    return pw_value;
}

ed::types::PathWay* PathWayGtfsHandler::handle_line(Data& data, const csv_row& row, bool) {
    auto* pw = new ed::types::PathWay();

    // Mandatory fields
    pw->uri = row[pathway_id_c];
    pw->name = row[pathway_id_c];
    pw->from_stop_id = row[from_stop_id_c];
    pw->to_stop_id = row[to_stop_id_c];
    pw->pathway_mode = fill_pathway_field(row, pathway_mode_c, "pathway.pathway_mode");
    pw->is_bidirectional = fill_pathway_field(row, is_bidirectional_c, "pathway.is_bidirectional");
    ;

    // Optionnal fields

    // length
    pw->length = fill_pathway_field(row, length_c, "pathway.length");
    // traversal_time
    pw->traversal_time = fill_pathway_field(row, traversal_time_c, "pathway.traversal_time");
    // stair_count
    pw->stair_count = fill_pathway_field(row, stair_count_c, "pathway.stair_count");
    // max_slope
    pw->max_slope = fill_pathway_field(row, max_slope_c, "pathway.max_slope");
    // min_width
    pw->min_width = fill_pathway_field(row, min_width_c, "pathway.min_width");
    // signposted_as
    if (has_col(signposted_as_c, row) && row[signposted_as_c] != "") {
        pw->signposted_as = row[signposted_as_c];
    }
    // reversed_signposted_as
    if (has_col(reversed_signposted_as_c, row) && row[reversed_signposted_as_c] != "") {
        pw->reversed_signposted_as = row[reversed_signposted_as_c];
    }

    // add new data
    gtfs_data.pathway_map[row[pathway_id_c]] = pw;
    data.pathways.push_back(pw);

    return pw;
}

void PathWayGtfsHandler::finish(Data& data) {
    BOOST_ASSERT(data.pathways.size() == gtfs_data.pathway_map.size());
    LOG4CPLUS_TRACE(logger, "Nb pathways: " << data.pathways.size());
}

void RouteGtfsHandler::init(Data&) {
    id_c = csv.get_pos_col("route_id");
    short_name_c = csv.get_pos_col("route_short_name");
    long_name_c = csv.get_pos_col("route_long_name");
    type_c = csv.get_pos_col("route_type");
    desc_c = csv.get_pos_col("route_desc");
    color_c = csv.get_pos_col("route_color");
    agency_c = csv.get_pos_col("agency_id");
    text_color_c = csv.get_pos_col("route_text_color");
}

nm::Line* RouteGtfsHandler::handle_line(Data& data, const csv_row& row, bool) {
    if (gtfs_data.line_map.find(row[id_c]) != gtfs_data.line_map.end()) {
        ignored++;
        LOG4CPLUS_WARN(logger, "duplicate on route line " + row[id_c]);
        return nullptr;
    }

    auto* line = new nm::Line();
    line->uri = row[id_c];
    line->name = row[long_name_c];
    line->code = row[short_name_c];
    if (has_col(desc_c, row)) {
        add_gtfs_comment(gtfs_data, data, line, row[desc_c]);
    }

    if (has_col(color_c, row)) {
        line->color = row[color_c];
    }
    if (is_valid(text_color_c, row)) {
        line->text_color = row[text_color_c];
    }
    line->additional_data = row[long_name_c];

    auto it_commercial_mode = gtfs_data.commercial_mode_map.find(row[type_c]);
    if (it_commercial_mode != gtfs_data.commercial_mode_map.end())
        line->commercial_mode = it_commercial_mode->second;
    else {
        LOG4CPLUS_ERROR(logger,
                        "impossible to find route type " << row[type_c] << " we ignore the route " << row[id_c]);
        ignored++;
        delete line;
        return nullptr;
    }

    if (is_valid(agency_c, row)) {
        auto network_it = gtfs_data.network_map.find(row[agency_c]);
        if (network_it != gtfs_data.network_map.end()) {
            line->network = network_it->second;
        } else {
            throw navitia::exception("line " + line->uri + " has an unknown network: " + row[agency_c]
                                     + ", the dataset is not valid");
        }
    } else {
        // if the line has no network, we get_or_create the default one
        line->network = gtfs_data.get_or_create_default_network(data);
    }

    gtfs_data.line_map[row[id_c]] = line;
    data.lines.push_back(line);
    return line;
}

void RouteGtfsHandler::finish(Data& data) {
    BOOST_ASSERT(data.lines.size() == gtfs_data.line_map.size());
    LOG4CPLUS_TRACE(logger, "Nb routes: " << data.lines.size());
    LOG4CPLUS_TRACE(logger, ignored << " routes have been ignored because of duplicates");
}

void TransfersGtfsHandler::init(Data&) {
    from_c = csv.get_pos_col("from_stop_id");
    to_c = csv.get_pos_col("to_stop_id");
    time_c = csv.get_pos_col("min_transfer_time");
}

void TransfersGtfsHandler::finish(Data& data) {
    LOG4CPLUS_TRACE(logger, nblines << " correspondances prises en compte sur " << data.stop_point_connections.size());
}

void TransfersGtfsHandler::fill_stop_point_connection(nm::StopPointConnection* connection, const csv_row& row) const {
    if (has_col(time_c, row)) {
        try {
            // the GTFS transfers duration already take into account a tolerance so duration and display duration are
            // equal
            connection->display_duration = boost::lexical_cast<int>(row[time_c]);
            connection->duration = connection->display_duration;
        } catch (const boost::bad_lexical_cast&) {
            // if no transfers time is given, we add an additional waiting time to the real duration
            // ie you want to walk for 2 mn for your transfert, and we add 2 more minutes to add robustness to your
            // transfers
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
        if (boost::lexical_cast<int>(row[time_c]) > 24 * 3600) {
            ++data.count_too_long_connections;
            return;
        }
    } catch (const boost::bad_lexical_cast&) {
        LOG4CPLUS_WARN(logger, "Invalid connection time: " + row[time_c]);
        return;
    }
    auto it = gtfs_data.stop_point_map.find(row[from_c]);
    if (it == gtfs_data.stop_point_map.end()) {
        auto it_sa = gtfs_data.sa_spmap.find(row[from_c]);
        if (it_sa == gtfs_data.sa_spmap.end()) {
            LOG4CPLUS_WARN(logger, "Impossible de find the stop point (from) " + row[from_c]);
            return;
        }
        departures = it_sa->second;
    } else {
        departures.push_back(it->second);
    }

    it = gtfs_data.stop_point_map.find(row[to_c]);
    if (it == gtfs_data.stop_point_map.end()) {
        auto it_sa = gtfs_data.sa_spmap.find(row[to_c]);
        if (it_sa == gtfs_data.sa_spmap.end()) {
            LOG4CPLUS_WARN(logger, "Impossible de find the stop point (to) " + row[to_c]);
            return;
        }
        arrivals = it_sa->second;
    } else {
        arrivals.push_back(it->second);
    }

    for (auto from_sp : departures) {
        for (auto to_sp : arrivals) {
            auto* connection = new nm::StopPointConnection();
            connection->departure = from_sp;
            connection->destination = to_sp;
            connection->uri = from_sp->uri + "=>" + to_sp->uri;
            connection->connection_kind = types::ConnectionType::Walking;

            fill_stop_point_connection(connection, row);

            data.stop_point_connections.push_back(connection);
        }
    }
    nblines++;
}

void CalendarGtfsHandler::init(Data& data) {
    data.validity_patterns.reserve(10000);

    id_c = csv.get_pos_col("service_id");
    monday_c = csv.get_pos_col("monday");
    tuesday_c = csv.get_pos_col("tuesday");
    wednesday_c = csv.get_pos_col("wednesday");
    thursday_c = csv.get_pos_col("thursday");
    friday_c = csv.get_pos_col("friday");
    saturday_c = csv.get_pos_col("saturday");
    sunday_c = csv.get_pos_col("sunday");
    start_date_c = csv.get_pos_col("start_date");
    end_date_c = csv.get_pos_col("end_date");
}

void CalendarGtfsHandler::finish(Data& data) {
    LOG4CPLUS_TRACE(logger, "Nb validity patterns : " << data.validity_patterns.size() << " nb lines : " << nb_lines);
    BOOST_ASSERT(data.validity_patterns.size() == gtfs_data.tz.vp_by_name.size());
}

void CalendarGtfsHandler::handle_line(Data& data, const csv_row& row, bool) {
    std::bitset<7> week;
    nb_lines++;

    if (gtfs_data.tz.non_split_vp.find(row[id_c]) != gtfs_data.tz.non_split_vp.end()) {
        return;
    }
    // we only build temporary validity pattern that will be split after the CalendarDateParser has been called
    nt::ValidityPattern& vp = gtfs_data.tz.non_split_vp[row[id_c]];
    vp.beginning_date = data.meta.production_date.begin();
    vp.uri = row[id_c];

    // week day init (remember that sunday is the first day of week in the us)
    week[1] = (row[monday_c] == "1");
    week[2] = (row[tuesday_c] == "1");
    week[3] = (row[wednesday_c] == "1");
    week[4] = (row[thursday_c] == "1");
    week[5] = (row[friday_c] == "1");
    week[6] = (row[saturday_c] == "1");
    week[0] = (row[sunday_c] == "1");

    // Init the validity period
    boost::gregorian::date b_date = boost::gregorian::from_undelimited_string(row[start_date_c]);

    // we add one day to the last day of the period because end_date_c is included in the period
    // and as say the constructor of boost periods, the second args is "end" so it is not included
    boost::gregorian::date e_date = boost::gregorian::from_undelimited_string(row[end_date_c]) + 1_days;

    boost::gregorian::date_period full_period(b_date, e_date);
    auto period = full_period.intersection(data.meta.production_date);

    for (boost::gregorian::day_iterator it(period.begin()); it < period.end(); ++it) {
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
        assert(end == std::numeric_limits<size_t>::min());  // if we did not find beg, end cannot be found too
        LOG4CPLUS_INFO(log4cplus::Logger::getInstance("log"),
                       "the calendar " << vp.uri << " has an empty validity period, we will ignore it");
        return {vp.beginning_date, vp.beginning_date};  // return null period
    }

    return {vp.beginning_date + boost::gregorian::days(beg), vp.beginning_date + boost::gregorian::days(end + 1)};
}

/*
 * Since all time have to be converted to UTC, we need to handle day saving time (DST) rules
 * we thus need to split all periods for them to be on only one DST
 * luckily in GTFS format all times are given in the same timezone (the default tz dataset) even if all stop areas are
 * not on the same tz
 */
void split_validity_pattern_over_dst(Data& data, GtfsData& gtfs_data) {
    // we start by filling the global tz_handler
    data.tz_wrapper.build_tz(data.meta.production_date);

    for (const auto& name_and_vp : gtfs_data.tz.non_split_vp) {
        const nt::ValidityPattern& original_vp = name_and_vp.second;

        boost::gregorian::date_period smallest_active_period = compute_smallest_active_period(original_vp);

        auto split_periods = data.tz_wrapper.split_over_dst(smallest_active_period);

        BOOST_ASSERT(!split_periods.empty()
                     || smallest_active_period
                            .is_null());  // by construction it cannot be empty if the validity period is not null

        size_t cpt(1);
        for (const auto& utc_shit_and_periods : split_periods) {
            auto* vp = new nt::ValidityPattern(data.meta.production_date.begin());

            for (const auto& period : utc_shit_and_periods.second) {
                for (boost::gregorian::day_iterator it(period.begin()); it < period.end(); ++it) {
                    if (original_vp.check(*it)) {
                        vp->add(*it);
                    }
                }
            }

            if (split_periods.size() == 1) {
                // we do not change the id if the period is not split
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
    id_c = csv.get_pos_col("service_id");
    date_c = csv.get_pos_col("date");
    e_type_c = csv.get_pos_col("exception_type");
}

void CalendarDatesGtfsHandler::handle_line(Data& data, const csv_row& row, bool) {
    bool new_vp = gtfs_data.tz.non_split_vp.find(row[id_c]) == gtfs_data.tz.non_split_vp.end();

    nt::ValidityPattern& vp = gtfs_data.tz.non_split_vp[row[id_c]];

    if (new_vp) {
        vp.beginning_date = data.meta.production_date.begin();
        vp.uri = row[id_c];
    }

    auto date = boost::gregorian::from_undelimited_string(row[date_c]);
    if (row[e_type_c] == "1")
        vp.add(date);
    else if (row[e_type_c] == "2")
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
        const navitia::type::GeographicalCoord coord(boost::lexical_cast<double>(row.at(shape_pt_lon_c)),
                                                     boost::lexical_cast<double>(row.at(shape_pt_lat_c)));
        const int seq = boost::lexical_cast<int>(row.at(shape_pt_sequence_c));
        shapes[row.at(shape_id_c)][seq] = coord;
    } catch (std::exception& e) {
        LOG4CPLUS_WARN(logger, "Exception in shapes.txt: " << e.what());
    }
}

void ShapesGtfsHandler::finish(Data& data) {
    LOG4CPLUS_INFO(logger, "Nb shapes: " << shapes.size());
    for (const auto& shape : shapes) {
        navitia::type::LineString line;
        for (const auto& coord : shape.second) {
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
    trip_headsign_c = csv.get_pos_col("trip_headsign");
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
        auto* route = new types::Route();
        route->line = route_id.first;
        // uri is {line}:{direction}
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
        LOG4CPLUS_WARN(logger,
                       "Impossible to find the Gtfs line " << row[id_c] << " referenced by trip " << row[trip_c]);
        ignored++;
        return;
    }

    nm::Line* line = it->second;

    // direction_id is optional (and possible values "0" or "1"), so defaulting to "0"
    std::string direction_id = "";
    if (direction_id_c != -1) {
        direction_id = row[direction_id_c];
    }
    nm::Route* route = get_or_create_route(data, {line, direction_id});

    auto vp_range = gtfs_data.tz.vp_by_name.equal_range(row[service_c]);
    if (empty(vp_range)) {
        LOG4CPLUS_WARN(logger,
                       "Impossible to find the Gtfs service " + row[service_c] + " referenced by trip " + row[trip_c]);
        ignored++;
        return;
    }

    // we look in the meta vj table to see if we already have one such vj
    if (data.meta_vj_map.find(row[trip_c]) != data.meta_vj_map.end()) {
        LOG4CPLUS_DEBUG(logger, "vj " << row[trip_c] << " already read, we skip the second one");
        ignored_vj++;
        return;
    }

    types::MetaVehicleJourney& meta_vj = data.meta_vj_map[row[trip_c]];  // we get a ref on a newly created meta vj
    meta_vj.uri = row[trip_c];

    // get shape if possible
    const std::string& shape_id = has_col(shape_id_c, row) ? row.at(shape_id_c) : "";

    bool has_been_split = more_than_one_elt(vp_range);  // check if the trip has been split over dst

    size_t cpt_vj(1);
    // the validity pattern may have been split because of DST, so we need to create one vj for each
    for (auto vp_it = vp_range.first; vp_it != vp_range.second; ++vp_it, cpt_vj++) {
        nt::ValidityPattern* vp_xx = vp_it->second;

        auto* vj = new nm::VehicleJourney();
        const std::string original_uri = row[trip_c];
        std::string vj_uri = original_uri;
        if (has_been_split) {
            vj_uri = generate_unique_vj_uri(gtfs_data, original_uri, cpt_vj);
        }

        vj->uri = vj_uri;
        if (has_col(trip_headsign_c, row))
            vj->name = row[trip_headsign_c];
        else
            vj->name = vj->uri;

        vj->validity_pattern = vp_xx;
        vj->adapted_validity_pattern = vp_xx;
        vj->route = route;
        if (has_col(block_id_c, row))
            vj->block_id = row[block_id_c];
        else
            vj->block_id = "";
        if (has_col(wheelchair_c, row) && row[wheelchair_c] == "1")
            vj->set_vehicle(navitia::type::hasVehicleProperties::WHEELCHAIR_ACCESSIBLE);
        if (has_col(bikes_c, row) && row[bikes_c] == "1")
            vj->set_vehicle(navitia::type::hasVehicleProperties::BIKE_ACCEPTED);

        auto itm = gtfs_data.physical_mode_map.find(line->commercial_mode->uri);
        if (itm != gtfs_data.physical_mode_map.end()) {
            vj->physical_mode = itm->second;
        }

        vj->company = gtfs_data.get_or_create_default_company(data);

        gtfs_data.tz.vj_by_name.insert({original_uri, vj});

        data.vehicle_journeys.push_back(vj);
        // we add them on our meta vj
        meta_vj.theoric_vj.push_back(vj);
        vj->meta_vj_name = row[trip_c];
        vj->shape_id = shape_id;
    }
}

void StopTimeGtfsHandler::init(Data&) {
    LOG4CPLUS_INFO(logger, "reading stop times");
    trip_c = csv.get_pos_col("trip_id");
    arrival_c = csv.get_pos_col("arrival_time");
    departure_c = csv.get_pos_col("departure_time");
    stop_c = csv.get_pos_col("stop_id");
    stop_seq_c = csv.get_pos_col("stop_sequence");
    pickup_c = csv.get_pos_col("pickup_type");
    drop_off_c = csv.get_pos_col("drop_off_type");
}

void StopTimeGtfsHandler::finish(Data& data) {
    LOG4CPLUS_INFO(logger, "sorting stoptimes of vehicle_journeys");
    for (auto* const vj : data.vehicle_journeys) {
        if (vj->stop_time_list.empty())
            continue;

        boost::sort(vj->stop_time_list, [](const nm::StopTime* st1, const nm::StopTime* st2) -> bool {
            if (st1->order == st2->order) {
                throw navitia::exception("two stoptime with the same order (" + std::to_string(st1->order)
                                         + ") for vj: " + st1->vehicle_journey->uri);
            }
            return st1->order < st2->order;
        });

        for (size_t it_st = 0; it_st < vj->stop_time_list.size(); ++it_st) {
            vj->stop_time_list[it_st]->order = it_st;
        }

        if (vj->stop_time_list.size() < 2) {
            continue;
        }
        // we check that the vj, once ordered by order are correctly sorted on the departure times
        auto it_st = vj->stop_time_list.begin();
        for (auto it_next = it_st + 1; it_next != vj->stop_time_list.end(); it_next++, it_st++) {
            const auto* st1 = *it_st;
            const auto* st2 = *it_next;
            bool is_valid_vj = true;
            if (!(st1->arrival_time <= st1->departure_time && st1->departure_time <= st2->arrival_time
                  && st2->arrival_time <= st2->departure_time)) {
                is_valid_vj = false;
                LOG4CPLUS_INFO(logger, "invalid vj " << vj->uri << ", stop times are not correctly ordered");
            }
            int neg_inf = std::numeric_limits<int>::min();
            if (st1->arrival_time == neg_inf || st1->departure_time == neg_inf || st2->arrival_time == neg_inf
                || st2->departure_time == neg_inf) {
                is_valid_vj = false;
                LOG4CPLUS_INFO(logger, "invalid vj " << vj->uri << ", stop times are not correct");
            }
            if (!is_valid_vj) {
                LOG4CPLUS_INFO(logger, "concerned stop times : "
                                           << "stop time " << st1->order << " [" << st1->arrival_time << ", "
                                           << st1->departure_time << "]"
                                           << " stop time " << st2->order << " [" << st2->arrival_time << ", "
                                           << st2->departure_time << "]"
                                           << " we erase the VJ");
                data.vj_to_erase.insert(vj);
                break;
            }
        }
    }
    LOG4CPLUS_INFO(logger, "Nb stop times: " << data.stops.size());
}

static int to_utc(const std::string& local_time, int utc_offset) {
    int local = time_to_int(local_time);
    if (local != std::numeric_limits<int>::min()) {
        local -= utc_offset;
    }
    return local;
}

std::vector<nm::StopTime*> StopTimeGtfsHandler::handle_line(Data& data, const csv_row& row, bool) {
    auto stop_it = gtfs_data.stop_point_map.find(row[stop_c]);
    if (stop_it == gtfs_data.stop_point_map.end()) {
        LOG4CPLUS_WARN(logger, "Impossible to find the stop_point " + row[stop_c] + "!");
        return {};
    }

    auto vj_it = gtfs_data.tz.vj_by_name.lower_bound(row[trip_c]);
    if (vj_it == gtfs_data.tz.vj_by_name.end()) {
        LOG4CPLUS_WARN(logger, "Impossible to find the vehicle_journey '" << row[trip_c] << "'");
        return {};
    }
    std::vector<nm::StopTime*> stop_times;

    // the validity pattern may have been split because of DST, so we need to create one vj for each
    for (auto vj_end_it = gtfs_data.tz.vj_by_name.upper_bound(row[trip_c]); vj_it != vj_end_it; ++vj_it) {
        auto* stop_time = new nm::StopTime();

        // we need to convert the stop times in UTC
        int utc_offset = data.tz_wrapper.tz_handler.get_utc_offset(*vj_it->second->validity_pattern);

        stop_time->arrival_time = to_utc(row[arrival_c], utc_offset);
        stop_time->departure_time = to_utc(row[departure_c], utc_offset);

        // GTFS don't handle boarding / alighting duration, assuming 0
        stop_time->alighting_time = stop_time->arrival_time;
        stop_time->boarding_time = stop_time->departure_time;

        stop_time->stop_point = stop_it->second;
        stop_time->order = boost::lexical_cast<unsigned int>(row[stop_seq_c]);
        stop_time->vehicle_journey = vj_it->second;

        if (has_col(pickup_c, row) && has_col(drop_off_c, row)) {
            stop_time->ODT = (row[pickup_c] == "2" || row[drop_off_c] == "2");
            stop_time->skipped_stop = (row[pickup_c] == "3" && row[drop_off_c] == "3");
        } else {
            stop_time->ODT = false;
            stop_time->skipped_stop = false;
        }
        if (has_col(pickup_c, row))
            stop_time->pick_up_allowed = (row[pickup_c] != "1" && row[pickup_c] != "3");
        else
            stop_time->pick_up_allowed = true;
        if (has_col(drop_off_c, row))
            stop_time->drop_off_allowed = (row[drop_off_c] != "1" && row[drop_off_c] != "3");
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
    trip_id_c = csv.get_pos_col("trip_id");
    start_time_c = csv.get_pos_col("start_time");
    end_time_c = csv.get_pos_col("end_time");
    headway_secs_c = csv.get_pos_col("headway_secs");
}

void FrequenciesGtfsHandler::handle_line(Data& data, const csv_row& row, bool) {
    for (auto vj_end_it = gtfs_data.tz.vj_by_name.upper_bound(row[trip_id_c]),
              vj_it = gtfs_data.tz.vj_by_name.lower_bound(row[trip_id_c]);
         vj_it != vj_end_it; ++vj_it) {
        auto* vj = vj_it->second;

        if (vj->stop_time_list.empty()) {
            LOG4CPLUS_WARN(logger, "vj " << row[trip_id_c] << " is empty cannot add stoptimes in frequencies");
            continue;
        }

        // we need to convert the stop times in UTC
        int utc_offset = data.tz_wrapper.tz_handler.get_utc_offset(*vj->validity_pattern);

        vj->start_time = to_utc(row[start_time_c], utc_offset);
        vj->end_time = to_utc(row[end_time_c], utc_offset);
        vj->headway_secs = boost::lexical_cast<int>(row[headway_secs_c]);
        const auto& start = vj->earliest_time();
        if (vj->start_time <= vj->stop_time_list.front()->arrival_time && start < vj->start_time) {
            vj->end_time -= (vj->start_time - start);
            vj->start_time = start;
        }
        int first_st_gap = vj->start_time - start;
        for (auto st : vj->stop_time_list) {
            st->is_frequency = true;
            st->arrival_time += first_st_gap;
            st->departure_time += first_st_gap;
            st->alighting_time += first_st_gap;
            st->boarding_time += first_st_gap;
        }
    }
}

GenericGtfsParser::GenericGtfsParser(std::string path) : path(std::move(path)) {
    logger = log4cplus::Logger::getInstance("log");
}
GenericGtfsParser::~GenericGtfsParser() = default;

void GenericGtfsParser::fill(Data& data, const std::string& beginning_date) {
    parse_files(data, beginning_date);

    normalize_extcodes(data);
}

void GenericGtfsParser::fill_default_modes(Data& data) {
    // default commercial_mode and physical_modes
    // all modes are represented by a number in GTFS
    // see route_type in https://developers.google.com/transit/gtfs/reference?hl=fr-FR#routestxt
    auto* physical_mode = new ed::types::PhysicalMode();
    physical_mode->uri = "Tramway";
    physical_mode->name = "Tramway";
    data.physical_modes.push_back(physical_mode);
    // NOTE: physical mode don't need to be indexed by the GTFS code, since they don't exist in GTFS
    gtfs_data.physical_mode_map[physical_mode->uri] = physical_mode;
    auto* commercial_mode = new ed::types::CommercialMode();
    commercial_mode->uri = physical_mode->uri;
    commercial_mode->name = "Tram, Streetcar, Light rail";
    data.commercial_modes.push_back(commercial_mode);
    gtfs_data.commercial_mode_map["0"] = commercial_mode;

    physical_mode = new ed::types::PhysicalMode();
    physical_mode->uri = "Metro";
    physical_mode->name = "Metro";
    data.physical_modes.push_back(physical_mode);
    gtfs_data.physical_mode_map[physical_mode->uri] = physical_mode;
    commercial_mode = new ed::types::CommercialMode();
    commercial_mode->uri = physical_mode->uri;
    commercial_mode->name = "Subway, Metro";
    data.commercial_modes.push_back(commercial_mode);
    gtfs_data.commercial_mode_map["1"] = commercial_mode;

    physical_mode = new ed::types::PhysicalMode();
    physical_mode->uri = "Train";
    physical_mode->name = "Train";
    data.physical_modes.push_back(physical_mode);
    gtfs_data.physical_mode_map[physical_mode->uri] = physical_mode;
    commercial_mode = new ed::types::CommercialMode();
    commercial_mode->uri = physical_mode->uri;
    commercial_mode->name = "Rail";
    data.commercial_modes.push_back(commercial_mode);
    gtfs_data.commercial_mode_map["2"] = commercial_mode;

    physical_mode = new ed::types::PhysicalMode();
    physical_mode->uri = "Bus";
    physical_mode->name = "Bus";
    data.physical_modes.push_back(physical_mode);
    gtfs_data.physical_mode_map[physical_mode->uri] = physical_mode;
    commercial_mode = new ed::types::CommercialMode();
    commercial_mode->uri = physical_mode->uri;
    commercial_mode->name = "Bus";
    data.commercial_modes.push_back(commercial_mode);
    gtfs_data.commercial_mode_map["3"] = commercial_mode;

    physical_mode = new ed::types::PhysicalMode();
    physical_mode->uri = "Ferry";
    physical_mode->name = "Ferry";
    data.physical_modes.push_back(physical_mode);
    gtfs_data.physical_mode_map[physical_mode->uri] = physical_mode;
    commercial_mode = new ed::types::CommercialMode();
    commercial_mode->uri = physical_mode->uri;
    commercial_mode->name = "Ferry";
    data.commercial_modes.push_back(commercial_mode);
    gtfs_data.commercial_mode_map["4"] = commercial_mode;

    physical_mode = new ed::types::PhysicalMode();
    physical_mode->uri = "SuspendedCableCar";
    physical_mode->name = "Suspended Cable Car";
    data.physical_modes.push_back(physical_mode);
    gtfs_data.physical_mode_map[physical_mode->uri] = physical_mode;
    commercial_mode = new ed::types::CommercialMode();
    commercial_mode->uri = physical_mode->uri;
    commercial_mode->name = "Gondola, Suspended cable car";
    data.commercial_modes.push_back(commercial_mode);
    gtfs_data.commercial_mode_map["6"] = commercial_mode;

    physical_mode = new ed::types::PhysicalMode();
    physical_mode->uri = "Funicular";
    physical_mode->name = "Funicular";
    data.physical_modes.push_back(physical_mode);
    gtfs_data.physical_mode_map[physical_mode->uri] = physical_mode;
    commercial_mode = new ed::types::CommercialMode();
    commercial_mode->uri = physical_mode->uri;
    commercial_mode->name = "Funicular";
    data.commercial_modes.push_back(commercial_mode);
    gtfs_data.commercial_mode_map["7"] = commercial_mode;

    commercial_mode = new ed::types::CommercialMode();
    commercial_mode->uri = "CableCar";
    commercial_mode->name = "Cable car";
    data.commercial_modes.push_back(commercial_mode);
    gtfs_data.commercial_mode_map["5"] = commercial_mode;
    // for physical mode, CableCar is Funicular
    gtfs_data.physical_mode_map[commercial_mode->uri] = physical_mode;
}

void normalize_extcodes(Data& data) {
    for (nm::StopArea* sa : data.stop_areas) {
        boost::algorithm::replace_first(sa->uri, "StopArea:", "");
    }
    for (nm::StopPoint* sp : data.stop_points) {
        boost::algorithm::replace_first(sp->uri, "StopPoint:", "");
    }
}

boost::gregorian::date_period GenericGtfsParser::basic_production_date(const std::string& beginning_date) {
    if (beginning_date == "") {
        throw UnableToFindProductionDateException();
    } else {
        boost::gregorian::date b_date(boost::gregorian::from_undelimited_string(beginning_date)),
            e_date(b_date + boost::gregorian::days(365 + 1));

        return {b_date, e_date};
    }
}

boost::gregorian::date_period GenericGtfsParser::find_production_date(const std::string& beginning_date) {
    std::string filename = path + "/stop_times.txt";
    CsvReader csv(filename, ',', true);
    if (!csv.is_open()) {
        LOG4CPLUS_FATAL(logger, "Impossible to find file " + filename);
        return basic_production_date(beginning_date);
    }

    std::vector<std::string> mandatory_headers = {"trip_id", "arrival_time", "departure_time", "stop_id",
                                                  "stop_sequence"};
    if (!csv.validate(mandatory_headers)) {
        LOG4CPLUS_FATAL(logger, "Error while reading "
                                    << filename << ". Columns missing: " << csv.missing_headers(mandatory_headers));
        return basic_production_date(beginning_date);
    }

    std::map<std::string, bool> trips;
    int trip_c = csv.get_pos_col("trip_id");
    while (!csv.eof()) {
        auto row = csv.next();
        if (!row.empty())
            trips.insert(std::make_pair(row[trip_c], true));
    }

    filename = path + "/trips.txt";
    CsvReader csv2(filename, ',', true);
    if (!csv2.is_open()) {
        LOG4CPLUS_FATAL(logger, "Impossible to find file " << filename);
        return basic_production_date(beginning_date);
    }

    mandatory_headers = {"trip_id", "service_id"};
    if (!csv2.validate(mandatory_headers)) {
        LOG4CPLUS_FATAL(logger, "Error while reading "
                                    << filename << ". Columns missing: " << csv.missing_headers(mandatory_headers));

        return basic_production_date(beginning_date);
    }
    int service_c = csv2.get_pos_col("service_id");
    trip_c = csv2.get_pos_col("trip_id");
    std::map<std::string, bool> services;
    while (!csv2.eof()) {
        auto row = csv2.next();
        if (!row.empty() && trips.find(row[trip_c]) != trips.end())
            services.insert(std::make_pair(row[service_c], true));
    }
    boost::gregorian::date start_date(boost::gregorian::max_date_time), end_date(boost::gregorian::min_date_time);

    filename = path + "/calendar.txt";
    CsvReader csv3(filename, ',', true);
    bool calendar_txt_exists = false;
    if (!csv3.is_open()) {
        LOG4CPLUS_WARN(logger, "impossible to find file " << filename);
    } else {
        calendar_txt_exists = true;
        mandatory_headers = {"start_date", "end_date", "service_id"};
        if (!csv3.validate(mandatory_headers)) {
            LOG4CPLUS_FATAL(logger, "Error while reading "
                                        << filename << ". Columns missing: " << csv.missing_headers(mandatory_headers));
            return basic_production_date(beginning_date);
        }

        int start_date_c = csv3.get_pos_col("start_date"), end_date_c = csv3.get_pos_col("end_date");
        service_c = csv3.get_pos_col("service_id");

        while (!csv3.eof()) {
            auto row = csv3.next();
            if (!row.empty()) {
                if (services.find(row[service_c]) != services.end()) {
                    boost::gregorian::date current_start_date =
                        boost::gregorian::from_undelimited_string(row[start_date_c]);
                    boost::gregorian::date current_end_date =
                        boost::gregorian::from_undelimited_string(row[end_date_c]);

                    if (current_start_date < start_date) {
                        start_date = current_start_date;
                    }
                    if (current_end_date > end_date) {
                        end_date = current_end_date;
                    }
                }
            }
        }
    }

    filename = path + "/calendar_dates.txt";
    CsvReader csv4(filename, ',', true);
    if (!csv4.is_open()) {
        if (calendar_txt_exists)
            LOG4CPLUS_WARN(logger, "impossible to find file " << filename);
        else
            LOG4CPLUS_WARN(logger, "impossible to find file " << filename << " and calendar.txt");
    } else {
        mandatory_headers = {"service_id", "date", "exception_type"};
        if (!csv4.validate(mandatory_headers)) {
            LOG4CPLUS_FATAL(logger, "Error while reading "
                                        << filename << ". Columns missing: " << csv.missing_headers(mandatory_headers));
            return basic_production_date(beginning_date);
        }
        int date_c = csv4.get_pos_col("date");
        service_c = csv4.get_pos_col("service_id");
        while (!csv4.eof()) {
            auto row = csv4.next();
            if (!row.empty() && services.find(row[service_c]) != services.end()) {
                boost::gregorian::date current_date = boost::gregorian::from_undelimited_string(row[date_c]);
                if (current_date < start_date) {
                    start_date = current_date;
                }
                if (current_date > end_date) {
                    end_date = current_date;
                }
            }
        }
    }
    return complete_production_date(beginning_date, start_date, end_date);
}

void GenericGtfsParser::manage_production_date(Data& data, const std::string& beginning_date) {
    auto start_it = data.feed_infos.find("feed_start_date");
    auto end_it = data.feed_infos.find("feed_end_date");
    boost::gregorian::date start_date(boost::gregorian::not_a_date_time), end_date(boost::gregorian::not_a_date_time);
    if ((start_it != data.feed_infos.end() && start_it->second != "")
        && (end_it != data.feed_infos.end() && end_it->second != "")) {
        try {
            start_date = boost::gregorian::from_undelimited_string(start_it->second);
        } catch (const std::exception& e) {
            LOG4CPLUS_WARN(logger, "manage_production_date, Unable to parse start_date :" << start_it->second
                                                                                          << ", error : " << e.what());
        }
        try {
            end_date = boost::gregorian::from_undelimited_string(end_it->second);
        } catch (const std::exception& e) {
            LOG4CPLUS_WARN(logger, "manage_production_date, Unable to parse end_date :" << end_it->second
                                                                                        << ", error : " << e.what());
        }
    } else {
        LOG4CPLUS_INFO(logger, "Unable to find production date in add_feed_info.");
    }
    if ((start_date.is_not_a_date()) || (end_date.is_not_a_date())) {
        try {
            data.meta.production_date = find_production_date(beginning_date);
        } catch (...) {
            if (beginning_date == "") {
                LOG4CPLUS_FATAL(logger, "Impossible to find the production date");
            }
        }
    } else {
        data.meta.production_date = complete_production_date(beginning_date, start_date, end_date);
    }
}

boost::gregorian::date_period GenericGtfsParser::complete_production_date(const std::string& beginning_date,
                                                                          boost::gregorian::date start_date,
                                                                          boost::gregorian::date end_date) {
    boost::gregorian::date b_date(boost::gregorian::min_date_time);
    if (beginning_date != "")
        b_date = boost::gregorian::from_undelimited_string(beginning_date);

    boost::gregorian::date beginning = std::max(start_date, b_date);

    // technically we cannot handle more than one year of data
    boost::gregorian::date end = std::min(end_date, beginning + boost::gregorian::days(365));

    LOG4CPLUS_INFO(logger, "date de production: " + boost::gregorian::to_simple_string(beginning) + " - "
                               + boost::gregorian::to_simple_string(end));
    // the end of a boost::gregorian::date_period is not in the period
    // since end_date is the last day is the data, we want the end to be the next day
    return {beginning, end + boost::gregorian::days(1)};
}

void GtfsParser::parse_files(Data& data, const std::string& beginning_date) {
    parse<FeedInfoGtfsHandler>(data, "feed_info.txt");

    manage_production_date(data, beginning_date);

    fill_default_modes(data);

    parse<ShapesGtfsHandler>(data, "shapes.txt");
    parse<AgencyGtfsHandler>(data, "agency.txt", true);
    parse<DefaultContributorHandler>(data);
    parse<StopsGtfsHandler>(data, "stops.txt", true);
    if (boost::filesystem::exists(this->path + "/pathways.txt")) {
        parse<PathWayGtfsHandler>(data, "pathways.txt", true);
    }
    parse<RouteGtfsHandler>(data, "routes.txt", true);
    parse<TransfersGtfsHandler>(data, "transfers.txt");

    parse<CalendarGtfsHandler>(data, "calendar.txt");
    parse<CalendarDatesGtfsHandler>(data, "calendar_dates.txt");
    // after the calendar load, we need to split the validitypattern
    split_validity_pattern_over_dst(data, gtfs_data);

    parse<TripsGtfsHandler>(data, "trips.txt", true);
    parse<StopTimeGtfsHandler>(data, "stop_times.txt", true);
    parse<FrequenciesGtfsHandler>(data, "frequencies.txt");
}

}  // namespace connectors
}  // namespace ed
