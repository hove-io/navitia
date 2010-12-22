#include "gtfs_parser.h"
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <fstream>
#include <iostream>
#include <deque>

typedef boost::tokenizer< boost::escaped_list_separator<char> > Tokenizer;

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

GtfsParser::GtfsParser(const std::string & path, const std::string & start) :
        path(path),
        start(boost::gregorian::from_undelimited_string(start))
{
    std::cout << this->start << std::endl;
    parse_stops();
    parse_calendar_dates();
    parse_routes();
    parse_trips();
    parse_stop_times();
}

void GtfsParser::save(const std::string & filename) {
    std::ofstream ofs(filename.c_str());
    boost::archive::text_oarchive oa(ofs);
    oa << *this;
}

void GtfsParser::load(const std::string & filename) {
    std::ifstream ifs(filename.c_str());
    boost::archive::text_iarchive ia(ifs);
    ia >> *this;
}

void GtfsParser::save_bin(const std::string & filename) {
    std::ofstream ofs(filename.c_str(),  std::ios::out | std::ios::binary);
    boost::archive::binary_oarchive oa(ofs);
    oa << *this;
}

void GtfsParser::load_bin(const std::string & filename) {
    std::ifstream ifs(filename.c_str(),  std::ios::in | std::ios::binary);
    boost::archive::binary_iarchive ia(ifs);
    ia >> *this;
}

void GtfsParser::parse_stops() {
    stop_points.reserve(56000);
    stop_areas.reserve(13000);
    // En GTFS les stopPoint et stopArea sont définis en une seule fois
    // On garde donc le "parent station" (aka. stopArea) dans un tableau lors de la 1ère passe.
    std::deque<std::pair<int, std::string> > stoppoint_areas;

    std::cout << "On parse : " << (path + "/stops.txt").c_str() << std::endl;
    std::fstream ifile((path + "/stops.txt").c_str());
    std::string line;
    if(!getline(ifile, line)) {
        std::cerr << "Impossible d'ouvrir stops.txt" << std::endl;
        return;
    }

    boost::trim(line);
    Tokenizer tok_header(line);
    std::vector<std::string> elts(tok_header.begin(), tok_header.end());
    BOOST_ASSERT(elts[0] == "stop_id");
    BOOST_ASSERT(elts[1] == "stop_code");
    BOOST_ASSERT(elts[2] == "stop_name");
    BOOST_ASSERT(elts[3] == "stop_desc");
    BOOST_ASSERT(elts[4] == "stop_lat");
    BOOST_ASSERT(elts[5] == "stop_lon");
    BOOST_ASSERT(elts[6] == "zone_id");
    BOOST_ASSERT(elts[7] == "stop_url");
    BOOST_ASSERT(elts[8] == "location_type");
    BOOST_ASSERT(elts[9] == "parent_station");

    int ignored = 0;

    while(getline(ifile, line)) {
        boost::trim(line);
        Tokenizer tok(line);
        elts.assign(tok.begin(), tok.end());
        StopPoint sp;
        try{
            sp.coord.x = boost::lexical_cast<double>(elts[4]);
            sp.coord.y = boost::lexical_cast<double>(elts[5]);
        }
        catch(boost::bad_lexical_cast ) {
            std::cerr << "Impossible de parser les coordonnées pour " << elts[0] << " " << elts[1]
                    << " " << elts[2] << std::endl;
        }

        sp.name = elts[2];
        if(elts[1] != "")
            sp.code = elts[1];
        else
            sp.code = elts[0];

        if(!stop_points_map.empty() && stop_points_map.find(sp.code) != stop_points_map.end()) {
            ignored++;
        }
        else {
            stop_points_map[sp.code] = stop_points.size();
            stop_points.push_back(sp);
            if(elts[9] != "")
                stoppoint_areas.push_back(std::make_pair(stop_points.size(), elts[9]));

            // Si c'est un stopArea
            if(elts[8] == "1") {
                StopArea sa;
                sa.coord = sp.coord;
                sa.name = sp.name;
                sa.code = sp.code;
                stop_areas_map[sa.code] =  stop_areas.size();
                stop_areas.push_back(sa);
            }
        }
    }

    // On reboucle pour récupérer les stop areas de tous les stop points
    BOOST_FOREACH(auto i, stoppoint_areas){
        auto sa = stop_areas_map.find(i.second);
        if(sa != stop_areas_map.end())
            stop_points[i.first].stop_area_idx = sa->second;
        else
            std::cerr << "Le stopPoint " << stop_points[i.first].code
                    << " a utilisé un stopArea inconnu : " << i.second << std::endl;
    }

    BOOST_ASSERT(stop_points.size() == stop_points_map.size());
    BOOST_ASSERT(stop_areas.size() == stop_areas_map.size());
    std::cout << "J'ai parsé " << stop_points.size() << " stop points" << std::endl;
    std::cout << "J'ai parsé " << stop_areas.size() << " stop areas" << std::endl;
    std::cout << "J'ai ignoré " << ignored << " points à cause de doublons" << std::endl;
    std::cout << std::endl;
}

