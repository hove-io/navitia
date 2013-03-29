#include "data.h"
#include <iostream>
#include "ptreferential/where.h"
#include "utils/timer.h"

#include <boost/geometry.hpp>

namespace navimake{

void Data::sort(){
#define SORT_AND_INDEX(type_name, collection_name) std::sort(collection_name.begin(), collection_name.end(), Less());\
    std::for_each(collection_name.begin(), collection_name.end(), Indexer());
    ITERATE_NAVITIA_PT_TYPES(SORT_AND_INDEX)

    std::sort(stops.begin(), stops.end(), Less());
    std::for_each(stops.begin(), stops.end(), Indexer());

    std::sort(journey_pattern_point_connections.begin(), journey_pattern_point_connections.end(), Less());
    std::for_each(journey_pattern_point_connections.begin(), journey_pattern_point_connections.end(), Indexer());
}

void Data::normalize_uri(){
    ::navimake::normalize_uri(networks);
    ::navimake::normalize_uri(companies);
    ::navimake::normalize_uri(commercial_modes);
    ::navimake::normalize_uri(lines);
    ::navimake::normalize_uri(physical_modes);
    ::navimake::normalize_uri(cities);
    ::navimake::normalize_uri(countries);
    ::navimake::normalize_uri(stop_areas);
    ::navimake::normalize_uri(stop_points);
    ::navimake::normalize_uri(vehicle_journeys);
    ::navimake::normalize_uri(districts);
    ::navimake::normalize_uri(departments);
    ::navimake::normalize_uri(validity_patterns);
}

void Data::complete(){
    build_journey_patterns();
    build_journey_pattern_points();
    build_journey_pattern_point_connections();
    //on construit les codes externe des journey pattern
    ::navimake::normalize_uri(journey_patterns);
    ::navimake::normalize_uri(routes);

    //Ajoute les connections entre les stop points d'un meme stop area

    std::multimap<std::string, std::string> conns;
    for(auto conn : connections) {
        conns.insert(std::make_pair(conn->departure_stop_point->uri, conn->destination_stop_point->uri));
    }

    std::multimap<std::string, types::StopPoint*> sa_sps;

    for(auto sp : stop_points) {
        if(sp->stop_area)
            sa_sps.insert(std::make_pair(sp->stop_area->uri, sp));
    }
    int connections_size = connections.size();

    std::string prec_sa = "";
    for(auto sa_sp : sa_sps) {
        if(prec_sa != sa_sp.first) {
            auto ret = sa_sps.equal_range(sa_sp.first);
            for(auto it = ret.first; it!= ret.second; ++it){
                for(auto it2 = it; it2!=ret.second; ++it2) {
                    if(it->second->uri != it2->second->uri) {
                        bool found = false;
                        auto ret2 = conns.equal_range(it->second->uri);
                        for(auto itc = ret2.first; itc!= ret2.second; ++itc) {
                            if(itc->second == it2->second->uri) {
                                found = true;
                                break;
                            }
                        }

                        if(!found) {
                            types::Connection * connection = new types::Connection();
                            connection->departure_stop_point = it->second;
                            connection->destination_stop_point  = it2->second;
                            connection->connection_kind = types::Connection::StopAreaConnection;
                            connection->duration = 120;
                            connections.push_back(connection);
                        }

                        found = false;
                        ret2 = conns.equal_range(it2->second->uri);
                        for(auto itc = ret2.first; itc!= ret2.second; ++itc) {
                            if(itc->second == it->second->uri) {
                                found = true;
                                break;
                            }
                        }

                        if(!found) {
                            types::Connection * connection = new types::Connection();
                            connection->departure_stop_point = it2->second;
                            connection->destination_stop_point  = it->second;
                            connection->connection_kind = types::Connection::StopAreaConnection;
                            connection->duration = 120;
                            connections.push_back(connection);
                        }
                    }
                }
            }
            prec_sa = sa_sp.first;
        }
    }

    std::cout << "On a ajouté " << (connections.size() - connections_size) << " connections lors de la completion" << std::endl;


}


void Data::clean(){

    std::set<std::string> toErase;

    typedef std::vector<navimake::types::VehicleJourney *> vjs;
    std::unordered_map<std::string, vjs> journey_pattern_vj;
    for(auto it = vehicle_journeys.begin(); it != vehicle_journeys.end(); ++it) {
        journey_pattern_vj[(*it)->journey_pattern->uri].push_back((*it));
    }

    int erase_overlap = 0, erase_emptiness = 0;

    for(auto it1 = journey_pattern_vj.begin(); it1 != journey_pattern_vj.end(); ++it1) {

        for(auto vj1 = it1->second.begin(); vj1 != it1->second.end(); ++vj1) {
            if((*vj1)->stop_time_list.size() == 0) {
                toErase.insert((*vj1)->uri);
                ++erase_emptiness;
                continue;
            }
            for(auto vj2 = (vj1+1); vj2 != it1->second.end(); ++vj2) {
                if(((*vj1)->validity_pattern->days & (*vj2)->validity_pattern->days).any()  &&
                        (*vj1)->stop_time_list.size() > 0 && (*vj2)->stop_time_list.size() > 0) {
                    navimake::types::VehicleJourney *vjs1, *vjs2;
                    if((*vj1)->stop_time_list.front()->departure_time <= (*vj2)->stop_time_list.front()->departure_time) {
                        vjs1 = *vj1;
                        vjs2 = *vj2;
                    }
                    else {
                        vjs1 = *vj2;
                        vjs2 = *vj1;
                    }

                    for(auto rp = (*vj1)->journey_pattern->journey_pattern_point_list.begin(); rp != (*vj1)->journey_pattern->journey_pattern_point_list.end();++rp) {
                        if(vjs1->stop_time_list.at((*rp)->order)->departure_time > vjs2->stop_time_list.at((*rp)->order)->departure_time) {
                            toErase.insert((*vj2)->uri);
                            ++erase_overlap;
                            break;
                        }
                    }
                }
            }
        }
    }

    std::vector<size_t> erasest;

    for(int i=stops.size()-1; i >=0;--i) {
        auto it = toErase.find(stops[i]->vehicle_journey->uri);
        if(it != toErase.end()) {
            erasest.push_back(i);
        }
    }

    // Pour chaque stopTime à supprimer on le détruit, et on remet le dernier élément du vector à la place
    // On ne redimentionne pas tout de suite le tableau pour des raisons de perf
    size_t num_elements = stops.size();
    for(size_t to_erase : erasest) {
        delete stops[to_erase];
        stops[to_erase] = stops[num_elements - 1];
        num_elements--;
    }


    stops.resize(num_elements);

    erasest.clear();
    for(int i=vehicle_journeys.size()-1; i >= 0;--i){
        auto it = toErase.find(vehicle_journeys[i]->uri);
        if(it != toErase.end()) {
            erasest.push_back(i);
        }
    }

    //Meme chose mais avec les vj
    num_elements = vehicle_journeys.size();
    for(size_t to_erase : erasest) {
        delete vehicle_journeys[to_erase];
        vehicle_journeys[to_erase] = vehicle_journeys[num_elements - 1];
        num_elements--;
    }
    vehicle_journeys.resize(num_elements);

    std::cout << "J'ai supprimé " << erase_overlap << "vehicle journey pour cause de dépassement, et " <<
                 erase_emptiness << " car il n'y avait pas de stop time dans le clean" << std::endl;

}

// Functor qui sert à transformer un objet navimake en objet navitia
// Il appelle la methode get_navitia_type
struct Transformer {
    template<class T> auto operator()(T * object) -> decltype(object->get_navitia_type()){
        return object->get_navitia_type();
    }
};

void Data::transform(navitia::type::PT_Data& data){
#define RESIZE_AND_TRANSFORM(type_name, collection_name) data.collection_name.resize(this->collection_name.size());\
    std::transform(this->collection_name.begin(), this->collection_name.end(), data.collection_name.begin(), Transformer());
    ITERATE_NAVITIA_PT_TYPES(RESIZE_AND_TRANSFORM)

    data.journey_pattern_point_connections.resize(this->journey_pattern_point_connections.size());
    std::transform(this->journey_pattern_point_connections.begin(), this->journey_pattern_point_connections.end(), data.journey_pattern_point_connections.begin(), Transformer());

    data.stop_times.resize(this->stops.size());
    std::transform(this->stops.begin(), this->stops.end(), data.stop_times.begin(), Transformer());

    build_relations(data);

}

void Data::build_relations(navitia::type::PT_Data &data){
    for(navitia::type::StopPoint & sp : data.stop_points){
        if(sp.stop_area_idx != navitia::type::invalid_idx) {
            navitia::type::StopArea & sa = data.stop_areas[sp.stop_area_idx];
            sa.stop_point_list.push_back(sp.idx);
        }
        if(sp.city_idx != navitia::type::invalid_idx) {
            navitia::type::City & city = data.cities.at(sp.city_idx);
            city.stop_point_list.push_back(sp.idx);
        }
    }

    for(navitia::type::Line & line : data.lines){
        if(line.commercial_mode_idx != navitia::type::invalid_idx)
            data.commercial_modes.at(line.commercial_mode_idx).line_list.push_back(line.idx);
        if(line.network_idx != navitia::type::invalid_idx)
            data.networks.at(line.network_idx).line_list.push_back(line.idx);
    }

    for(navitia::type::City & city : data.cities){
        if(city.department_idx != navitia::type::invalid_idx)
            data.departments.at(city.department_idx).city_list.push_back(city.idx);
    }

    for(navitia::type::District & district : data.districts){
        if(district.country_idx != navitia::type::invalid_idx)
            data.countries.at(district.country_idx).district_list.push_back(district.idx);
    }

    for(navitia::type::Department & department : data.departments) {
        if(department.district_idx != navitia::type::invalid_idx)
            data.districts.at(department.district_idx).department_list.push_back(department.idx);
    }

    //BOOST_FOREACH(navitia::type::Network & network, data.networks){}

    //BOOST_FOREACH(navitia::type::Connection & connection, data.connections){}

    for(navitia::type::JourneyPatternPoint & journey_pattern_point : data.journey_pattern_points){
        data.journey_patterns.at(journey_pattern_point.journey_pattern_idx).journey_pattern_point_list.push_back(journey_pattern_point.idx);
        data.stop_points.at(journey_pattern_point.stop_point_idx).journey_pattern_point_list.push_back(journey_pattern_point.idx);
    }

    for(navitia::type::StopTime & st : data.stop_times){
        data.vehicle_journeys.at(st.vehicle_journey_idx).stop_time_list.push_back(st.idx);
    }

    for(navitia::type::JourneyPattern & journey_pattern : data.journey_patterns){
        if(journey_pattern.route_idx != navitia::type::invalid_idx)
            data.routes.at(journey_pattern.route_idx).journey_pattern_list.push_back(journey_pattern.idx);
        std::sort(journey_pattern.journey_pattern_point_list.begin(), journey_pattern.journey_pattern_point_list.end(), sort_journey_pattern_points_list(data));
        std::sort(journey_pattern.vehicle_journey_list.begin(), journey_pattern.vehicle_journey_list.end(), sort_vehicle_journey_list(data));
    }

    for(navitia::type::Route & route : data.routes){
        if(route.line_idx != navitia::type::invalid_idx)
            data.lines.at(route.line_idx).route_list.push_back(route.idx);
    }

    for(navitia::type::VehicleJourney & vj : data.vehicle_journeys){
        data.journey_patterns[vj.journey_pattern_idx].vehicle_journey_list.push_back(vj.idx);


        if(vj.journey_pattern_idx != navitia::type::invalid_idx){
            navitia::type::JourneyPattern jp = data.journey_patterns.at(vj.journey_pattern_idx);
            if(jp.route_idx != navitia::type::invalid_idx){
                navitia::type::Route route = data.routes.at(jp.route_idx);
                if(route.line_idx != navitia::type::invalid_idx){
                    navitia::type::Line & line = data.lines.at(route.line_idx);
                    if(vj.company_idx != navitia::type::invalid_idx){
                        navitia::type::Company & company = data.companies.at(vj.company_idx);
                        if(std::find(line.company_list.begin(), line.company_list.end(), vj.company_idx) == line.company_list.end())
                            line.company_list.push_back(vj.company_idx);
                        if(std::find(company.line_list.begin(), company.line_list.end(), line.idx) == company.line_list.end())
                            company.line_list.push_back(line.idx);
                    }
                }
            }
        }
        std::sort(vj.stop_time_list.begin(), vj.stop_time_list.end());
    }

   //for(navitia::type::Company & company : data.companies) {}
}

std::string Data::compute_bounding_box(navitia::type::PT_Data &data) {

    std::vector<navitia::type::GeographicalCoord> bag;
    for(navitia::type::StopPoint sp : data.stop_points) {
        bag.push_back(sp.coord);
    }
    boost::geometry::model::box<navitia::type::GeographicalCoord> envelope, buffer;
    boost::geometry::envelope(bag, envelope);
    boost::geometry::buffer(envelope, buffer, 0.01);

    std::ostringstream os;
    os << "{\"type\": \"Polygon\", \"coordinates\": [[";
    typedef boost::geometry::box_view<decltype(buffer)> box_view;
    std::string sep = "";
    auto functor = [&os, &sep](navitia::type::GeographicalCoord coord){os << sep << "[" << coord.lon() << ", " << coord.lat() << "]"; sep = ",";};
    boost::geometry::for_each_point(box_view(buffer), functor);
    os << "]]}";
    return os.str();
}

// TODO : pour l'instant on construit une route par journey pattern
// Il faudrait factoriser les routes
void Data::build_journey_patterns(){
    auto logger = log4cplus::Logger::getInstance("log");
    LOG4CPLUS_TRACE(logger, "On calcule les journey_patterns");

    // Associe à chaque line uri le nombre de journey_pattern trouvées jusqu'à present
    std::map<std::string, int> line_journey_patterns_count;
    for(auto it1 = this->vehicle_journeys.begin(); it1 != this->vehicle_journeys.end(); ++it1){
        types::VehicleJourney * vj1 = *it1;
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

            types::Route * route = new types::Route();
            types::JourneyPattern * journey_pattern = new types::JourneyPattern();
            journey_pattern->uri = vj1->tmp_line->uri + "-" + boost::lexical_cast<std::string>(count);
            journey_pattern->route = route;
            journey_pattern->physical_mode = vj1->physical_mode;
            vj1->journey_pattern = journey_pattern;
            this->journey_patterns.push_back(journey_pattern);

            route->line = vj1->tmp_line;
            route->uri = journey_pattern->uri;
            route->name = journey_pattern->name;
            this->routes.push_back(route);

            for(auto it2 = it1 + 1; it1 != this->vehicle_journeys.end() && it2 != this->vehicle_journeys.end(); ++it2){
                types::VehicleJourney * vj2 = *it2;
                if(vj2->journey_pattern == 0 && same_journey_pattern(vj1, vj2)){
                    vj2->journey_pattern = vj1->journey_pattern;
                }
            }
        }
    }
    LOG4CPLUS_TRACE(logger, "Nombre de journey_patterns : " +boost::lexical_cast<std::string>(this->journey_patterns.size()));
}


