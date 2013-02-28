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

GtfsParser::GtfsParser(const std::string & path) :
    path(path), production_date(boost::gregorian::date(), boost::gregorian::date()){}

void GtfsParser::fill(Data & data, const std::string beginning_date){
    production_date = find_production_date(beginning_date);
    fill_commercial_modes(data);
    parse_agency(data);
    parse_stops(data);
    parse_journey_patterns(data);
    parse_transfers(data);
    parse_calendar(data);
    parse_calendar_dates(data);
    parse_trips(data);
    parse_stop_times(data);
    parse_frequencies();
    normalize_extcodes(data);
    build_journey_patterns(data);
    build_journey_pattern_points(data);
    build_journey_pattern_point_connections(data);
}

void GtfsParser::fill_commercial_modes(Data & data) {
    navimake::types::CommercialMode* commercial_mode = new navimake::types::CommercialMode();
    commercial_mode->id = "0";
    commercial_mode->name = "Tram";
    commercial_mode->uri = "0x0";
    data.commercial_modes.push_back(commercial_mode);
    commercial_mode_map[commercial_mode->id] = commercial_mode;

    commercial_mode = new navimake::types::CommercialMode();
    commercial_mode->id = "1";
    commercial_mode->name = "Metro";
    commercial_mode->uri = "0x1";
    data.commercial_modes.push_back(commercial_mode);
    commercial_mode_map[commercial_mode->id] = commercial_mode;

    commercial_mode = new navimake::types::CommercialMode();
    commercial_mode->id = "2";
    commercial_mode->name = "Rail";
    commercial_mode->uri = "0x2";
    data.commercial_modes.push_back(commercial_mode);
    commercial_mode_map[commercial_mode->id] = commercial_mode;

    commercial_mode = new navimake::types::CommercialMode();
    commercial_mode->id = "3";
    commercial_mode->name = "Bus";
    commercial_mode->uri = "0x3";
    data.commercial_modes.push_back(commercial_mode);
    commercial_mode_map[commercial_mode->id] = commercial_mode;

    commercial_mode = new navimake::types::CommercialMode();
    commercial_mode->id = "4";
    commercial_mode->name = "Ferry";
    commercial_mode->uri = "0x4";
    data.commercial_modes.push_back(commercial_mode);
    commercial_mode_map[commercial_mode->id] = commercial_mode;

    commercial_mode = new navimake::types::CommercialMode();
    commercial_mode->id = "5";
    commercial_mode->name = "Cable car";
    commercial_mode->uri = "0x5";
    data.commercial_modes.push_back(commercial_mode);
    commercial_mode_map[commercial_mode->id] = commercial_mode;

    commercial_mode = new navimake::types::CommercialMode();
    commercial_mode->id = "6";
    commercial_mode->name = "Gondola";
    commercial_mode->uri = "0x6";
    data.commercial_modes.push_back(commercial_mode);
    commercial_mode_map[commercial_mode->id] = commercial_mode;

    commercial_mode = new navimake::types::CommercialMode();
    commercial_mode->id = "7";
    commercial_mode->name = "Funicular";
    commercial_mode->uri = "0x7";
    data.commercial_modes.push_back(commercial_mode);
    commercial_mode_map[commercial_mode->id] = commercial_mode;


    BOOST_FOREACH(navimake::types::CommercialMode *mt, data.commercial_modes) {
        navimake::types::PhysicalMode* mode = new navimake::types::PhysicalMode();
        mode->id = mt->id;
        mode->name = mt->name;
        mode->uri = mt->uri;
        mode->commercial_mode = mt;
        data.modes.push_back(mode);
        mode_map[mode->id] = mode;
    }


}

void GtfsParser::parse_agency(Data & data){
    std::cout << "On parse : " << (path + "/agency.txt").c_str() << std::endl;
    std::fstream ifile((path + "/agency.txt").c_str());
    remove_bom(ifile);
    std::string line;
    if(!getline(ifile, line)) {
        std::cerr << "Impossible d'ouvrir agency.txt" << std::endl;
        throw std::exception();
        return;
    }
    int id_c = -1, name_c = -1;
    boost::trim(line);
    Tokenizer tok_header(line);
    std::vector<std::string> elts(tok_header.begin(), tok_header.end());
    for(size_t i = 0; i < elts.size(); i++){
        if(elts[i] == "agency_id")
            id_c = i;
        else if(elts[i] == "agency_name")
            name_c = i;
    }

    int id = 1;

    while(getline(ifile, line)) {
        boost::trim(line);
        if(line == "")
            continue;
        Tokenizer tok(line);
        elts.assign(tok.begin(), tok.end());
        for(uint32_t i=0;i<elts.size();++i)
            boost::algorithm::trim(elts[i]);
        nm::Network * network = new nm::Network();
        if(id_c != -1)
            network->uri = elts[id_c];
        else
            network->uri = boost::lexical_cast<std::string>(id);
        network->name = elts[name_c];
        data.networks.push_back(network);
        agency_map[network->uri] = network;
        ++id;
    }
}

