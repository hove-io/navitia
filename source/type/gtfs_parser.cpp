#include "gtfs_parser.h"
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
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
        start(boost::gregorian::from_undelimited_string(start)) {
    std::cout << "Date de début : " << this->start << std::endl;
    parse_stops();
    parse_calendar_dates();
    parse_routes();
    parse_trips();
    parse_stop_times();

}


void GtfsParser::parse_stops() {
    data.stop_points.reserve(56000);
    data.stop_areas.reserve(13000);
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

    if(id_c == -1 || code_c == -1 || lat_c == -1 || lon_c == -1 || type_c == -1 || parent_c == -1 || name_c == -1) {
        std::cerr << "Il manque au moins une colonne dans le fichier stops.txt" << std::endl;
        return;
    }

    int ignored = 0;

    while(getline(ifile, line)) {
        boost::trim(line);
        Tokenizer tok(line);
        elts.assign(tok.begin(), tok.end());
        StopPoint sp;
        try{
            sp.coord.x = boost::lexical_cast<double>(elts[lon_c]);
            sp.coord.y = boost::lexical_cast<double>(elts[lat_c]);
        }
        catch(boost::bad_lexical_cast ) {
            std::cerr << "Impossible de parser les coordonnées pour " << elts[id_c] << " " << elts[code_c]
                    << " " << elts[name_c] << std::endl;
        }

        sp.name = elts[name_c];
        sp.external_code = elts[id_c];

        if(!data.stop_points.empty() && stop_map.find(sp.external_code) != stop_map.end()) {
            ignored++;
        }
        else {

            // Si c'est un stopArea
            if(elts[type_c] == "1") {
                StopArea sa;
                sa.coord = sp.coord;
                sa.name = sp.name;
                sa.external_code = sp.external_code;
                sa.idx = stop_area_map[sa.external_code] = data.stop_areas.size();
                data.stop_areas.push_back(sa);
            }
            // C'est un StopPoint
            else {
                sp.idx = stop_map[sp.external_code] = data.stop_points.size();
                data.stop_points.push_back(sp);
                if(elts[parent_c] != "") ///On sauvegarde la référence à la zone d'arrêt
                    stoppoint_areas.push_back(std::make_pair(sp.idx, elts[parent_c]));
            }
        }
    }

    // On reboucle pour récupérer les stop areas de tous les stop points
    BOOST_FOREACH(auto sa, stoppoint_areas){
        auto it = stop_area_map.find(sa.second);
        if(it != stop_area_map.end()) {
            data.stop_points[sa.first].stop_area_idx = it->second;
        }
        else
            std::cerr << "Le stopPoint " << data.stop_points[sa.first].external_code
                    << " a utilisé un stopArea inconnu : " << sa.second << std::endl;
    }

    std::cout << "J'ai parsé " << data.stop_points.size() << " stop points" << std::endl;
    std::cout << "J'ai parsé " << data.stop_areas.size() << " stop areas" << std::endl;
    std::cout << "J'ai ignoré " << ignored << " points à cause de doublons" << std::endl;
    std::cout << std::endl;
}

void GtfsParser::parse_calendar_dates(){
    data.validity_patterns.reserve(10000);
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

        int idx;
        boost::unordered_map<std::string, int>::iterator it= vp_map.find(elts[id_c]);
        if(it == vp_map.end()){
            ValidityPattern vp(start);
            vp.idx = data.validity_patterns.size();
            vp_map[elts[id_c]] = vp.idx;
            data.validity_patterns.push_back(vp);
            idx = vp.idx;
        }
        else {
            idx = it->second;
        }

        auto date = boost::gregorian::from_undelimited_string(elts[date_c]);
        if(elts[e_type_c] == "1")
            data.validity_patterns[idx].add(date);
        else if(elts[e_type_c] == "2")
            data.validity_patterns[idx].remove(date);
        else
            std::cerr << "Exception pour le service " << elts[id_c] << " inconnue : " << elts[e_type_c] << std::endl;
    }
    BOOST_ASSERT(data.validity_patterns.size() == vp_map.size());
    std::cout << "Nombre de validity patterns : " << data.validity_patterns.size() << std::endl;
}

