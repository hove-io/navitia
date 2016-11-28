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

#include "conf.h"

#include "utils/timer.h"
#include "utils/exception.h"
#include "ed_reader.h"
#include "type/data.h"
#include "utils/init.h"
#include "utils/functions.h"
#include "type/meta_data.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>
#include <pqxx/pqxx>
#include <iostream>
#include <fstream>

namespace po = boost::program_options;
namespace pt = boost::posix_time;
namespace georef = navitia::georef;

// A functor that first asks to GeoRef the admins of coord, and, if
// GeoRef found nothing, asks to the cities database.
struct FindAdminWithCities {
    typedef std::unordered_map<std::string, navitia::georef::Admin*> AdminMap;
    typedef std::vector<georef::Admin*> result_type;

    boost::shared_ptr<pqxx::connection> conn;
    georef::GeoRef& georef;
    AdminMap added_admins;
    AdminMap insee_admins_map;
    size_t nb_call = 0;
    size_t nb_uninitialized = 0;
    size_t nb_georef = 0;
    std::map<size_t, size_t> cities_stats;// number of response for size of the result

    FindAdminWithCities(const std::string& connection_string, georef::GeoRef& gr):
        conn(boost::make_shared<pqxx::connection>(connection_string)),
        georef(gr)
        {}

    FindAdminWithCities(const FindAdminWithCities&) = default;
    FindAdminWithCities& operator=(const FindAdminWithCities&) = default;
    ~FindAdminWithCities() {
        if (nb_call == 0) return;

        auto log = log4cplus::Logger::getInstance("ed2nav::FindAdminWithCities");
        LOG4CPLUS_INFO(log, "FindAdminWithCities: " << nb_call << " calls");
        LOG4CPLUS_INFO(log, "FindAdminWithCities: " << nb_uninitialized
                       << " calls with uninitialized or zeroed coord");
        LOG4CPLUS_INFO(log, "FindAdminWithCities: " << nb_georef << " GeoRef responses");
        LOG4CPLUS_INFO(log, "FindAdminWithCities: " << added_admins.size() << " admins added using cities");
        for (const auto& elt: cities_stats) {
            LOG4CPLUS_INFO(log, "FindAdminWithCities: "
                           << elt.second << " cities responses with "
                           << elt.first << " admins.");
        }
        for (const auto& admin: added_admins) {
            LOG4CPLUS_INFO(log, "FindAdminWithCities: "
                           << "We have added the following admin: "
                           << admin.second->label << " insee: "
                           << admin.second->insee << " uri: "
                           << admin.second->uri);
        }
    }

    void init(){
        for(auto* admin: georef.admins){
            if(!admin->insee.empty()){
                insee_admins_map[admin->insee] = admin;
            }
        }
    }

    result_type operator()(const navitia::type::GeographicalCoord& c) {
        if(nb_call == 0){
            init();
        }
        ++nb_call;

        if (!c.is_initialized()) {++nb_uninitialized; return {};}

        const auto &georef_res = georef.find_admins(c);
        if (!georef_res.empty()) {++nb_georef; return georef_res;}

        std::stringstream request;
        request << "SELECT uri, name, insee, level, post_code, "
                << "ST_X(coord::geometry) as lon, ST_Y(coord::geometry) as lat "
                << "FROM administrative_regions "
                << "WHERE ST_DWithin(ST_GeographyFromText('POINT("
                << std::setprecision(16) << c.lon() << " " << c.lat() << ")'), boundary, 0.001)";
        pqxx::work work(*conn);
        pqxx::result result = work.exec(request);
        result_type res;
        for (auto it = result.begin(); it != result.end(); ++it) {
            const std::string uri = it["uri"].as<std::string>() + "extern";
            const std::string insee = it["insee"].as<std::string>();
            //we try to find the admin in georef by using it's insee code (only work in France)
            navitia::georef::Admin* admin = nullptr;
            if (!insee.empty()) { admin = find_or_default(insee, insee_admins_map);}
            if (!admin) { admin = find_or_default(uri, added_admins);}
            if (!admin) {
                georef.admins.push_back(new navitia::georef::Admin());
                admin = georef.admins.back();
                admin->comment = "from cities";
                admin->uri = uri;
                it["name"].to(admin->name);
                admin->insee = insee;
                it["level"].to(admin->level);
                admin->coord.set_lon(it["lon"].as<double>());
                admin->coord.set_lat(it["lat"].as<double>());
                admin->idx = georef.admins.size() - 1;
                admin->from_original_dataset = false;
                std::string postal_code;
                it["post_code"].to(postal_code);

                if(!postal_code.empty()){
                    boost::split(admin->postal_codes, postal_code, boost::is_any_of("-"));
                }
                added_admins[uri] = admin;
            }
            res.push_back(admin);
        }
        ++cities_stats[res.size()];
        return res;
    }
};