void GtfsParser::parse_stops(Data & data) {
    // On réserve un peu large, de l'ordre de l'Île-de-France
    data.stop_points.reserve(56000);
    data.stop_areas.reserve(13000);

    std::cout << "On parse : " << (path + "/stops.txt").c_str() << std::endl;
    std::fstream ifile((path + "/stops.txt").c_str());
    remove_bom(ifile);
    std::string line;
    if(!getline(ifile, line)) {
        std::cerr << "Impossible d'ouvrir stops.txt" << std::endl;
        throw std::exception();
        return;
    }
    std::vector<types::StopPoint*> wheelchair_heritance;

    // colonne que l'on va utiliser
    int id_c = -1, code_c = -1, lat_c = -1, lon_c = -1, type_c = -1, parent_c = -1, name_c = -1, wheelchair_c = -1;

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
        else if(elts[i] == "wheelchair_boarding")
            wheelchair_c = i;
    }
    if(code_c == -1){
        code_c = id_c;
    }

    if(id_c == -1 || code_c == -1 || lat_c == -1 || lon_c == -1 || name_c == -1) {
        std::cerr << "Il manque au moins une colonne dans le fichier stops.txt" << std::endl
                  << "stop_id : " << (id_c!=-1) << " stop_code : " << (code_c!=-1) << " stop_name : " << (name_c!=-1) << " stop_lat : " << (lat_c!=-1)
                  << " stop_lon : " << (lon_c != -1) << std::endl;
        return;
    }

    int ignored = 0;

    while(getline(ifile, line)) {
        boost::trim(line);
        if(line == "")
            continue;
        Tokenizer tok(line);
        elts.assign(tok.begin(), tok.end());
        nm::StopPoint * sp = new nm::StopPoint();
        for(uint32_t i=0;i<elts.size();++i)
            boost::algorithm::trim(elts[i]);
        try{
            sp->coord.set_lon(boost::lexical_cast<double>(elts[lon_c]));
            sp->coord.set_lat(boost::lexical_cast<double>(elts[lat_c]));
        }
        catch(boost::bad_lexical_cast ) {
            std::cerr << "Impossible de parser les coordonnées pour " << elts[id_c] << " " << elts[code_c]
                      << " " << elts[name_c] << std::endl;
        }

        sp->name = elts[name_c];
        sp->uri = elts[id_c];


        if(!data.stop_points.empty() && stop_map.find(sp->uri) != stop_map.end()) {
            ignored++;
        }
        else {
            // Si c'est un stopArea
            if(type_c != -1 && elts[type_c] == "1") {
                nm::StopArea * sa = new nm::StopArea();
                sa->coord = sp->coord;
                sa->name = sp->name;
                sa->uri = sp->uri;
                if(wheelchair_c != -1)
                    sa->is_adapted = elts[wheelchair_c] == "1";
                stop_area_map[sa->uri] = sa;
                data.stop_areas.push_back(sa);
                delete sp;
            }
            // C'est un StopPoint
            else {
                if(wheelchair_c != -1) {
                    if(elts[wheelchair_c] == "0") {
                        wheelchair_heritance.push_back(sp);
                    } else {
                        sp->is_adapted = elts[wheelchair_c] == "1";
                    }
                }
                stop_map[sp->uri] = sp;
                data.stop_points.push_back(sp);
                if(parent_c!=-1 && elts[parent_c] != "") {// On sauvegarde la référence à la zone d'arrêt
                    if(sa_spmap.find(elts[parent_c]) == sa_spmap.end()) {
                        sa_spmap.insert(std::make_pair(elts[parent_c], vector_sp()));
                    }
                    sa_spmap[elts[parent_c]].push_back(sp);
                }
            }
        }
    }


    // On reboucle pour récupérer les stop areas de tous les stop points
    for(auto sa_sps : sa_spmap) {
        if(stop_area_map.find(sa_sps.first) != stop_area_map.end()) {
            auto it = stop_area_map.find(sa_sps.first);
            for(auto sp : sa_sps.second) {
                sp->stop_area = it->second;
            }
        } else {
            std::cerr << "Le stop area " << sa_sps.first
                      << " n'a pas été trouvé pour les stops points :  ";
            for(auto sp : sa_sps.second) {
                std::cerr << sp->uri << " ";
                sp->stop_area = 0;
            }
            std::cerr << std::endl;
        }
    }

    //On va chercher l'accessibilité pour les stop points qui hérite de l'accessibilité de leur stop area
    for(auto sp : wheelchair_heritance) {
        if(sp->stop_area != 0) {
            sp->is_adapted = sp->stop_area->is_adapted;
        } else {
            std::cerr << "Impossible de récuperer l'accessibilité du stop area pour le stop point " << sp->uri << std::endl;
        }
    }




    std::cout << "J'ai parsé " << data.stop_points.size() << " stop points" << std::endl;
    std::cout << "J'ai parsé " << data.stop_areas.size() << " stop areas" << std::endl;
    std::cout << "J'ai ignoré " << ignored << " points à cause de doublons" << std::endl;
    std::cout << std::endl;
}