void GtfsParser::parse_calendar_dates(){
    validity_patterns.reserve(10000);
    std::cout << "On parse : " << (path + "/calendar_dates.txt").c_str() << std::endl;
    std::fstream ifile((path + "/calendar_dates.txt").c_str());
    std::string line;
    if(!getline(ifile, line)) {
        std::cerr << "Impossible d'ouvrir le fichier calendar_dates.txt" << std::endl;
        return;
    }

    boost::trim(line);
    Tokenizer tok_header(line);
    std::vector<std::string> elts(tok_header.begin(), tok_header.end());
    BOOST_ASSERT(elts[0] == "service_id");
    BOOST_ASSERT(elts[1] == "date");
    BOOST_ASSERT(elts[2] == "exception_type");

    while(getline(ifile, line)) {
        boost::trim(line);
        Tokenizer tok(line);
        elts.assign(tok.begin(), tok.end());

        int idx;
        if(validity_patterns_map.find(elts[0]) == validity_patterns_map.end()){
            idx = validity_patterns.size();
            validity_patterns.push_back(ValidityPattern(start));
            validity_patterns_map[elts[0]] = idx;
        }
        else {
            idx = validity_patterns_map[elts[0]];
        }

        auto date = boost::gregorian::from_undelimited_string(elts[1]);
        if(elts[2] == "1")
            validity_patterns[idx].add(date);
        else if(elts[2] == "2")
            validity_patterns[idx].remove(date);
        else
            std::cerr << "Exception pour le service " << elts[0] << " inconnue : " << elts[2] << std::endl;
    }
    BOOST_ASSERT(validity_patterns.size() == validity_patterns_map.size());
    std::cout << "Nombre de validity patterns : " << validity_patterns.size() << std::endl;
}

void GtfsParser::parse_routes(){
    lines.reserve(10000);
    std::cout << "On parse : " << (path + "/routes.txt").c_str() << std::endl;
    std::fstream ifile((path + "/routes.txt").c_str());
    std::string line;
    if(!getline(ifile, line)) {
        std::cerr << "Impossible d'ouvrir le fichier routes.txt" << std::endl;
        return;
    }

    boost::trim(line);
    Tokenizer tok_header(line);
    std::vector<std::string> elts(tok_header.begin(), tok_header.end());
    BOOST_ASSERT(elts[0] == "route_id");
    BOOST_ASSERT(elts[1] == "agency_id");
    BOOST_ASSERT(elts[2] == "route_short_name");
    BOOST_ASSERT(elts[3] == "route_long_name");
    BOOST_ASSERT(elts[4] == "route_desc");
    BOOST_ASSERT(elts[5] == "route_type");
    BOOST_ASSERT(elts[6] == "route_url");
    BOOST_ASSERT(elts[7] == "route_color");
    BOOST_ASSERT(elts[8] == "route_text_color");

    int ignored = 0;
    while(getline(ifile, line)) {
        boost::trim(line);
        Tokenizer tok(line);
        elts.assign(tok.begin(), tok.end());

        if(lines.empty() || lines_map.find(elts[0]) == lines_map.end()) {
            Line line;
            line.name = elts[3];
            line.code = elts[2];
            line.mode = elts[5];
            ///line->network =  networks[elts[1]];
            line.color = elts[7];
            line.additional_data = elts[3];

            lines_map[elts[0]] = lines.size();
            lines.push_back(line);
        }
        else {
            ignored++;
        }
    }
    BOOST_ASSERT(lines.size() == lines_map.size());
    std::cout << "Nombre de lignes : " << lines.size() << std::endl;
    std::cout << "J'ai ignoré " << ignored << " lignes pour cause de doublons" << std::endl;
    std::cout << std::endl;
}