int main(int argc, char * argv[])
{
    navitia::init_app();
    auto logger = log4cplus::Logger::getInstance("log");
    std::string output, connection_string, region_name, cities_connection_string;
    double min_non_connected_graph_ratio;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Show this message")
        ("version,v", "Show version")
        ("config-file", po::value<std::string>(), "Path to config file")
        ("output,o", po::value<std::string>(&output)->default_value("data.nav.lz4"),
            "Output file")
        ("name,n", po::value<std::string>(&region_name)->default_value("default"),
            "Name of the region you are extracting")
        ("min_non_connected_ratio,m",
         po::value<double>(&min_non_connected_graph_ratio)->default_value(0.1),
         "min ratio for the size of non connected graph")
        ("full_street_network_geometries", "If true export street network geometries allowing kraken to return accurate"
         "geojson for street network sections. Also improve projections accuracy. "
         "WARNING : memory intensive. The lz4 can more than double in size and kraken will consume significantly more memory.")
        ("connection-string", po::value<std::string>(&connection_string)->required(),
         "database connection parameters: host=localhost user=navitia dbname=navitia password=navitia")
        ("cities-connection-string", po::value<std::string>(&cities_connection_string)->default_value(""),
         "cities database connection parameters: host=localhost user=navitia dbname=cities password=navitia");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    bool export_georef_edges_geometries(vm.count("full_street_network_geometries"));

    if(vm.count("version")){
        std::cout << argv[0] << " " << navitia::config::project_version << " "
                  << navitia::config::navitia_build_type << std::endl;
        return 0;
    }


    if(vm.count("config-file")){
        std::ifstream stream;
        stream.open(vm["config-file"].as<std::string>());
        if(!stream.is_open()){
            throw navitia::exception("Unable to load config file");
        }else{
            po::store(po::parse_config_file(stream, desc), vm);
        }
    }

    if(vm.count("help")) {
        std::cout << "Extracts data from a database to a file readable by kraken" << std::endl;
        std::cout << desc <<  std::endl;
        return 1;
    }

    po::notify(vm);

    pt::ptime start, now;
    int read, save;

    navitia::type::Data data;

    //on init now pour le moment à now, à rendre paramétrable pour le debug
    now = start = pt::microsec_clock::local_time();

    ed::EdReader reader(connection_string);

    if (!cities_connection_string.empty()) {
        data.find_admins = FindAdminWithCities(cities_connection_string, *data.geo_ref);
    }

    try {
        reader.fill(data, min_non_connected_graph_ratio, export_georef_edges_geometries);
    }
    catch (const navitia::exception& e) {
        LOG4CPLUS_ERROR(logger, "error while reading the database "  << e.what());
        LOG4CPLUS_ERROR(logger, "stack: "  << e.backtrace());
        throw;
    }

    read = (pt::microsec_clock::local_time() - start).total_milliseconds();
    data.complete();
    data.meta->publication_date = pt::microsec_clock::local_time();

    LOG4CPLUS_INFO(logger, "line: " << data.pt_data->lines.size());
    LOG4CPLUS_INFO(logger, "line_groups: " << data.pt_data->line_groups.size());
    LOG4CPLUS_INFO(logger, "route: " << data.pt_data->routes.size());
    LOG4CPLUS_INFO(logger, "stoparea: " << data.pt_data->stop_areas.size());
    LOG4CPLUS_INFO(logger, "stoppoint: " << data.pt_data->stop_points.size());
    LOG4CPLUS_INFO(logger, "vehiclejourney: " << data.pt_data->vehicle_journeys.size());
    LOG4CPLUS_INFO(logger, "stop: " << data.pt_data->nb_stop_times());
    LOG4CPLUS_INFO(logger, "connection: " << data.pt_data->stop_point_connections.size());
    LOG4CPLUS_INFO(logger, "modes: " << data.pt_data->physical_modes.size());
    LOG4CPLUS_INFO(logger, "validity pattern : " << data.pt_data->validity_patterns.size());
    LOG4CPLUS_INFO(logger, "calendars: " << data.pt_data->calendars.size());
    LOG4CPLUS_INFO(logger, "synonyms : " << data.geo_ref->synonyms.size());
    LOG4CPLUS_INFO(logger, "fare tickets: " << data.fare->fare_map.size());
    LOG4CPLUS_INFO(logger, "fare transitions: " << data.fare->nb_transitions());
    LOG4CPLUS_INFO(logger, "fare od: " << data.fare->od_tickets.size());
    LOG4CPLUS_INFO(logger, "Begin to save ...");

    start = pt::microsec_clock::local_time();
    try {
        data.save(output);
    } catch(const navitia::exception &e) {
        LOG4CPLUS_ERROR(logger, "Unable to save");
        LOG4CPLUS_ERROR(logger, e.what());
        return 1;
    }
    save = (pt::microsec_clock::local_time() - start).total_milliseconds();
    LOG4CPLUS_INFO(logger, "Data saved");

    LOG4CPLUS_INFO(logger, "Computing times");
    LOG4CPLUS_INFO(logger, "\t File reading: " << read << "ms");
    LOG4CPLUS_INFO(logger, "\t Data writing: " << save << "ms");

    return 0;
}