void GtfsParser::parse_transfers(Data & data) {
    std::cout << "On parse : " << (path + "/transfers.txt").c_str() << std::endl;
    std::fstream ifile((path + "/transfers.txt").c_str());
    remove_bom(ifile);
    std::string line;

    if(!getline(ifile, line)){
        std::cerr << "Impossible d'ouvrir le fichier transfers.txt" << std::endl;
        return;
    }


    boost::trim(line);
    Tokenizer tok_header(line);
    std::vector<std::string> elts(tok_header.begin(), tok_header.end());

    int from_c = -1, to_c = -1, time_c = -1;
    for(size_t  i = 0; i < elts.size(); ++i){
        if(elts[i] == "from_stop_id")
            from_c = i;
        else if(elts[i] == "to_stop_id")
            to_c = i;
        else if(elts[i] == "min_transfer_time")
            time_c = i;
    }

    if(from_c == -1 || to_c == -1){
        std::cerr << "Il manque au moins une colonne dans transfers.txt" << std::endl << "from_stop_id : " << (from_c!=-1) << " to_stop_id : " << (to_c!=-1) << std::endl;
        return;
    }

    int nblines = 0;
    while(getline(ifile, line)) {
        boost::trim(line);
        if(line == "")
            continue;
        Tokenizer tok(line);
        elts.assign(tok.begin(), tok.end());
        for(uint32_t i=0;i<elts.size();++i)
            boost::algorithm::trim(elts[i]);

        typedef boost::unordered_map<std::string, navimake::types::StopPoint*>::iterator sp_iterator;
        vector_sp departures, arrivals;
        boost::unordered_map<std::string, navimake::types::StopPoint*>::iterator it;
        it = this->stop_map.find(elts[from_c]);
        if(it == this->stop_map.end()){
            std::unordered_map<std::string, vector_sp>::iterator it_sa = this->sa_spmap.find(elts[from_c]);
            if(it_sa == this->sa_spmap.end()) {
                std::cerr << "Impossible de trouver le stop point (from) " << elts[from_c] << std::endl;
                continue;
            } else {
                departures = it_sa->second;
            }
        } else {
            departures.push_back(it->second);
        }

        it = this->stop_map.find(elts[to_c]);
        if(it == this->stop_map.end()){
            std::unordered_map<std::string, vector_sp>::iterator it_sa = this->sa_spmap.find(elts[to_c]);
            if(it_sa == this->sa_spmap.end()) {
                std::cerr << "Impossible de trouver le stop point (to) " << elts[to_c] << std::endl;
                continue;
            } else {
                arrivals = it_sa->second;
            }
        } else {
            arrivals.push_back(it->second);
        }

        for(auto from_sp : departures) {
            for(auto to_sp : arrivals) {
                nm::Connection * connection = new nm::Connection();
                connection->departure_stop_point = from_sp;
                connection->destination_stop_point  = to_sp;
                connection->connection_kind = nm::Connection::LinkConnection;
                if(time_c != -1) {
                    try{
                        connection->duration = boost::lexical_cast<int>(elts[time_c]);
                    } catch (...) {
                        connection->duration = 120;
                    }
                } else {
                   connection->duration = 120;
                }

                data.connections.push_back(connection);
            }
        }
        nblines++;
    }

    std::cout << nblines << " correspondances prises en compte sur " << data.connections.size() << std::endl;
}