void GtfsParser::parse_trips() {
    routes.reserve(350000);
    vehicle_journeys.reserve(350000);
    std::cout << "On parse : " << (path + "/trips.txt").c_str() << std::endl;
    std::fstream ifile((path + "/trips.txt").c_str());
    std::string line;
    if(!getline(ifile, line)) {
        std::cerr << "Impossible d'ouvrir le fichier trips.txt" << std::endl;
        return;
    }

    boost::trim(line);
    Tokenizer tok_header(line);
    std::vector<std::string> elts(tok_header.begin(), tok_header.end());
    BOOST_ASSERT(elts[0] == "route_id");
    BOOST_ASSERT(elts[1] == "service_id");
    BOOST_ASSERT(elts[2] == "trip_id");
    BOOST_ASSERT(elts[3] == "trip_headsign");
    BOOST_ASSERT(elts[4] == "direction_id");
    BOOST_ASSERT(elts[5] == "block_id");
    BOOST_ASSERT(elts[6] == "shape_id");
    int ignored = 0;
    int ignored_vj = 0;
    while(getline(ifile, line)) {
        boost::trim(line);
        Tokenizer tok(line);
        elts.assign(tok.begin(), tok.end());

        if(lines_map.find(elts[0]) == lines_map.end()){
            std::cerr << "Impossible de trouver la Route (au sens GTFS) " << elts[0]
                    << " référencée par trip " << elts[2] << std::endl;
        }
        else {
            int line_idx = lines_map[elts[0]];
            Route route;
            route.line_idx = line_idx;

            if(validity_patterns_map.find(elts[1]) == validity_patterns_map.end()) {
                ignored++;
            }
            else {
                lines[line_idx].validity_pattern_list.push_back(validity_patterns_map[elts[1]]);
            }
            routes_map[elts[2]] = routes.size();
            routes.push_back(route);

            if(vehicle_journeys.empty() || vehicle_journeys_map.find(elts[3]) == vehicle_journeys_map.end()) {
                VehicleJourney vj;
                vj.route_idx = routes.size() - 1;
                //vj->company = route->line->network;
                vj.name = elts[3];
                vj.external_code = elts[2];
                //vj->mode = route->mode_type;
                vj.validity_pattern_idx = validity_patterns_map[elts[1]];

                vehicle_journeys_map[vj.name] = vehicle_journeys.size();
                vehicle_journeys.push_back(vj);
            }
            else {
                ignored_vj++;
            }
        }
    }
    BOOST_ASSERT(routes.size() == routes_map.size());
    BOOST_ASSERT(vehicle_journeys.size() == vehicle_journeys_map.size());
    std::cout << "Nombre de routes : " << routes.size() << std::endl;
    std::cout << "Nombre de vehicle journeys : " << vehicle_journeys.size() << std::endl;
    std::cout << "Nombre d'erreur de référence de service : " << ignored << std::endl;
    std::cout << "J'ai ignoré " << ignored_vj << " vehicule journey à cause de doublons" << std::endl;
    std::cout << std::endl;
}

void GtfsParser::parse_stop_times() {
    stop_times.reserve(8000000);
    std::cout << "On parse : " << (path + "/stop_times.txt").c_str() << std::endl;
    std::fstream ifile((path + "/stop_times.txt").c_str());
    std::string line;
    if(!getline(ifile, line)) {
        std::cerr << "Impossible d'ouvrir le fichier stop_times.txt" << std::endl;
        return;
    }

    boost::trim(line);
    Tokenizer tok_header(line);
    std::vector<std::string> elts(tok_header.begin(), tok_header.end());
    BOOST_ASSERT(elts[0] == "trip_id");
    BOOST_ASSERT(elts[1] == "arrival_time");
    BOOST_ASSERT(elts[2] == "departure_time");
    BOOST_ASSERT(elts[3] == "stop_id");
    BOOST_ASSERT(elts[4] == "stop_sequence");
    BOOST_ASSERT(elts[5] == "stop_headsign");
    BOOST_ASSERT(elts[6] == "pickup_type");
    BOOST_ASSERT(elts[7] == "drop_off_type");
    BOOST_ASSERT(elts[8] == "shape_dist_traveled");

    while(getline(ifile, line)) {
        boost::trim(line);
        Tokenizer tok(line);
        elts.assign(tok.begin(), tok.end());
        if(routes_map.find(elts[0]) == routes_map.end()) {
            std::cerr << "Impossible de trouver la route " << elts[0] << std::endl;
        }
        else if(stop_points_map.find(elts[3]) == stop_points_map.end()){
            std::cerr << "Impossible le StopPoint " << elts[3] << std::endl;
        }
        else {
            StopTime stop_time;
            stop_time.arrival_time = time_to_int(elts[1]);
            stop_time.departure_time = time_to_int(elts[2]);
            stop_time.stop_point_idx = stop_points_map[elts[3]];
            stop_time.order = boost::lexical_cast<int>(elts[4]);
            stop_time.vehicle_journey_idx = vehicle_journeys_map[elts[0]];
            stop_time.ODT = (elts[6] == "2" && elts[7] == "2");
            stop_time.zone = 0; // à définir selon pickup_type ou drop_off_type = 10
            stop_times.push_back(stop_time);
        }
    }

    std::cout << "Nombre d'horaires : " << stop_times.size() << std::endl;

}
