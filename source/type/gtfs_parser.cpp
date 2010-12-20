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

GtfsParser::GtfsParser(const std::string & path, const std::string & start) :
        path(path),
        start(boost::gregorian::from_undelimited_string(start))
{
    parse_stops();
    parse_calendar_dates();
    parse_routes();
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
        sp->external_code = elts[0];

        if(elts[9] != "")
            stoppoint_areas.push_back(std::make_pair(sp, elts[9]));

        stop_points[sp->external_code] = sp;

        // Si c'est un stopArea
        if(elts[8] == "1") {
            StopArea_ptr sa(new StopArea());
            sa->coord = sp->coord;
            sa->name = sp->name;
            sa->external_code = sp->external_code;
            stop_areas[(sa->external_code)] =  sa;
        }
    }

    // On reboucle pour récupérer les stop areas de tous les stop points
    BOOST_FOREACH(auto i, stoppoint_areas){
        auto sa = stop_areas.find(i.second);
        if(sa != stop_areas.end())
            (i.first)->stop_area = sa->second;
        else
            std::cerr << "Le stopPoint " << (i.first)->external_code
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
        line->color = elts[7];
        line->name = elts[2];
        line->additional_data = elts[3];
        line->external_code = elts[0];

        lines[line->external_code] = line;

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

    while(getline(ifile, line)) {
        boost::trim(line);
        Tokenizer tok(line);
        elts.assign(tok.begin(), tok.end());

        if(lines.find(elts[0]) == lines.end()){
            std::cerr << "Impossible de trouver la Route (au sens GTFS) " << elts[0]
                    << " référencée par trip " << elts[2] << std::endl;
        }
        else {
            Route_ptr route(new Route());
            route->line = lines[elts[0]];
            if(validity_patterns.find(elts[1]) == validity_patterns.end()) {
                std::cerr << "Impossible de trouvé le service " << elts[1] << " référencé par trip " << elts[2] << std::endl;
            }
            else {
               route->vehicle_journey_list.push_back(validity_patterns[elts[1]]);
           }
            routes[elts[0] + elts[2]] = route;
        }
    }
}
