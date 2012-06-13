#include "gtfs_parser.h"
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <fstream>
#include <iostream>
#include <deque>
#include "utils/encoding_converter.h"

namespace nm = navimake::types;
typedef boost::tokenizer< boost::escaped_list_separator<char> > Tokenizer;

namespace navimake{ namespace connectors {


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
    start(boost::gregorian::from_undelimited_string(start)) {
    std::cout << "Date de début : " << this->start << std::endl;
}

void GtfsParser::fill(Data & data){
    fill_mode_types(data);
    parse_stops(data);
    parse_calendar(data);
    parse_calendar_dates(data);
    parse_routes(data);
    parse_trips(data);
    parse_stop_times(data);
    build_routes(data);
    build_route_points(data);
}

void GtfsParser::fill_mode_types(Data & data) {
    navimake::types::ModeType* mode_type = new navimake::types::ModeType();
    mode_type->id = "0";
    mode_type->name = "Tram";
    mode_type->external_code = "0x0";
    data.mode_types.push_back(mode_type);
    mode_type_map[mode_type->id] = mode_type;

    mode_type = new navimake::types::ModeType();
    mode_type->id = "1";
    mode_type->name = "Metro";
    mode_type->external_code = "0x1";
    data.mode_types.push_back(mode_type);
    mode_type_map[mode_type->id] = mode_type;

    mode_type = new navimake::types::ModeType();
    mode_type->id = "2";
    mode_type->name = "Rail";
    mode_type->external_code = "0x2";
    data.mode_types.push_back(mode_type);
    mode_type_map[mode_type->id] = mode_type;

    mode_type = new navimake::types::ModeType();
    mode_type->id = "3";
    mode_type->name = "Bus";
    mode_type->external_code = "0x3";
    data.mode_types.push_back(mode_type);
    mode_type_map[mode_type->id] = mode_type;

    mode_type = new navimake::types::ModeType();
    mode_type->id = "4";
    mode_type->name = "Ferry";
    mode_type->external_code = "0x4";
    data.mode_types.push_back(mode_type);
    mode_type_map[mode_type->id] = mode_type;

    mode_type = new navimake::types::ModeType();
    mode_type->id = "5";
    mode_type->name = "Cable car";
    mode_type->external_code = "0x5";
    data.mode_types.push_back(mode_type);
    mode_type_map[mode_type->id] = mode_type;

    mode_type = new navimake::types::ModeType();
    mode_type->id = "6";
    mode_type->name = "Gondola";
    mode_type->external_code = "0x6";
    data.mode_types.push_back(mode_type);
    mode_type_map[mode_type->id] = mode_type;

    mode_type = new navimake::types::ModeType();
    mode_type->id = "7";
    mode_type->name = "Funicular";
    mode_type->external_code = "0x7";
    data.mode_types.push_back(mode_type);
    mode_type_map[mode_type->id] = mode_type;


    BOOST_FOREACH(navimake::types::ModeType *mt, data.mode_types) {
        navimake::types::Mode* mode = new navimake::types::Mode();
        mode->id = mt->id;
        mode->name = mt->name;
        mode->external_code = mt->external_code;
        mode->mode_type = mt;
        data.modes.push_back(mode);
        mode_map[mode->id] = mode;
    }


}

void GtfsParser::parse_stops(Data & data) {
    // On réserve un peu large, de l'ordre de l'Île-de-France
    data.stop_points.reserve(56000);
    data.stop_areas.reserve(13000);
    // En GTFS les stopPoint et stopArea sont définis en une seule fois
    // On garde donc le "parent station" (aka. stopArea) dans un tableau lors de la 1ère passe.
    std::deque<std::pair<nm::StopPoint*, std::string> > stoppoint_areas;

    std::cout << "On parse : " << (path + "/stops.txt").c_str() << std::endl;
    std::fstream ifile((path + "/stops.txt").c_str());
    remove_bom(ifile);
    std::string line;
    if(!getline(ifile, line)) {
        std::cerr << "Impossible d'ouvrir stops.txt" << std::endl;
        return;
    }

    // colonne que l'on va utiliser
    int id_c = -1, code_c = -1, lat_c = -1, lon_c = -1, type_c = -1, parent_c = -1, name_c = -1;

    boost::trim(line);
    Tokenizer tok_header(line);
    std::vector<std::string> elts(tok_header.begin(), tok_header.end());
    for(size_t i = 0; i < elts.size(); i++){
        if(elts[i] == "stop_id")
            id_c = i;
        else if (elts[i] == "stop_code")
            code_c = i;
        else if (elts[i] == "stop_name")
            name_c = i;
        else if (elts[i] == "stop_lat")
            lat_c = i;
        else if (elts[i] == "stop_lon")
            lon_c = i;
        else if (elts[i] == "location_type")
            type_c = i;
        else if (elts[i] ==  "parent_station")
            parent_c = i;
    }
    if(code_c == -1){
        code_c = id_c;
    }

    if(id_c == -1 || code_c == -1 || lat_c == -1 || lon_c == -1 || name_c == -1) {
        std::cerr << "Il manque au moins une colonne dans le fichier stops.txt" << std::endl;
        return;
    }

    int ignored = 0;

    while(getline(ifile, line)) {
        boost::trim(line);
        Tokenizer tok(line);
        elts.assign(tok.begin(), tok.end());
        nm::StopPoint * sp = new nm::StopPoint();
        try{
            sp->coord.x = boost::lexical_cast<double>(elts[lon_c]);
            sp->coord.y = boost::lexical_cast<double>(elts[lat_c]);
        }
        catch(boost::bad_lexical_cast ) {
            std::cerr << "Impossible de parser les coordonnées pour " << elts[id_c] << " " << elts[code_c]
                      << " " << elts[name_c] << std::endl;
        }

        sp->name = elts[name_c];
        sp->external_code = elts[id_c];

        if(!data.stop_points.empty() && stop_map.find(sp->external_code) != stop_map.end()) {
            ignored++;
        }
        else {


            // Si c'est un stopArea
            if(elts[type_c] == "1") {
                nm::StopArea * sa = new nm::StopArea();
                sa->coord = sp->coord;
                sa->name = sp->name;
                sa->external_code = sp->external_code;
                stop_area_map[sa->external_code] = sa;
                data.stop_areas.push_back(sa);
                delete sp;
            }
            // C'est un StopPoint
            else {
                stop_map[sp->external_code] = sp;
                data.stop_points.push_back(sp);
                if(parent_c < elts.size() && elts[parent_c] != "") ///On sauvegarde la référence à la zone d'arrêt
                    stoppoint_areas.push_back(std::make_pair(sp, elts[parent_c]));
            }

        }
    }


    // On reboucle pour récupérer les stop areas de tous les stop points
    BOOST_FOREACH(auto sa, stoppoint_areas){
        auto it = stop_area_map.find(sa.second);
        if(it != stop_area_map.end()) {
            (sa.first)->stop_area = it->second;
        }
        else{
            std::cerr << "Le stopPoint " << (sa.first)->external_code
                      << " a utilisé un stopArea inconnu : " << sa.second << std::endl;
            (sa.first)->stop_area = 0;
        }
    }



    std::cout << "J'ai parsé " << data.stop_points.size() << " stop points" << std::endl;
    std::cout << "J'ai parsé " << data.stop_areas.size() << " stop areas" << std::endl;
    std::cout << "J'ai ignoré " << ignored << " points à cause de doublons" << std::endl;
    std::cout << std::endl;
}


void GtfsParser::parse_calendar(Data & data) {
    data.validity_patterns.reserve(10000);

    std::cout << "On parse : " << (path + "calendar.txt").c_str() << std::endl;
    std::fstream ifile((path + "calendar.txt").c_str());
    remove_bom(ifile);
    std::string line;

    if(!getline(ifile, line)){
        std::cerr << "Impossible d'ouvrir le fichier calendar.txt" << std::endl;
        return;
    }


    boost::trim(line);
    Tokenizer tok_header(line);
    std::vector<std::string> elts(tok_header.begin(), tok_header.end());
    int id_c = -1, monday_c = -1, tuesday_c = -1, wednesday_c = -1, thursday_c = -1, friday_c = -1, saturday_c = -1, sunday_c = -1, start_date_c = -1, end_date_c = -1;
    for(size_t i = 0; i < elts.size(); i++) {
        if (elts[i] == "service_id")
            id_c = i;
        else if (elts[i] == "monday")
            monday_c = i;
        else if (elts[i] == "tuesday")
            tuesday_c = i;
        else if (elts[i] == "wednesday")
            wednesday_c = i;
        else if (elts[i] == "thursday")
            thursday_c = i;
        else if (elts[i] == "friday")
            friday_c = i;
        else if (elts[i] == "saturday")
            saturday_c = i;
        else if (elts[i] == "sunday")
            sunday_c = i;
        else if (elts[i] == "start_date")
            start_date_c = i;
        else if (elts[i] == "end_date")
            end_date_c = i;
    }
    if(id_c == -1 || monday_c == -1 || tuesday_c == -1 || wednesday_c == -1 || thursday_c == -1 || friday_c == -1 || saturday_c == -1 || sunday_c == -1 || start_date_c == -1 || end_date_c == -1){
        std::cerr << "Il manque au moins une colonne dans calendar.txt" << std::endl;
        return;
    }

    std::bitset<7> week;
    unsigned int nblignes = 0;
    while(getline(ifile, line)) {
        nblignes ++;
        boost::trim(line);
        Tokenizer tok(line);
        elts.assign(tok.begin(), tok.end());


        nm::ValidityPattern * vp;
        boost::unordered_map<std::string, nm::ValidityPattern*>::iterator it= vp_map.find(elts[id_c]);
        if(it == vp_map.end()){
            //On initialise la semaine
            week[1] = (elts[monday_c] == "1");
            week[2] = (elts[tuesday_c] == "1");
            week[3] = (elts[wednesday_c] == "1");
            week[4] = (elts[thursday_c] == "1");
            week[5] = (elts[friday_c] == "1");
            week[6] = (elts[saturday_c] == "1");
            week[0] = (elts[sunday_c] == "1");

            vp = new nm::ValidityPattern(start);


            for(unsigned int i = 0; i<366;++i)
                vp->remove(i);

            //Initialisation des jours de la date de départ jusqu'à la date de fin du service
            boost::gregorian::date_period period = boost::gregorian::date_period(boost::gregorian::from_undelimited_string(elts[start_date_c]), boost::gregorian::from_undelimited_string(elts[end_date_c]));
            for(boost::gregorian::day_iterator it(period.begin()); it<period.end(); ++it) {
                if(week[(*it).day_of_week()]) {
                    vp->add((*it));
                }
                else
                    vp->remove((*it));
            }


            vp_map[elts[id_c]] = vp;
            data.validity_patterns.push_back(vp);

        }
        else {
            vp = it->second;
        }
    }
    BOOST_ASSERT(data.validity_patterns.size() == vp_map.size());
    std::cout << "Nombre de validity patterns : " << data.validity_patterns.size() << "nb lignes : " << nblignes <<  std::endl;

}

void GtfsParser::parse_calendar_dates(Data & data){
    std::cout << "On parse : " << (path + "/calendar_dates.txt").c_str() << std::endl;
    std::fstream ifile((path + "/calendar_dates.txt").c_str());
    remove_bom(ifile);
    std::string line;

    if(!getline(ifile, line)){
        std::cerr << "Impossible d'ouvrir le fichier calendar_dates.txt" << std::endl;
        return;
    }

    boost::trim(line);
    Tokenizer tok_header(line);
    std::vector<std::string> elts(tok_header.begin(), tok_header.end());
    int id_c = -1, date_c = -1, e_type_c = -1;
    for(size_t i = 0; i < elts.size(); i++) {
        if (elts[i] == "service_id")
            id_c = i;
        else if (elts[i] == "date")
            date_c = i;
        else if (elts[i] == "exception_type")
            e_type_c = i;
    }
    if(id_c == -1 || date_c == -1 || e_type_c == -1){
        std::cerr << "Il manque au moins une colonne dans calendar_dates.txt" << std::endl;
        return;
    }

    while(getline(ifile, line)) {
        boost::trim(line);
        Tokenizer tok(line);
        elts.assign(tok.begin(), tok.end());


        nm::ValidityPattern * vp;
        boost::unordered_map<std::string, nm::ValidityPattern*>::iterator it= vp_map.find(elts[id_c]);
        if(it == vp_map.end()){
            vp = new nm::ValidityPattern(start);
            vp_map[elts[id_c]] = vp;
            data.validity_patterns.push_back(vp);
        }
        else {
            vp = it->second;
        }

        auto date = boost::gregorian::from_undelimited_string(elts[date_c]);
        if(elts[e_type_c] == "1")
            vp->add(date);
        else if(elts[e_type_c] == "2")
            vp->remove(date);
        else
            std::cerr << "Exception pour le service " << elts[id_c] << " inconnue : " << elts[e_type_c] << std::endl;
    }
    BOOST_ASSERT(data.validity_patterns.size() == vp_map.size());
    std::cout << "Nombre de validity patterns : " << data.validity_patterns.size() << std::endl;
}

void GtfsParser:: parse_routes(Data & data){
    data.lines.reserve(10000);
    std::cout << "On parse : " << (path + "/routes.txt").c_str() << std::endl;
    std::fstream ifile((path + "/routes.txt").c_str());
    remove_bom(ifile);
    std::string line;
    if(!getline(ifile, line)) {
        std::cerr << "Impossible d'ouvrir le fichier routes.txt" << std::endl;
        return;
    }

    boost::trim(line);
    Tokenizer tok_header(line);
    std::vector<std::string> elts(tok_header.begin(), tok_header.end());
    int id_c = -1, agency_c = -1, short_name_c = -1, long_name_c = -1, desc_c = -1, type_c = -1, color_c = -1;
    for(size_t i = 0; i < elts.size(); i++){
        if (elts[i] == "route_id")
            id_c = i;
        else if(elts[i] == "agency_id")
            agency_c = i;
        else if(elts[i] == "route_short_name")
            short_name_c = i;
        else if(elts[i] == "route_long_name")
            long_name_c = i;
        else if(elts[i] == "route_desc")
            desc_c = i;
        else if(elts[i] == "route_type")
            type_c = i;
        else if(elts[i] == "route_color")
            color_c = i;
    }
    if(id_c == -1 || short_name_c == -1 || long_name_c == -1 || type_c == -1 || color_c == -1) {
        std::cerr << "Il manque au moins une colonne dans routes.txt" << std::endl;
        return;
    }
    int ignored = 0;
    while(getline(ifile, line)) {
        boost::trim(line);
        Tokenizer tok(line);
        elts.assign(tok.begin(), tok.end());

        if(line_map.find(elts[id_c]) == line_map.end()) {
            nm::Line * line = new nm::Line();
            line->external_code = elts[id_c];
            line->name = elts[long_name_c];
            line->code = elts[short_name_c];
            line->color = elts[color_c];
            line->additional_data = elts[long_name_c];

            boost::unordered_map<std::string, nm::ModeType*>::iterator it= mode_type_map.find(elts[type_c]);
            if(it != mode_type_map.end())
                line->mode_type = it->second;

            line_map[elts[id_c]] = line;
            data.lines.push_back(line);
        }
        else {
            ignored++;
            std::cout << "Doublon de la ligne " << elts[id_c] << std::endl;
        }
    }
    BOOST_ASSERT(data.lines.size() == line_map.size());
    std::cout << "Nombre de lignes : " << data.lines.size() << std::endl;
    std::cout << "J'ai ignoré " << ignored << " lignes pour cause de doublons" << std::endl;
    std::cout << std::endl;
}

void GtfsParser::parse_trips(Data & data) {
    data.routes.reserve(350000);
    data.vehicle_journeys.reserve(350000);
    std::cout << "On parse : " << (path + "/trips.txt").c_str() << std::endl;
    std::fstream ifile((path + "/trips.txt").c_str());
    remove_bom(ifile);
    std::string line;
    if(!getline(ifile, line)) {
        std::cerr << "Impossible d'ouvrir le fichier trips.txt" << std::endl;
        return;
    }

    boost::trim(line);
    Tokenizer tok_header(line);
    std::vector<std::string> elts(tok_header.begin(), tok_header.end());
    int id_c = -1, service_c = -1, trip_c = -1, headsign_c = -1, direction_c = -1, block_c = -1;
    for(size_t i = 0; i < elts.size(); i++){
        if (elts[i] == "route_id")
            id_c = i;
        else if (elts[i] == "service_id")
            service_c = i;
        else if (elts[i] == "trip_id")
            trip_c = i;
        else if (elts[i] == "trip_headsign")
            headsign_c = i;
        else if (elts[i] == "direction_id")
            direction_c = i;
        else if (elts[i] == "block_id")
            block_c = i;
    }

    if (id_c == -1 || service_c == -1 || trip_c == -1 || headsign_c == -1){
        std::cerr << "Il manque au moins une colonne dans trips.txt" << std::endl;
        return;
    }

    int ignored = 0;
    int ignored_vj = 0;
    while(getline(ifile, line)) {
        boost::trim(line);
        Tokenizer tok(line);
        elts.assign(tok.begin(), tok.end());

        boost::unordered_map<std::string, nm::Line*>::iterator it = line_map.find(elts[id_c]);
        if(it == line_map.end()){
            std::cerr << "Impossible de trouver la Route (au sens GTFS) " << elts[id_c]
                      << " référencée par trip " << elts[trip_c] << std::endl;
        }
        else {
            nm::Line * line = it->second;

            boost::unordered_map<std::string, nm::Mode*>::iterator itm = mode_map.find(line->mode_type->id);
            if(itm == mode_map.end()){
                std::cerr << "Impossible de trouver le mode (au sens GTFS) " << line->mode_type->id
                          << " référencée par trip " << elts[trip_c] << std::endl;
            } else {
                nm::ValidityPattern * vp_xx;

                boost::unordered_map<std::string, nm::ValidityPattern*>::iterator vp_it = vp_map.find(elts[service_c]);
                if(vp_it == vp_map.end()) {
                    ignored++;
                    //std::cerr << "Impossible de trouver le service " << elts[service_c] << std::endl;
                }
                else {
                    vp_xx = vp_it->second;
                }


                boost::unordered_map<std::string, nm::VehicleJourney*>::iterator vj_it = vj_map.find(elts[trip_c]);
                if(vj_it == vj_map.end()) {
                    nm::VehicleJourney * vj = new nm::VehicleJourney();
                    vj->name = elts[trip_c];
                    vj->external_code = elts[trip_c];
                    vj->validity_pattern = vp_xx;
                    vj->route = 0;
                    vj->tmp_line = line;
                    vj->mode = itm->second;
                    vj_map[vj->name] = vj;
                    data.vehicle_journeys.push_back(vj);
                }
                else {
                    ignored_vj++;
                }

            }
        }
    }

    BOOST_ASSERT(data.vehicle_journeys.size() == vj_map.size());
    std::cout << "Nombre de vehicle journeys : " << data.vehicle_journeys.size() << std::endl;
    std::cout << "Nombre d'erreur de référence de service : " << ignored << std::endl;
    std::cout << "J'ai ignoré " << ignored_vj << " vehicule journey à cause de doublons" << std::endl;
    std::cout << std::endl;
}

// Compare si deux vehicle journey appartiennent à la même route
bool same_route(nm::VehicleJourney * vj1, nm::VehicleJourney * vj2){
    if(vj1->stop_time_list.size() != vj2->stop_time_list.size())
        return false;
    for(size_t i = 0; i < vj1->stop_time_list.size(); ++i)
        if(vj1->stop_time_list[i]->tmp_stop_point != vj2->stop_time_list[i]->tmp_stop_point){
            return false;
        }
    return true;
}

void GtfsParser::build_routes(Data & data){
    std::cout << "On calcule les routes" << std::endl;
    // Associe à chaque line external_code le nombre de route trouvées jusqu'à present
    std::map<std::string, int> line_routes_count;
    for(auto it1 = data.vehicle_journeys.begin(); it1 != data.vehicle_journeys.end(); ++it1){
        nm::VehicleJourney * vj1 = *it1;
        // Si le vj n'appartient encore à aucune route
        if(vj1->route == 0) {
            auto it = line_routes_count.find(vj1->tmp_line->external_code);
            int count = 1;
            if(it == line_routes_count.end()){
                line_routes_count[vj1->tmp_line->external_code] = count;
            } else {
                count = it->second + 1;
                it->second = count;
            }

            nm::Route * route = new nm::Route();
            route->external_code = vj1->tmp_line->external_code + "-" + boost::lexical_cast<std::string>(count);
            route->line = vj1->tmp_line;
            route->mode = vj1->mode;
            vj1->route = route;
            data.routes.push_back(route);

            for(auto it2 = it1 + 1; it1 != data.vehicle_journeys.end() && it2 != data.vehicle_journeys.end(); ++it2){
                nm::VehicleJourney * vj2 = *it2;
                if(vj2->route == 0 && same_route(vj1, vj2)){
                    vj2->route = vj1->route;
                }
            }
        }
    }
    std::cout << "Nombre de routes : " << data.routes.size() << std::endl;
    std::cout << std::endl;
}

void GtfsParser::build_route_points(Data & data){
    std::cout << "Construction des route points" << std::endl;
    std::map<std::string, navimake::types::RoutePoint*> route_point_map;

    int stop_seq;
    BOOST_FOREACH(nm::VehicleJourney * vj, data.vehicle_journeys){

        stop_seq = 0;
        BOOST_FOREACH(nm::StopTime * stop_time, vj->stop_time_list){
            std::string route_point_extcode = vj->route->external_code + ":" + stop_time->tmp_stop_point->external_code+":"+boost::lexical_cast<std::string>(stop_seq);


            auto route_point_it = route_point_map.find(route_point_extcode);
            nm::RoutePoint * route_point;
            if(route_point_it == route_point_map.end()) {
                route_point = new nm::RoutePoint();
                route_point->route = vj->route;
                route_point->stop_point = stop_time->tmp_stop_point;
                route_point_map[route_point_extcode] = route_point;
                route_point->order = stop_seq;
                route_point->external_code = route_point_extcode;
                data.route_points.push_back(route_point);
            } else {
                route_point = route_point_it->second;
            }
            ++stop_seq;
            stop_time->route_point = route_point;

        }

    }
    std::cout << "Nombre de route points : " << data.route_points.size() << std::endl;
}

void GtfsParser::parse_stop_times(Data & data) {
    std::cout << "On parse : " << (path + "/stop_times.txt").c_str() << std::endl;
    data.stops.reserve(8000000);
    std::fstream ifile((path + "/stop_times.txt").c_str());
    remove_bom(ifile);
    std::string line;
    if(!getline(ifile, line)) {
        std::cerr << "Impossible d'ouvrir le fichier stop_times.txt" << std::endl;
        return;
    }

    boost::trim(line);
    Tokenizer tok_header(line);
    std::vector<std::string> elts(tok_header.begin(), tok_header.end());
    int id_c = -1, arrival_c = -1, departure_c = -1, stop_c = -1, stop_seq_c = -1, pickup_c = -1, drop_c = -1, dist_c = -1;
    for(size_t i = 0; i < elts.size(); i++){
        if (elts[i] == "trip_id")
            id_c = i;
        else if (elts[i] == "arrival_time")
            arrival_c = i;
        else if (elts[i] == "departure_time")
            departure_c = i;
        else if (elts[i] == "stop_id")
            stop_c = i;
        else if (elts[i] == "stop_sequence")
            stop_seq_c = i;
        else if (elts[i] == "pickup_type")
            pickup_c = i;
        else if (elts[i] == "drop_off_type")
            drop_c = i;
        else if (elts[i] == "shape_dist_traveled")
            dist_c = i;
    }

    if(id_c == -1 || arrival_c == -1 || departure_c == -1 || stop_c == -1 || stop_seq_c == -1){
        std::cerr << "Il manque au moins une colonne dans stop_times.txt" << std::endl;
        return;
    }


    size_t count = 0;
    while(getline(ifile, line)) {
        boost::trim(line);
        Tokenizer tok(line);
        std::vector<std::string> elts(tok.begin(), tok.end());

        auto vj_it = vj_map.find(elts[id_c]);
        boost::algorithm::trim(elts[stop_c]);
        auto stop_it = stop_map.find(elts[stop_c]);
        if(vj_it == vj_map.end()) {
            std::cerr << "Impossible de trouver le vehicle_journey " << elts[id_c] << std::endl;
        }
        else if(stop_it == stop_map.end()){
            std::cerr << "Impossible de trouver le StopPoint " << elts[stop_c] << "!"<< std::endl;
        }
        else {
            nm::StopTime * stop_time = new nm::StopTime();
            stop_time->arrival_time = time_to_int(elts[arrival_c]);
            stop_time->departure_time = time_to_int(elts[departure_c]);
            stop_time->tmp_stop_point = stop_it->second;
            //stop_time->route_point = route_point;
            stop_time->order = boost::lexical_cast<int>(elts[stop_seq_c]);
            stop_time->vehicle_journey = vj_it->second;
            stop_time->ODT = 0;//(elts[pickup_c] == "2" && elts[drop_c] == "2");
            stop_time->zone = 0; // à définir selon pickup_type ou drop_off_type = 10
            stop_time->vehicle_journey->stop_time_list.push_back(stop_time);
            data.stops.push_back(stop_time);
            count++;
        }
    }
    std::cout << "Nombre d'horaires : " << data.stops.size() << std::endl;
}

}}
