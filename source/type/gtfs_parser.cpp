#include "gtfs_parser.h"
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
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
    parse_stops();
    parse_calendar_dates();
    parse_routes();
    parse_trips();
    parse_stop_times();
}

void GtfsParser::parse_stops() {
    // En GTFS les stopPoint et stopArea sont définis en une seule fois
    // On garde donc le "parent station" (aka. stopArea) dans un tableau lors de la 1ère passe.
    std::deque<std::pair<StopPoint_ptr, std::string> > stoppoint_areas;

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

    while(getline(ifile, line)) {
        boost::trim(line);
        Tokenizer tok(line);
        elts.assign(tok.begin(), tok.end());
        StopPoint_ptr sp(new StopPoint());
        try{
            sp->coord.x = boost::lexical_cast<double>(elts[4]);
            sp->coord.y = boost::lexical_cast<double>(elts[5]);
        }
        catch(boost::bad_lexical_cast ex) {
            std::cerr << "Impossible de parser les coordonnées pour " << elts[0] << " " << elts[1]
                    << " " << elts[2] << std::endl;
        }

        sp->name = elts[2];
        if(elts[1] != "")
            sp->code = elts[1];
        else
            sp->code = elts[0];

        if(elts[9] != "")
            stoppoint_areas.push_back(std::make_pair(sp, elts[9]));

        stop_points[sp->code] = sp;

        // Si c'est un stopArea
        if(elts[8] == "1") {
            StopArea_ptr sa(new StopArea());
            sa->coord = sp->coord;
            sa->name = sp->name;
            sa->code = sp->code;
            stop_areas[(sa->code)] =  sa;
        }
    }

    // On reboucle pour récupérer les stop areas de tous les stop points
    BOOST_FOREACH(auto i, stoppoint_areas){
        auto sa = stop_areas.find(i.second);
        if(sa != stop_areas.end())
            (i.first)->stop_area = sa->second;
        else
            std::cerr << "Le stopPoint " << (i.first)->code
                    << " a utilisé un stopArea inconnu : " << (i.second) << std::endl;
    }

    std::cout << "J'ai parsé " << stop_points.size() << " stop points" << std::endl;
    std::cout << "J'ai parsé " << stop_areas.size() << " stop areas" << std::endl;
}

void GtfsParser::parse_calendar_dates(){
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

        if(validity_patterns.find(elts[0]) == validity_patterns.end()){
            validity_patterns[elts[0]] = ValidityPattern_ptr(new ValidityPattern(start));
        }
        auto date = boost::gregorian::from_undelimited_string(elts[1]);
        if(elts[2] == "1")
            validity_patterns[elts[0]]->add(date);
        else if(elts[2] == "2")
            validity_patterns[elts[0]]->remove(date);
        else
            std::cerr << "Exception pour le service " << elts[0] << " inconnue : " << elts[2] << std::endl;
    }
    std::cout << "Nombre de validity patterns : " << validity_patterns.size() << std::endl;
}

void GtfsParser::parse_routes(){
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

    while(getline(ifile, line)) {
        boost::trim(line);
        Tokenizer tok(line);
        elts.assign(tok.begin(), tok.end());

        Line_ptr line(new Line());
        line->name = elts[3];
        line->code = elts[2];
        line->mode = elts[5];
        ///line->network =  networks[elts[1]];
        line->color = elts[7];
        line->additional_data = elts[3];

        lines[elts[0]] = line;

    }
    std::cout << "Nombre de lignes : " << lines.size() << std::endl;
}

void GtfsParser::parse_trips() {
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
    while(getline(ifile, line)) {
        boost::trim(line);
        Tokenizer tok(line);
        elts.assign(tok.begin(), tok.end());

        if(lines.find(elts[0]) == lines.end()){
            std::cerr << "Impossible de trouver la Route (au sens GTFS) " << elts[0]
                    << " référencée par trip " << elts[2] << std::endl;
        }
        else {
            Line_ptr line = lines[elts[0]];
            Route_ptr route(new Route());
            route->line = line;

            if(validity_patterns.find(elts[1]) == validity_patterns.end()) {
                std::cerr << "Impossible de trouver le service " << elts[1] << " référencé par trip " << elts[2] << std::endl;
                ignored++;
            }
            else {
                line->validity_pattern_list.push_back(validity_patterns[elts[1]]);
            }
            routes[elts[2]] = route;

            VehicleJourney_ptr vj(new VehicleJourney());
            vj->route = route;
            //vj->company = route->line->network;
            vj->name = elts[3];
            vj->external_code = elts[2];
            //vj->mode = route->mode_type;
            vj->validity_pattern = validity_patterns[elts[1]];
            vehicle_journeys[vj->name] = vj;
        }
    }
    std::cout << "Nombre de routes : " << routes.size() << std::endl;
    std::cout << "Nombre de vehicle journeys : " << vehicle_journeys.size() << std::endl;
    std::cout << "Nombre d'erreur de référence de service : " << ignored << std::endl;
}

void GtfsParser::parse_stop_times() {
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
        if(routes.find(elts[0]) == routes.end()) {
            std::cerr << "Impossible de trouver la route " << elts[0] << std::endl;
        }
        else if(stop_points.find(elts[3]) == stop_points.end()){
            std::cerr << "Impossible le StopPoint " << elts[3] << std::endl;
        }
        else {
            StopTime_ptr stop_time(new StopTime());
            stop_time->arrival_time = time_to_int(elts[1]);
            stop_time->departure_time = time_to_int(elts[2]);
            stop_time->stop_point = stop_points[elts[3]];
            stop_time->order = boost::lexical_cast<int>(elts[4]);
            stop_time->vehicle_journey = vehicle_journeys[elts[0]];
            stop_time->ODT = (elts[6] == "2" && elts[7] == "2");
            stop_time->zone = 0; // à définir selon pickup_type ou drop_off_type = 10
            stop_times.push_back(stop_time);
        }
    }
    std::cout << "Nombre d'horaires : " << stop_times.size() << std::endl;

}