void GtfsParser::parse_routes(){
    data.lines.reserve(10000);
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
    if(id_c == -1 || agency_c == -1 || short_name_c == -1 || long_name_c == -1 || desc_c == -1 || type_c == -1 || color_c == -1) {
        std::cerr << "Il manque au moins une colonne dans routes.txt" << std::endl;
        return;
    }
    int ignored = 0;
    while(getline(ifile, line)) {
        boost::trim(line);
        Tokenizer tok(line);
        elts.assign(tok.begin(), tok.end());

        if(line_map.find(elts[id_c]) == line_map.end()) {
            Line line;
            line.name = elts[long_name_c];
            line.code = elts[short_name_c];
            line.mode_list.push_back(0);
            line.color = elts[color_c];
            line.additional_data = elts[long_name_c];

            line.idx = line_map[elts[id_c]] = data.lines.size();
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

void GtfsParser::parse_trips() {
    data.routes.reserve(350000);
    data.vehicle_journeys.reserve(350000);
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

    if (id_c == -1 || service_c == -1 || trip_c == -1 || headsign_c == -1 || direction_c == -1 || block_c == -1){
        std::cerr << "Il manque au moins une colonne dans trips.txt" << std::endl;
        return;
    }

    int ignored = 0;
    int ignored_vj = 0;
    while(getline(ifile, line)) {
        boost::trim(line);
        Tokenizer tok(line);
        elts.assign(tok.begin(), tok.end());

        boost::unordered_map<std::string, int>::iterator it = line_map.find(elts[id_c]);
        if(it == line_map.end()){
            std::cerr << "Impossible de trouver la Route (au sens GTFS) " << elts[id_c]
                    << " référencée par trip " << elts[trip_c] << std::endl;
        }
        else {
            int line_idx = it->second;
            Route route;
            route.line_idx = line_idx;
            int vp_xx = 0;

            boost::unordered_map<std::string, int>::iterator vp_it = vp_map.find(elts[service_c]);
            if(vp_it == vp_map.end()) {
                ignored++;
                //std::cerr << "Impossible de trouver le service " << elts[service_c] << std::endl;
            }
            else {
                data.lines[line_idx].validity_pattern_list.push_back(vp_it->second);
                vp_xx = vp_it->second;
            }

            route.idx = route_map[elts[trip_c]] = data.routes.size();
            data.routes.push_back(route); //elts[2]

            boost::unordered_map<std::string, int>::iterator vj_it = vj_map.find(elts[trip_c]);
            if(vj_it == vj_map.end()) {
                VehicleJourney vj;
                vj.route_idx = data.routes.size() - 1;
                //vj->company = route->line->network;
                vj.name = elts[trip_c];
                vj.external_code = elts[trip_c];
                //vj->mode = route->mode_type;
                vj.validity_pattern_idx =vp_xx;// vp_it->second;

                vj.idx = vj_map[vj.name] = data.vehicle_journeys.size();
                data.vehicle_journeys.push_back(vj);
            }
            else {
                ignored_vj++;
            }
        }
    }

    BOOST_ASSERT(data.vehicle_journeys.size() == vj_map.size());
    std::cout << "Nombre de routes : " << data.routes.size() << std::endl;
    std::cout << "Nombre de vehicle journeys : " << data.vehicle_journeys.size() << std::endl;
    std::cout << "Nombre d'erreur de référence de service : " << ignored << std::endl;
    std::cout << "J'ai ignoré " << ignored_vj << " vehicule journey à cause de doublons" << std::endl;
    std::cout << std::endl;
}

void GtfsParser::parse_stop_times() {
    std::cout << "On parse : " << (path + "/stop_times.txt").c_str() << std::endl;
    data.stop_times.reserve(8000000);
    std::fstream ifile((path + "/stop_times.txt").c_str());
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

    if(id_c == -1 || arrival_c == -1 || departure_c == -1 || stop_c == -1 || stop_seq_c == -1 || pickup_c == -1 || drop_c == -1 || dist_c == -1){
        std::cerr << "Il manque au moins une colonne dans stop_times.txt" << std::endl;
        return;
    }

    parse_ended = false;
    int nb_threads = 4;
    boost::thread_group theads;
    for(int i=0; i < nb_threads; i++){
        theads.create_thread(boost::bind(&GtfsParser::parse_stop_times_worker, this, id_c, arrival_c, departure_c, stop_c, stop_seq_c, pickup_c, drop_c));
    }

    while(getline(ifile, line)) {
        queue_mutex.lock();
        queue.push(line);
        queue_mutex.unlock();
        cond.notify_one();
    }
    parse_ended = true;
    cond.notify_all();
    theads.join_all();
    std::cout << "Nombre d'horaires : " << data.stop_times.size() << std::endl;
}

void GtfsParser::parse_stop_times_worker(int id_c, int arrival_c, int departure_c, int stop_c, int stop_seq_c, int pickup_c, int drop_c){
    while(true){
        boost::unique_lock<boost::mutex> lock(queue_mutex);
        while(queue.empty() && !parse_ended)
            cond.wait(lock);
        if(parse_ended && queue.empty())
            return;
        std::string line = queue.back();
        queue.pop();
        lock.unlock();

        boost::trim(line);
        Tokenizer tok(line);
        std::vector<std::string> elts(tok.begin(), tok.end());

        // Accès en lecture seule, donc pas besoin de mutex
        auto route_it = route_map.find(elts[id_c]);
        auto stop_it = stop_map.find(elts[stop_c]);
        if(route_it == route_map.end()) {
            std::cerr << "Impossible de trouver la route " << elts[id_c] << std::endl;
        }
        else if(stop_it == stop_map.end()){
            std::cerr << "Impossible de trouver le StopPoint " << elts[stop_c] << std::endl;
        }
        else {
            StopTime stop_time;
            stop_time.arrival_time = time_to_int(elts[arrival_c]);
            stop_time.departure_time = time_to_int(elts[departure_c]);
            stop_time.stop_point_idx = stop_it->second;
            stop_time.order = boost::lexical_cast<int>(elts[stop_seq_c]);
            stop_time.vehicle_journey_idx = route_it->second;
            stop_time.ODT = (elts[pickup_c] == "2" && elts[drop_c] == "2");
            stop_time.zone = 0; // à définir selon pickup_type ou drop_off_type = 10
            data_mutex.lock();
            data.stop_times.push_back(stop_time);
            data_mutex.unlock();
        }
    }
}
