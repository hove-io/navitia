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

#include "data.h"
#include <iostream>
#include "ptreferential/where.h"
#include "utils/timer.h"
#include "utils/functions.h"
#include <boost/geometry.hpp>
#include <boost/geometry/geometry.hpp>



namespace nt = navitia::type;
namespace ed{

void Data::sort(){
#define SORT_AND_INDEX(type_name, collection_name) std::sort(collection_name.begin(), collection_name.end(), Less());\
    std::for_each(collection_name.begin(), collection_name.end(), Indexer<nt::idx_t>());
    ITERATE_NAVITIA_PT_TYPES(SORT_AND_INDEX)

    std::sort(stops.begin(), stops.end(), Less());
}

void Data::normalize_uri(){
    ::ed::normalize_uri(networks);
    ::ed::normalize_uri(companies);
    ::ed::normalize_uri(commercial_modes);
    ::ed::normalize_uri(lines);
    ::ed::normalize_uri(physical_modes);
    ::ed::normalize_uri(stop_areas);
    ::ed::normalize_uri(stop_points);
    ::ed::normalize_uri(vehicle_journeys);
    ::ed::normalize_uri(validity_patterns);
    ::ed::normalize_uri(calendars);
}

void Data::build_block_id() {
    /// We want to group vehicle journeys by their block_id
    /// Two vehicle_journeys vj1 are consecutive if
    /// the last arrival_time of vj1 <= to the departure_time of vj2
    std::sort(vehicle_journeys.begin(), vehicle_journeys.end(),
            [](const types::VehicleJourney* vj1, const types::VehicleJourney* vj2) {
            if(vj1->block_id != vj2->block_id) {
                return vj1->block_id < vj2->block_id;
            } else {
                return vj1->stop_time_list.back()->arrival_time <=
                        vj2->stop_time_list.front()->departure_time;
            }
        }
        );

    types::VehicleJourney* prev_vj = nullptr;
    for(auto it=vehicle_journeys.begin(); it!=vehicle_journeys.end(); ++it) {
        auto vj = *it;
        if(prev_vj && prev_vj->block_id != "" && prev_vj->block_id == vj->block_id) {
            prev_vj->next_vj = vj;
            vj->prev_vj = prev_vj;
        }
        prev_vj = vj;
    }
}

void Data::complete(){
    build_journey_patterns();
    build_journey_pattern_points();
    build_block_id();
    finalize_frequency();
    //on construit les codes externe des journey pattern
    ::ed::normalize_uri(journey_patterns);
    ::ed::normalize_uri(routes);

    //Ajoute les connections entre les stop points d'un meme stop area

    std::multimap<std::string, std::string> conns;
    for(auto conn : stop_point_connections) {
        conns.insert(std::make_pair(conn->departure->uri, conn->destination->uri));
    }

    std::multimap<std::string, types::StopPoint*> sa_sps;

    for(auto sp : stop_points) {
        if(sp->stop_area)
            sa_sps.insert(std::make_pair(sp->stop_area->uri, sp));
    }
    int connections_size = stop_point_connections.size();

    for(types::StopArea * sa : stop_areas) {
        auto ret = sa_sps.equal_range(sa->uri);
        for(auto sp1 = ret.first; sp1!= ret.second; ++sp1){
            for(auto sp2 = sp1; sp2!=ret.second; ++sp2) {
                if(sp1->second->uri != sp2->second->uri) {
                    bool found = false;
                    auto ret2 = conns.equal_range(sp1->second->uri);
                    for(auto itc = ret2.first; itc!= ret2.second; ++itc) {
                        if(itc->second == sp2->second->uri) {
                            found = true;
                            break;
                        }
                    }

                    if(!found) {
                        types::StopPointConnection * connection = new types::StopPointConnection();
                        connection->departure = sp1->second;
                        connection->destination  = sp2->second;
                        connection->connection_kind = types::ConnectionType::StopArea;
                        connection->duration = 240;
                        connection->display_duration = connection->duration - 120;
                        connection->uri = sp1->second->uri +"=>"+sp2->second->uri;
                        stop_point_connections.push_back(connection);
                        conns.insert(std::make_pair(connection->departure->uri, connection->destination->uri));
                    }

                    found = false;
                    ret2 = conns.equal_range(sp2->second->uri);
                    for(auto itc = ret2.first; itc!= ret2.second; ++itc) {
                        if(itc->second == sp1->second->uri) {
                            found = true;
                            break;
                        }
                    }

                    if(!found) {
                        types::StopPointConnection * connection = new types::StopPointConnection();
                        connection->departure = sp2->second;
                        connection->destination  = sp1->second;
                        connection->connection_kind = types::ConnectionType::StopArea;
                        connection->duration = 240;
                        connection->display_duration = connection->duration - 120;
                        connection->uri = sp2->second->uri +"=>"+sp1->second->uri;
                        stop_point_connections.push_back(connection);
                        conns.insert(std::make_pair(connection->departure->uri, connection->destination->uri));
                    }
                }
            }
        }
    }
    std::cout << "On a ajouté " << stop_point_connections.size() - connections_size << " connections lors de la completion" << std::endl;
}


void Data::clean(){
    auto logger = log4cplus::Logger::getInstance("log");

    std::set<std::string> toErase;

    typedef std::vector<ed::types::VehicleJourney *> vjs;
    std::unordered_map<std::string, vjs> journey_pattern_vj;
    for(auto it = vehicle_journeys.begin(); it != vehicle_journeys.end(); ++it) {
        journey_pattern_vj[(*it)->journey_pattern->uri].push_back((*it));
    }

    int erase_overlap = 0, erase_emptiness = 0, erase_no_circulation = 0;

    for(auto it1 = journey_pattern_vj.begin(); it1 != journey_pattern_vj.end(); ++it1) {

        for(auto vj1 = it1->second.begin(); vj1 != it1->second.end(); ++vj1) {
            if((*vj1)->stop_time_list.size() == 0) {
                toErase.insert((*vj1)->uri);
                ++erase_emptiness;
                continue;
            }
            if((*vj1)->validity_pattern->days.none() && (*vj1)->adapted_validity_pattern->days.none()){
                toErase.insert((*vj1)->uri);
                ++erase_no_circulation;
                continue;
            }
            for(auto vj2 = (vj1+1); vj2 != it1->second.end(); ++vj2) {
                if(((*vj1)->validity_pattern->days & (*vj2)->validity_pattern->days).any()  &&
                        (*vj1)->stop_time_list.size() > 0 && (*vj2)->stop_time_list.size() > 0) {
                    ed::types::VehicleJourney *vjs1, *vjs2;
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



    LOG4CPLUS_INFO(logger, "J'ai supprimé " + boost::lexical_cast<std::string>(erase_overlap) + "vehicle journey pour cause de dépassement, " +
                   boost::lexical_cast<std::string>(erase_emptiness) + " car il n'y avait pas de stop time dans le clean, et "
                   +boost::lexical_cast<std::string>(erase_no_circulation)+ " car ils ne circulaient jamais.");

    // Suppression des connections en doublons
    // On trie les connections par departure, destination
    auto sort_function = [](types::StopPointConnection * spc1, types::StopPointConnection *spc2) {return spc1->uri < spc2->uri
                                                                                                    || (spc1->uri == spc2->uri && spc1 < spc2 );};

    auto unique_function = [](types::StopPointConnection * spc1, types::StopPointConnection *spc2) {return spc1->uri == spc2->uri;};

    std::sort(stop_point_connections.begin(), stop_point_connections.end(), sort_function);
    num_elements = stop_point_connections.size();
    auto it_end = std::unique(stop_point_connections.begin(), stop_point_connections.end(), unique_function);
    //@TODO : Attention ça fuit, il faudrait réussir à effacer les objets
    //Ce qu'il y a dans la fin du vecteur apres unique n'est pas garanti, on ne peut pas itérer sur la suite pour effacer
    stop_point_connections.resize(std::distance(stop_point_connections.begin(), it_end));
    LOG4CPLUS_INFO(logger, "J'ai supprimé " + boost::lexical_cast<std::string>(num_elements-stop_point_connections.size()) + " doublons dans les connections");
}

// Functor qui sert à transformer un objet ed en objet navitia
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

    data.stop_point_connections.resize(this->stop_point_connections.size());
    std::transform(this->stop_point_connections.begin(), this->stop_point_connections.end(), data.stop_point_connections.begin(), Transformer());

    data.stop_times.resize(this->stops.size());
    std::transform(this->stops.begin(), this->stops.end(), data.stop_times.begin(), Transformer());

    build_relations(data);

}

void Data::build_relations(navitia::type::PT_Data &data){
    for(navitia::type::StopPoint* sp : data.stop_points){
        if(sp->stop_area != nullptr) {
            sp->stop_area->stop_point_list.push_back(sp);
        }
    }

    for(navitia::type::Line* line : data.lines){
        if(line->commercial_mode != nullptr)
            line->commercial_mode->line_list.push_back(line);
        if(line->network!= nullptr)
            line->network->line_list.push_back(line);
    }

    //for(navitia::type::Network & network: data.networks){}

    for(navitia::type::JourneyPatternPoint* journey_pattern_point : data.journey_pattern_points){
        journey_pattern_point->journey_pattern->journey_pattern_point_list.push_back(journey_pattern_point);
        journey_pattern_point->stop_point->journey_pattern_point_list.push_back(journey_pattern_point);
    }

    //for(navitia::type::StopTime & st : data.stop_times){
    for(size_t i = 0; i < data.stop_times.size(); ++i){
        auto st = data.stop_times[i];
        st->vehicle_journey->stop_time_list.push_back(st);
    }

    for(navitia::type::JourneyPattern* journey_pattern : data.journey_patterns){
        if(journey_pattern->route != nullptr)
            journey_pattern->route->journey_pattern_list.push_back(journey_pattern);
        auto comp = [](const nt::JourneyPatternPoint* jpp1, const nt::JourneyPatternPoint* jpp2){return jpp1->order < jpp2->order;};
        std::sort(journey_pattern->journey_pattern_point_list.begin(), journey_pattern->journey_pattern_point_list.end(), comp);
        std::sort(journey_pattern->vehicle_journey_list.begin(), journey_pattern->vehicle_journey_list.end(), sort_vehicle_journey_list(data));
    }

    for(navitia::type::Route* route : data.routes){
        if(route->line != nullptr)
            route->line->route_list.push_back(route);
    }

    for(navitia::type::VehicleJourney* vj : data.vehicle_journeys){
        vj->journey_pattern->vehicle_journey_list.push_back(vj);

        if(vj->journey_pattern != nullptr){
            navitia::type::JourneyPattern* jp = vj->journey_pattern;
            if(jp->route != nullptr){
                navitia::type::Route* route = jp->route;
                if(route != nullptr) {
                    navitia::type::Line* line = route->line;
                    if(vj->company != nullptr){
                        if(std::find(line->company_list.begin(), line->company_list.end(), vj->company) == line->company_list.end())
                            line->company_list.push_back(vj->company);
                        if(std::find(vj->company->line_list.begin(), vj->company->line_list.end(), line) == vj->company->line_list.end())
                            vj->company->line_list.push_back(line);
                    }
                }
            }
        }
        std::sort(vj->stop_time_list.begin(), vj->stop_time_list.end());
    }

   //for(navitia::type::Company & company : data.companies) {}
}

std::string Data::compute_bounding_box(navitia::type::PT_Data &data) {

    std::vector<navitia::type::GeographicalCoord> bag;
    for(const navitia::type::StopPoint* sp : data.stop_points) {
        bag.push_back(sp->coord);
    }
    using coord_box = boost::geometry::model::box<navitia::type::GeographicalCoord>;
    coord_box envelope, buffer;
    boost::geometry::envelope(bag, envelope);
    boost::geometry::buffer(envelope, buffer, 0.01);

    std::ostringstream os;
    os << "{\"type\": \"Polygon\", \"coordinates\": [[";
    std::string sep = "";

    boost::geometry::box_view<coord_box> view {buffer};
    for (auto coord : view) {
        os << sep << "[" << coord.lon() << ", " << coord.lat() << "]";
        sep = ",";
    }
    os << "]]}";
    return os.str();
}

// TODO : pour l'instant on construit une route par journey pattern
// Il faudrait factoriser les routes
void Data::build_journey_patterns(){
    auto logger = log4cplus::Logger::getInstance("log");
    LOG4CPLUS_TRACE(logger, "On calcule les journey_patterns");

    // Associe à chaque line uri le nombre de journey_pattern trouvées jusqu'à present
    for(auto it1 = this->vehicle_journeys.begin(); it1 != this->vehicle_journeys.end(); ++it1){
        types::VehicleJourney * vj1 = *it1;
        ed::types::Route* route = vj1->tmp_route;
        // Si le vj n'appartient encore à aucune journey_pattern
        if(vj1->journey_pattern == 0) {
            std::string journey_pattern_uri = vj1->tmp_line->uri;
            journey_pattern_uri += "-";
            journey_pattern_uri += boost::lexical_cast<std::string>(this->journey_patterns.size());

            types::JourneyPattern * journey_pattern = new types::JourneyPattern();
            journey_pattern->uri = journey_pattern_uri;
            if(route == nullptr){
                route = new types::Route();
                route->line = vj1->tmp_line;
                route->uri = journey_pattern->uri;
                route->name = journey_pattern->name;
                this->routes.push_back(route);
            }
            journey_pattern->route = route;
            journey_pattern->physical_mode = vj1->physical_mode;
            vj1->journey_pattern = journey_pattern;
            this->journey_patterns.push_back(journey_pattern);

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
    std::map<std::string, ed::types::JourneyPatternPoint*> journey_pattern_point_map;

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


void Data::finalize_frequency() {
    for(auto * vj : this->vehicle_journeys) {
        if(!vj->stop_time_list.empty() && vj->stop_time_list.front()->is_frequency) {
            auto * first_st = vj->stop_time_list.front();
            size_t nb_trips = std::ceil((first_st->end_time - first_st->start_time)/first_st->headway_secs);
            for(auto * st : vj->stop_time_list) {
                st->start_time = first_st->start_time+(st->arrival_time - first_st->arrival_time);
                st->end_time = first_st->start_time + nb_trips * st->headway_secs;
                st->end_time += (st->departure_time - first_st->departure_time);
            }
        }
    }
}

Georef::~Georef(){
    for(auto itm : this->nodes)
         delete itm.second;
    for(auto itm : this->edges)
         delete itm.second;
    for(auto itm : this->ways)
         delete itm.second;
    for(auto itm : this->house_numbers)
         delete itm.second;
    for(auto itm : this->admins)
         delete itm.second;
}

PoiPoiType::~PoiPoiType(){
    for(auto itm : this->poi_types)
         delete itm.second;
    for(auto itm : this->pois)
         delete itm.second;
}
}//namespace