void Data::build_journey_pattern_points(){
    auto logger = log4cplus::Logger::getInstance("log");
    LOG4CPLUS_TRACE(logger, "Construction des journey_pattern points");
    std::map<std::string, navimake::types::JourneyPatternPoint*> journey_pattern_point_map;

    int stop_seq;
    for(types::VehicleJourney * vj : this->vehicle_journeys){
        stop_seq = 0;
        for(types::StopTime * stop_time : vj->stop_time_list){
            std::string journey_pattern_point_extcode = vj->journey_pattern->uri + ":" + stop_time->tmp_stop_point->uri+":"+boost::lexical_cast<std::string>(stop_seq);
            auto journey_pattern_point_it = journey_pattern_point_map.find(journey_pattern_point_extcode);
            types::JourneyPatternPoint * journey_pattern_point;
            if(journey_pattern_point_it == journey_pattern_point_map.end()) {
                journey_pattern_point = new types::JourneyPatternPoint();
                journey_pattern_point->journey_pattern = vj->journey_pattern;
                journey_pattern_point->journey_pattern->journey_pattern_point_list.push_back(journey_pattern_point);
                journey_pattern_point->stop_point = stop_time->tmp_stop_point;
                journey_pattern_point_map[journey_pattern_point_extcode] = journey_pattern_point;
                journey_pattern_point->order = stop_seq;
                journey_pattern_point->uri = journey_pattern_point_extcode;
                this->journey_pattern_points.push_back(journey_pattern_point);
            } else {
                journey_pattern_point = journey_pattern_point_it->second;
            }
            ++stop_seq;
            stop_time->journey_pattern_point = journey_pattern_point;
        }
    }

    for(types::JourneyPattern *journey_pattern : this->journey_patterns){
        if(! journey_pattern->journey_pattern_point_list.empty()){
            types::JourneyPatternPoint * last = journey_pattern->journey_pattern_point_list.back();
            if(last->stop_point->stop_area != NULL)
                journey_pattern->name = last->stop_point->stop_area->name;
            else
                journey_pattern->name = last->stop_point->name;
            journey_pattern->route->name = journey_pattern->name;
        }
    }
    LOG4CPLUS_TRACE(logger, "Nombre de journey_pattern points : "+ boost::lexical_cast<std::string>(this->journey_pattern_points.size()));
}