void GtfsParser::parse_calendar(Data & data) {

    data.validity_patterns.reserve(10000);

    std::cout << "On parse : " << (path + "/calendar.txt").c_str() << std::endl;
    std::fstream ifile((path + "/calendar.txt").c_str());
    if(!ifile.is_open()) {
        std::cout << "Aucun fichier calendar.txt" << std::endl;
        return;
    }
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
        std::cerr << "Il manque au moins une colonne dans calendar.txt" << std::endl
                  << "service_id : " << (id_c != -1) << "monday : " << (monday_c!=-1) << " tuesday : " << (tuesday_c!=-1) << " wednesday : " << (wednesday_c!=-1)
                  << " thursday : " << (thursday_c!=-1) << " friday : " << (friday_c!=-1) << " saturday : " << (saturday_c!=-1) << " sunday : " << (sunday_c!=-1)
                  << " start_date : " << (start_date_c!=-1) << " end_date : " << (end_date_c!=-1) << std::endl;
        return;
    }

    std::bitset<7> week;
    unsigned int nblignes = 0;
    while(getline(ifile, line)) {
        nblignes ++;
        boost::trim(line);
        if(line == "")
            continue;
        Tokenizer tok(line);
        elts.assign(tok.begin(), tok.end());
        for(uint32_t i=0;i<elts.size();++i)
            boost::algorithm::trim(elts[i]);


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

            vp = new nm::ValidityPattern(production_date.begin());

            for(unsigned int i = 0; i<366;++i)
                vp->remove(i);

            //Initialisation des jours de la date de départ jusqu'à la date de fin du service
            boost::gregorian::date b_date = boost::gregorian::from_undelimited_string(elts[start_date_c]);
            boost::gregorian::date_period period = boost::gregorian::date_period((b_date > production_date.begin() ? b_date : production_date.begin()), boost::gregorian::from_undelimited_string(elts[end_date_c]));
            for(boost::gregorian::day_iterator it(period.begin()); it<period.end(); ++it) {
                if(week.test((*it).day_of_week())) {
                    vp->add((*it));
                }
                else
                    vp->remove((*it));
            }

            vp->uri = elts[id_c];
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
        std::cerr << "Il manque au moins une colonne dans calendar_dates.txt" << std::endl
                  << " service_id : " << (id_c!=-1) << " date : " << (date_c!=-1) << " exception_type : " << (e_type_c!=-1) << std::endl;
        return;
    }

    while(getline(ifile, line)) {
        boost::trim(line);
        if(line == "")
            continue;
        Tokenizer tok(line);
        elts.assign(tok.begin(), tok.end());


        nm::ValidityPattern * vp;
        boost::unordered_map<std::string, nm::ValidityPattern*>::iterator it= vp_map.find(elts[id_c]);
        if(it == vp_map.end()){
            vp = new nm::ValidityPattern(production_date.begin());
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

void GtfsParser:: parse_journey_patterns(Data & data){
    data.lines.reserve(10000);
    std::cout << "On parse : " << (path + "/routes.txt").c_str() << std::endl;
    std::fstream ifile((path + "/routes.txt").c_str());
    remove_bom(ifile);
    std::string line;
    if(!getline(ifile, line)) {
        std::cerr << "Impossible d'ouvrir le fichier routes.txt" << std::endl;
        throw std::exception();
        return;
    }

    boost::trim(line);
    Tokenizer tok_header(line);
    std::vector<std::string> elts(tok_header.begin(), tok_header.end());
    int id_c = -1, short_name_c = -1, long_name_c = -1, type_c = -1, color_c = -1, agency_c = -1;
    for(size_t i = 0; i < elts.size(); i++){
        if (elts[i] == "route_id")
            id_c = i;
        else if(elts[i] == "route_short_name")
            short_name_c = i;
        else if(elts[i] == "route_long_name")
            long_name_c = i;
        else if(elts[i] == "route_type")
            type_c = i;
        else if(elts[i] == "route_color")
            color_c = i;
        else if(elts[i] == "agency_id")
            agency_c = i;
    }
    if(id_c == -1 || short_name_c == -1 || long_name_c == -1 || type_c == -1) {
        std::cerr << "Il manque au moins une colonne dans route.txt journey_pattern_id : " << (id_c!=-1) << " route_short_name : "  << (short_name_c!=-1) <<
                     " route long name : " << (long_name_c!=-1) << std::endl;
        return;
    }
    int ignored = 0;
    while(getline(ifile, line)) {
        boost::trim(line);
        if(line == "")
            continue;
        Tokenizer tok(line);
        elts.assign(tok.begin(), tok.end());
        for(uint32_t i=0;i<elts.size();++i)
            boost::algorithm::trim(elts[i]);

        if(line_map.find(elts[id_c]) == line_map.end()) {
            nm::Line * line = new nm::Line();
            line->uri = elts[id_c];
            line->name = elts[long_name_c];
            line->code = elts[short_name_c];
            if(color_c != -1)
                line->color = elts[color_c];
            else
                line->color = "";
            line->additional_data = elts[long_name_c];

            boost::unordered_map<std::string, nm::CommercialMode*>::iterator it= commercial_mode_map.find(elts[type_c]);
            if(it != commercial_mode_map.end())
                line->commercial_mode = it->second;
            if(agency_c != -1) {
                auto agency_it = agency_map.find(elts[agency_c]);
                if(agency_it != agency_map.end())
                    line->network = agency_it->second;
            }
            else {
                auto agency_it = agency_map.find("1");
                if(agency_it != agency_map.end())
                    line->network = agency_it->second;
            }

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
    data.journey_patterns.reserve(350000);
    data.vehicle_journeys.reserve(350000);
    std::cout << "On parse : " << (path + "/trips.txt").c_str() << std::endl;
    std::fstream ifile((path + "/trips.txt").c_str());
    remove_bom(ifile);
    std::string line;
    if(!getline(ifile, line)) {
        std::cerr << "Impossible d'ouvrir le fichier trips.txt" << std::endl;
        throw std::exception();
        return;
    }

    boost::trim(line);
    Tokenizer tok_header(line);
    std::vector<std::string> elts(tok_header.begin(), tok_header.end());
    int id_c = -1, service_c = -1, trip_c = -1, headsign_c = -1, block_id_c = -1, wheelchair_c = -1;
    for(size_t i = 0; i < elts.size(); i++){
        if (elts[i] == "route_id")
            id_c = i;
        else if (elts[i] == "service_id")
            service_c = i;
        else if (elts[i] == "trip_id")
            trip_c = i;
        else if(elts[i] == "trip_headsign")
            headsign_c = i;
        else if(elts[i] == "block_id")
            block_id_c = i;
        else if(elts[i] == "wheelchair_accessible")
            wheelchair_c = i;
    }

    if (id_c == -1 || service_c == -1 || trip_c == -1){
        std::cerr << "Il manque au moins une colonne dans trips.txt" << std::endl
                  << "route_id : " << (id_c!=-1) << " service_id " << (service_c!=-1) << " trip_id : " << (trip_c!=-1) << std::endl;
        return;
    }

    int ignored = 0;
    int ignored_vj = 0;
    while(getline(ifile, line)) {
        boost::trim(line);
        if(line == "")
            continue;
        Tokenizer tok(line);
        elts.assign(tok.begin(), tok.end());
        for(uint32_t i=0;i<elts.size();++i)
            boost::algorithm::trim(elts[i]);

        boost::unordered_map<std::string, nm::Line*>::iterator it = line_map.find(elts[id_c]);
        if(it == line_map.end()){
            std::cerr << "Impossible de trouver la Route (au sens GTFS) " << elts[id_c]
                      << " référencée par trip " << elts[trip_c] << std::endl;
        }
        else {
            nm::Line * line = it->second;

            boost::unordered_map<std::string, nm::PhysicalMode*>::iterator itm = mode_map.find(line->commercial_mode->id);
            if(itm == mode_map.end()){
                std::cerr << "Impossible de trouver le mode (au sens GTFS) " << line->commercial_mode->id
                          << " référencée par trip " << elts[trip_c] << std::endl;
            } else {
                nm::ValidityPattern * vp_xx;

                boost::unordered_map<std::string, nm::ValidityPattern*>::iterator vp_it = vp_map.find(elts[service_c]);
                if(vp_it == vp_map.end()) {
                    ignored++;
                    continue;
                }
                else {
                    vp_xx = vp_it->second;
                }


                boost::unordered_map<std::string, nm::VehicleJourney*>::iterator vj_it = vj_map.find(elts[trip_c]);
                if(vj_it == vj_map.end()) {
                    nm::VehicleJourney * vj = new nm::VehicleJourney();
                    vj->uri = elts[trip_c];
                    if(headsign_c != -1)
                        vj->name = elts[headsign_c];
                    else
                        vj->name = vj->uri;
                    vj->validity_pattern = vp_xx;
                    vj->journey_pattern = 0;
                    vj->tmp_line = line;
                    vj->physical_mode = itm->second;
                    if(block_id_c != -1)
                        vj->block_id = elts[block_id_c];
                    else
                        vj->block_id = "";
                    if(wheelchair_c != -1)
                        vj->is_adapted = elts[wheelchair_c] == "1";
                    vj_map[vj->uri] = vj;
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

void GtfsParser::parse_frequencies() {
    std::cout << "On parse : " << (path + "/frequencies.txt").c_str() << std::endl;
    std::fstream ifile((path + "/frequencies.txt").c_str());
    remove_bom(ifile);
    std::string line;
    if(!getline(ifile, line)) {
        std::cerr << "Impossible d'ouvrir le fichier frequencies.txt" << std::endl;
        return;
    }

    boost::trim(line);
    Tokenizer tok_header(line);
    std::vector<std::string> elts(tok_header.begin(), tok_header.end());
    int trip_id_c = -1, start_time_c = -1, end_time_c = -1, headway_secs_c = -1;
    for(size_t i = 0; i < elts.size(); i++){
        if (elts[i] == "trip_id")
            trip_id_c = i;
        else if (elts[i] == "start_time")
            start_time_c = i;
        else if (elts[i] == "end_time")
            end_time_c = i;
        else if(elts[i] == "headway_secs")
            headway_secs_c = i;
    }

    if (trip_id_c == -1 || start_time_c == -1 || end_time_c == -1 || headway_secs_c == -1){
        std::cerr << "Il manque au moins une colonne dans frenquencies.txt" << std::endl
                  << "trip_id : " << (trip_id_c!=-1) << " start_time " << (start_time_c!=-1) << " end_time : " << (end_time_c!=-1) << " headway_secs : " << (headway_secs_c!=-1) << std::endl;
        return;
    }

    while(getline(ifile, line)) {
        boost::trim(line);
        if(line == "")
            continue;
        Tokenizer tok(line);
        elts.assign(tok.begin(), tok.end());
        for(uint32_t i=0;i<elts.size();++i)
            boost::algorithm::trim(elts[i]);

        boost::unordered_map<std::string, nm::VehicleJourney*>::iterator vj_it = vj_map.find(elts[trip_id_c]);
        if(vj_it != vj_map.end()) {
            int begin = vj_it->second->stop_time_list.front()->arrival_time;
            for(auto st_it = vj_it->second->stop_time_list.begin(); st_it != vj_it->second->stop_time_list.end(); ++st_it) {
                (*st_it)->start_time = time_to_int(elts[start_time_c]) + (*st_it)->arrival_time - begin;
                (*st_it)->end_time = time_to_int(elts[end_time_c]) + (*st_it)->arrival_time - begin;
                (*st_it)->headway_secs = boost::lexical_cast<int>(elts[headway_secs_c]);
                (*st_it)->is_frequency = true;
            }
        }
    }
}

// Compare si deux vehicle journey appartiennent à la même journey_pattern
bool same_journey_pattern(nm::VehicleJourney * vj1, nm::VehicleJourney * vj2){
    if(vj1->stop_time_list.size() != vj2->stop_time_list.size())
        return false;
    for(size_t i = 0; i < vj1->stop_time_list.size(); ++i)
        if(vj1->stop_time_list[i]->tmp_stop_point != vj2->stop_time_list[i]->tmp_stop_point){
            return false;
        }
    return true;
}

void normalize_extcodes(Data & data){
    for(nm::StopArea * sa : data.stop_areas){
        boost::algorithm::replace_first(sa->uri, "StopArea:", "");
    }
    for(nm::StopPoint * sp : data.stop_points){
        boost::algorithm::replace_first(sp->uri, "StopPoint:", "");
    }
}

// TODO : pour l'instant on construit une route par journey pattern
// Il faudrait factoriser les routes
void build_journey_patterns(Data & data){
    std::cout << "On calcule les journey_patterns" << std::endl;
    // Associe à chaque line uri le nombre de journey_pattern trouvées jusqu'à present
    std::map<std::string, int> line_journey_patterns_count;
    for(auto it1 = data.vehicle_journeys.begin(); it1 != data.vehicle_journeys.end(); ++it1){
        nm::VehicleJourney * vj1 = *it1;
        // Si le vj n'appartient encore à aucune journey_pattern
        if(vj1->journey_pattern == 0) {
            auto it = line_journey_patterns_count.find(vj1->tmp_line->uri);
            int count = 1;
            if(it == line_journey_patterns_count.end()){
                line_journey_patterns_count[vj1->tmp_line->uri] = count;
            } else {
                count = it->second + 1;
                it->second = count;
            }

            nm::Route * route = new nm::Route();

            nm::JourneyPattern * journey_pattern = new nm::JourneyPattern();
            journey_pattern->uri = vj1->tmp_line->uri + "-" + boost::lexical_cast<std::string>(count);
            journey_pattern->route = route;
            journey_pattern->physical_mode = vj1->physical_mode;
            vj1->journey_pattern = journey_pattern;
            data.journey_patterns.push_back(journey_pattern);

            route->line = vj1->tmp_line;
            route->uri = journey_pattern->uri;
            route->name = journey_pattern->name;
            data.routes.push_back(route);

            for(auto it2 = it1 + 1; it1 != data.vehicle_journeys.end() && it2 != data.vehicle_journeys.end(); ++it2){
                nm::VehicleJourney * vj2 = *it2;
                if(vj2->journey_pattern == 0 && same_journey_pattern(vj1, vj2)){
                    vj2->journey_pattern = vj1->journey_pattern;
                }
            }
        }
    }
    std::cout << "Nombre de journey_patterns : " << data.journey_patterns.size() << std::endl;
    std::cout << std::endl;
}

void build_journey_pattern_points(Data & data){
    std::cout << "Construction des journey_pattern points" << std::endl;
    std::map<std::string, navimake::types::JourneyPatternPoint*> journey_pattern_point_map;

    int stop_seq;
    BOOST_FOREACH(nm::VehicleJourney * vj, data.vehicle_journeys){

        stop_seq = 0;
        BOOST_FOREACH(nm::StopTime * stop_time, vj->stop_time_list){
            std::string journey_pattern_point_extcode = vj->journey_pattern->uri + ":" + stop_time->tmp_stop_point->uri+":"+boost::lexical_cast<std::string>(stop_seq);


            auto journey_pattern_point_it = journey_pattern_point_map.find(journey_pattern_point_extcode);
            nm::JourneyPatternPoint * journey_pattern_point;
            if(journey_pattern_point_it == journey_pattern_point_map.end()) {
                journey_pattern_point = new nm::JourneyPatternPoint();
                journey_pattern_point->journey_pattern = vj->journey_pattern;
                journey_pattern_point->journey_pattern->journey_pattern_point_list.push_back(journey_pattern_point);
                journey_pattern_point->stop_point = stop_time->tmp_stop_point;
                journey_pattern_point_map[journey_pattern_point_extcode] = journey_pattern_point;
                journey_pattern_point->order = stop_seq;
                journey_pattern_point->uri = journey_pattern_point_extcode;
                data.journey_pattern_points.push_back(journey_pattern_point);
            } else {
                journey_pattern_point = journey_pattern_point_it->second;
            }
            ++stop_seq;
            stop_time->journey_pattern_point = journey_pattern_point;

        }

    }

    for(nm::JourneyPattern *journey_pattern : data.journey_patterns){
        if(! journey_pattern->journey_pattern_point_list.empty()){
            nm::JourneyPatternPoint * last = journey_pattern->journey_pattern_point_list.back();
            if(last->stop_point->stop_area != NULL)
                journey_pattern->name = last->stop_point->stop_area->name;
            else
                journey_pattern->name = last->stop_point->name;
        }
    }
    std::cout << "Nombre de journey_pattern points : " << data.journey_pattern_points.size() << std::endl;
}

void GtfsParser::parse_stop_times(Data & data) {
    std::cout << "On parse : " << (path + "/stop_times.txt").c_str() << std::endl;
    data.stops.reserve(8000000);
    std::fstream ifile((path + "/stop_times.txt").c_str());
    remove_bom(ifile);
    std::string line;
    if(!getline(ifile, line)) {
        std::cerr << "Impossible d'ouvrir le fichier stop_times.txt" << std::endl;
        throw std::exception();
        return;
    }

    boost::trim(line);
    Tokenizer tok_header(line);
    std::vector<std::string> elts(tok_header.begin(), tok_header.end());
    int id_c = -1, arrival_c = -1, departure_c = -1, stop_c = -1, stop_seq_c = -1, pickup_c = -1, drop_off_c = -1, itl_c = -1;
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
            drop_off_c = i;
        else if (elts[i] == "stop_times_itl")
            itl_c = i;
    }

    if(id_c == -1 || arrival_c == -1 || departure_c == -1 || stop_c == -1 || stop_seq_c == -1){
        std::cerr << "Il manque au moins une colonne dans stop_times.txt" << std::endl
                  << "trip_id : " << (id_c=-1) << " arrival_time : " << (arrival_c!=-1) << " departure_time : " << (departure_c!=-1)
                  << " stop_id : " << (stop_c!=-1) << " stop_sequence : " << (stop_seq_c!=-1) << std::endl;
        return;
    }


    size_t count = 0;
    while(getline(ifile, line)) {
        boost::trim(line);
        if(line == "")
            continue;
        Tokenizer tok(line);
        std::vector<std::string> elts(tok.begin(), tok.end());
        for(uint32_t i=0;i<elts.size();++i)
            boost::algorithm::trim(elts[i]);

        auto vj_it = vj_map.find(elts[id_c]);
        if(vj_it == vj_map.end()) {
            std::cerr << "Impossible de trouver le vehicle_journey " << elts[id_c] <<   std::endl;
            exit(1);
        }
        auto stop_it = stop_map.find(elts[stop_c]);
        if(stop_it == stop_map.end()){
            std::cerr << "Impossible de trouver le StopPoint " << elts[stop_c] << "!"<< std::endl;
        }
        else {
            nm::StopTime * stop_time = new nm::StopTime();
            stop_time->arrival_time = time_to_int(elts[arrival_c]);
            stop_time->departure_time = time_to_int(elts[departure_c]);
            stop_time->tmp_stop_point = stop_it->second;
            //stop_time->journey_pattern_point = journey_pattern_point;
            stop_time->order = boost::lexical_cast<int>(elts[stop_seq_c]);
            stop_time->vehicle_journey = vj_it->second;
            if(pickup_c != -1 && drop_off_c != -1)
                stop_time->ODT = (elts[pickup_c] == "2" && elts[drop_off_c] == "2");
            else
                stop_time->ODT = false;
            if(pickup_c != -1)
                stop_time->pick_up_allowed = elts[pickup_c] != "1";
            else
                stop_time->pick_up_allowed = true;
            if(drop_off_c != -1)
                stop_time->drop_off_allowed = elts[drop_off_c] != "1";
            else
                stop_time->drop_off_allowed = true;
            if(itl_c != -1)
                stop_time->local_traffic_zone = boost::lexical_cast<int>(elts[itl_c]);
            else
                stop_time->local_traffic_zone = std::numeric_limits<uint32_t>::max();
            stop_time->vehicle_journey->stop_time_list.push_back(stop_time);
            stop_time->is_adapted = stop_time->vehicle_journey->is_adapted;
            data.stops.push_back(stop_time);
            count++;
        }
    }
    std::cout << "Nombre d'horaires : " << data.stops.size() << std::endl;
}

boost::gregorian::date_period GtfsParser::find_production_date(const std::string beginning_date) {
    std::fstream ifile((path + "/stop_times.txt").c_str());
    remove_bom(ifile);
    std::string line;
    if(!getline(ifile, line)) {
        std::cerr << "Impossible d'ouvrir le fichier trips.txt" << std::endl;
        throw std::exception();
    }

    boost::trim(line);
    Tokenizer tok_header(line);
    std::vector<std::string> elts(tok_header.begin(), tok_header.end());
    int trip_c = -1;
    for(size_t i = 0; i < elts.size(); i++){
        if (elts[i] == "trip_id")
            trip_c = i;
    }

    if (trip_c == -1){
        std::cerr << "Il manque au moinsla colonne trip_id dans stop_times.txt, pour trouver la production date " << std::endl
                  << "trip_id : " << (trip_c != -1) << std::endl;
        throw std::exception();
    }
    std::map<std::string, bool> trips;
    while(getline(ifile, line)) {
        boost::trim(line);
        if(line == "")
            continue;
        Tokenizer tok(line);
        std::vector<std::string> elts(tok.begin(), tok.end());
        trips.insert(std::make_pair(elts[trip_c], true));
    }


    ifile.close();
    ifile.open((path + "/trips.txt").c_str());
    remove_bom(ifile);
    if(!getline(ifile, line)) {
        std::cerr << "Impossible d'ouvrir le fichier trips.txt" << std::endl;
        throw std::exception();
    }

    boost::trim(line);
    tok_header.assign(line);
    elts.assign(tok_header.begin(), tok_header.end());
    int service_c = -1;
    trip_c = -1;
    for(size_t i = 0; i < elts.size(); i++){
        if (elts[i] == "service_id")
            service_c = i;
        else if(elts[i] == "trip_id")
            trip_c = i;
    }

    if (service_c == -1 || trip_c == -1){
        std::cerr << "Il manque au moins la colonne service_id ou la colonne trip_id dans trips.txt, pour trouver la production date" << std::endl
                  << "trip_id : " << (trip_c != -1) << " service_id : " << (service_c != -1) << std::endl;
        throw std::exception();
    }
    std::map<std::string, bool> services;
    while(getline(ifile, line)) {
        boost::trim(line);
        if(line == "")
            continue;
        Tokenizer tok(line);
        std::vector<std::string> elts(tok.begin(), tok.end());
        if(trips.find(elts[trip_c]) != trips.end())
            services.insert(std::make_pair(elts[service_c], true));
    }
    ifile.close();


    ifile.open((path + "/calendar.txt").c_str());
    remove_bom(ifile);
    boost::gregorian::date start_date(boost::gregorian::max_date_time), end_date(boost::gregorian::min_date_time);

    bool find_calendar = true;
    if(getline(ifile, line)){
        boost::trim(line);
        Tokenizer tok_header(line);
        std::vector<std::string> elts(tok_header.begin(), tok_header.end());
        int start_date_c = -1, end_date_c = -1, service_c = -1;
        for(size_t i = 0; i < elts.size(); i++) {
            if (elts[i] == "start_date")
                start_date_c = i;
            else if (elts[i] == "end_date")
                end_date_c = i;
            else if (elts[i] == "service_id")
                service_c = i;
        }
        if(start_date_c == -1 || end_date_c == -1 || service_c == -1){
            std::cerr << "Il manque au moins une colonne dans calendar.txt" << std::endl
                      << "start_date : " << (start_date_c!=-1) << " end_date : " << (end_date_c != -1) << " service_id : " << (service_c!=-1) << std::endl;
            throw std::exception();
        }

        while(getline(ifile, line)) {
            boost::trim(line);
            if(line == "")
                continue;
            Tokenizer tok(line);
            elts.assign(tok.begin(), tok.end());
            if(services.find(elts[service_c]) != services.end()) {
                boost::gregorian::date current_start_date = boost::gregorian::from_undelimited_string(elts[start_date_c]);
                boost::gregorian::date current_end_date = boost::gregorian::from_undelimited_string(elts[end_date_c]);

                if(current_start_date < start_date){
                    start_date = current_start_date;
                }
                if(current_end_date > end_date){
                    end_date = current_end_date;
                }
            }
        }
    } else {
        find_calendar = false;
    }
    ifile.close();
    ifile.open((path + "/calendar_dates.txt").c_str());
    remove_bom(ifile);

    if(!getline(ifile, line)) {
        if(!find_calendar) {
            std::cerr << "Impossible d'ouvrir les fichiers calendar.txt et calendar_dates.txt, on ne peut donc pas trouver la date de production" << std::endl;
            throw std::exception();
        }
    } else {
        boost::trim(line);
        tok_header.assign(line);
        elts.assign(tok_header.begin(), tok_header.end());
        int date_c = -1;
        service_c = -1;
        for(size_t i = 0; i < elts.size(); i++) {
            if (elts[i] == "date")
                date_c = i;
            else if (elts[i] == "service_id")
                service_c = i;
        }

        if(date_c == -1 || service_c == -1){
            std::cerr << "Il manque au moins une colonne dans calendar.txt" << std::endl
                      << "trip_id : " << (trip_c != -1) << " service_id : " << (service_c != -1) << std::endl;
            throw std::exception();
        }
        while(getline(ifile, line)) {
            boost::trim(line);
            if(line == "")
                continue;
            Tokenizer tok(line);
            elts.assign(tok.begin(), tok.end());
            if(services.find(elts[service_c]) != services.end()) {
                boost::gregorian::date current_date = boost::gregorian::from_undelimited_string(elts[date_c]);
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
    std::cout << "date de production: ";
    std::cout <<boost::gregorian::to_simple_string((start_date>b_date ? start_date : b_date)) << " - " << boost::gregorian::to_simple_string(end_date) << std::endl;
    return boost::gregorian::date_period((start_date>b_date ? start_date : b_date), end_date);
}


void  add_journey_pattern_point_connection(nm::JourneyPatternPoint *rp1, nm::JourneyPatternPoint *rp2, int length,
                           std::multimap<std::string, nm::JourneyPatternPointConnection> &journey_pattern_point_connections) {
    //Si la connexion n'existe pas encore alors on va la créer, sinon on regarde sa durée, si elle est inférieure, on la modifie
    auto pp = journey_pattern_point_connections.equal_range(rp1->uri);
    bool find = false;
    for(auto it_pp = pp.first; it_pp != pp.second; ++it_pp) {
        if(it_pp->second.destination_journey_pattern_point->uri == rp2->uri) {
            find = true;
            if(it_pp->second.length > length)
                it_pp->second.length = length;
            break;
        }
    }
    if(!find) {
        nm::JourneyPatternPointConnection rpc;
        rpc.departure_journey_pattern_point = rp1;
        rpc.destination_journey_pattern_point = rp2;
        rpc.journey_pattern_point_connection_kind = nm::JourneyPatternPointConnection::JourneyPatternPointConnectionKind::Extension;
        rpc.length = length;
        journey_pattern_point_connections.insert(std::make_pair(rp1->uri, rpc));

    }
}


void build_journey_pattern_point_connections(Data & data) {

    std::multimap<std::string, nm::VehicleJourney*> block_vj; 
    std::multimap<std::string, nm::JourneyPatternPointConnection> journey_pattern_point_connections;
    for(nm::VehicleJourney *vj: data.vehicle_journeys) {
        if(vj->block_id != "")
            block_vj.insert(std::make_pair(vj->block_id, vj));
    }

    std::string prec_block = "";
    for(auto it = block_vj.begin(); it!=block_vj.end(); ++it) {
        std::string block_id = it->first;
        if(prec_block != block_id) {
            auto pp = block_vj.equal_range(block_id);
            //On trie les vj appartenant au meme bloc par leur premier stop time
            std::vector<nm::VehicleJourney*> vjs;
            for(auto it_sub = pp.first; it_sub != pp.second; ++it_sub) {
                vjs.push_back(it_sub->second);
            }
            std::sort(vjs.begin(), vjs.end(), [](nm::VehicleJourney *  vj1, nm::VehicleJourney*vj2){
                                               return vj1->stop_time_list.front()->arrival_time < 
                                                      vj2->stop_time_list.front()->arrival_time; });

            //On crée les connexions entre le dernier journey_pattern point et le premier journey_pattern point
            auto prec_vj = vjs.begin();
            auto it_vj =vjs.begin() + 1;

            for(; it_vj!=vjs.end(); ++it_vj) {
                auto &st1 = (*prec_vj)->stop_time_list.back(),
                     &st2 = (*it_vj)->stop_time_list.front();
                if((st2->departure_time - st1->arrival_time) >= 0) {
                    add_journey_pattern_point_connection(st1->journey_pattern_point, st2->journey_pattern_point,
                                               (st2->departure_time - st1->arrival_time),
                                               journey_pattern_point_connections);
                }
                prec_vj = it_vj;
            }
            prec_block = block_id;
        }
    }


    //On ajoute les journey_pattern points dans data
    for(auto rpc : journey_pattern_point_connections) {
        data.journey_pattern_point_connections.push_back(new nm::JourneyPatternPointConnection(rpc.second));
    }

}


}}