void Data::build_journey_pattern_point_connections(){
    std::multimap<std::string, types::VehicleJourney*> block_vj; 
    std::multimap<std::string, types::JourneyPatternPointConnection> journey_pattern_point_connections;
    for(types::VehicleJourney *vj: this->vehicle_journeys) {
        if(vj->block_id != "")
            block_vj.insert(std::make_pair(vj->block_id, vj));
    }

    std::string prec_block = "";
    for(auto it = block_vj.begin(); it!=block_vj.end(); ++it) {
        std::string block_id = it->first;
        if(block_id != "" && prec_block != block_id) {
            auto pp = block_vj.equal_range(block_id);
            //On trie les vj appartenant au meme bloc par leur premier stop time
            std::vector<types::VehicleJourney*> vjs;
            for(auto it_sub = pp.first; it_sub != pp.second; ++it_sub) {
                vjs.push_back(it_sub->second);
            }
            std::sort(vjs.begin(), vjs.end(), [](types::VehicleJourney *  vj1, types::VehicleJourney*vj2){
                if(!vj1->stop_time_list.empty() && !vj2->stop_time_list.empty()) {
                    return (vj1->stop_time_list.front()->arrival_time < vj2->stop_time_list.front()->arrival_time);
                } else {
                    return !vj1->stop_time_list.empty();
                }
            });
            //On crée les connexions entre le dernier journey_pattern point et le premier journey_pattern point
            auto prec_vj = vjs.begin();
            auto it_vj =vjs.begin() + 1;

            for(; it_vj!=vjs.end(); ++it_vj) {
                if(!((*prec_vj)->stop_time_list.empty()) && (!(*it_vj)->stop_time_list.empty())) {
                    auto &st1 = (*prec_vj)->stop_time_list.back(),
                         &st2 = (*it_vj)->stop_time_list.front();
                    if((st2->departure_time - st1->arrival_time) >= 0) {
                        add_journey_pattern_point_connection(st1->journey_pattern_point, st2->journey_pattern_point,
                                (st2->departure_time - st1->arrival_time),
                                journey_pattern_point_connections);
                    }
                }
                prec_vj = it_vj;
            }
            prec_block = block_id;
        }
    }

    //On ajoute les journey_pattern points dans data
    for(auto rpc : journey_pattern_point_connections) {
        this->journey_pattern_point_connections.push_back(new types::JourneyPatternPointConnection(rpc.second));
    }
}

// Compare si deux vehicle journey appartiennent à la même journey_pattern
bool same_journey_pattern(types::VehicleJourney * vj1, types::VehicleJourney * vj2){
    if(vj1->stop_time_list.size() != vj2->stop_time_list.size())
        return false;
    for(size_t i = 0; i < vj1->stop_time_list.size(); ++i)
        if(vj1->stop_time_list[i]->tmp_stop_point != vj2->stop_time_list[i]->tmp_stop_point){
            return false;
        }
    return true;
}

void  add_journey_pattern_point_connection(types::JourneyPatternPoint *rp1, types::JourneyPatternPoint *rp2, int length,
                           std::multimap<std::string, types::JourneyPatternPointConnection> &journey_pattern_point_connections) {
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
        types::JourneyPatternPointConnection rpc;
        rpc.departure_journey_pattern_point = rp1;
        rpc.destination_journey_pattern_point = rp2;
        rpc.journey_pattern_point_connection_kind = types::JourneyPatternPointConnection::JourneyPatternPointConnectionKind::Extension;
        rpc.length = length;
        journey_pattern_point_connections.insert(std::make_pair(rp1->uri, rpc));

    }
}

}//namespace
