#include "data.h"
#include <iostream>
#include "ptreferential/where.h"
#include "utils/timer.h"

#include <boost/geometry.hpp>

using namespace navimake;

void Data::sort(){
    std::sort(networks.begin(), networks.end(), Less<navimake::types::Network>());
    std::for_each(networks.begin(), networks.end(), Indexer<navimake::types::Network>());

    std::sort(mode_types.begin(), mode_types.end(), Less<navimake::types::CommercialMode>());
    std::for_each(mode_types.begin(), mode_types.end(), Indexer<navimake::types::CommercialMode>());

    std::sort(modes.begin(), modes.end(), Less<navimake::types::PhysicalMode>());
    std::for_each(modes.begin(), modes.end(), Indexer<navimake::types::PhysicalMode>());

    std::sort(cities.begin(), cities.end(), Less<navimake::types::City>());
    std::for_each(cities.begin(), cities.end(), Indexer<navimake::types::City>());

    std::sort(lines.begin(), lines.end(), Less<navimake::types::Line>());
    std::for_each(lines.begin(), lines.end(), Indexer<navimake::types::Line>());

    std::sort(routes.begin(), routes.end(), Less<navimake::types::Route>());
    std::for_each(routes.begin(), routes.end(), Indexer<navimake::types::Route>());

    std::sort(stops.begin(), stops.end(), Less<navimake::types::StopTime>());
    std::for_each(stops.begin(), stops.end(), Indexer<navimake::types::StopTime>());

    std::sort(stop_areas.begin(), stop_areas.end(), Less<navimake::types::StopArea>());
    std::for_each(stop_areas.begin(), stop_areas.end(), Indexer<navimake::types::StopArea>());

    std::sort(stop_points.begin(), stop_points.end(), Less<navimake::types::StopPoint>());
    std::for_each(stop_points.begin(), stop_points.end(), Indexer<navimake::types::StopPoint>());

    std::sort(vehicle_journeys.begin(), vehicle_journeys.end(), Less<navimake::types::VehicleJourney>());
    std::for_each(vehicle_journeys.begin(), vehicle_journeys.end(), Indexer<navimake::types::VehicleJourney>());

    std::sort(connections.begin(), connections.end(), Less<navimake::types::Connection>());
    std::for_each(connections.begin(), connections.end(), Indexer<navimake::types::Connection>());

    std::sort(route_point_connections.begin(), route_point_connections.end(), Less<navimake::types::RoutePointConnection>());
    std::for_each(route_point_connections.begin(), route_point_connections.end(), Indexer<navimake::types::RoutePointConnection>());

    std::sort(route_points.begin(), route_points.end(), Less<navimake::types::RoutePoint>());
    std::for_each(route_points.begin(), route_points.end(), Indexer<navimake::types::RoutePoint>());

    std::sort(validity_patterns.begin(), validity_patterns.end(), Less<navimake::types::ValidityPattern>());
    std::for_each(validity_patterns.begin(), validity_patterns.end(), Indexer<navimake::types::ValidityPattern>());

    std::sort(departments.begin(), departments.end(), Less<navimake::types::Department>());
    std::for_each(departments.begin(), departments.end(), Indexer<navimake::types::Department>());

    std::sort(districts.begin(), districts.end(), Less<navimake::types::District>());
    std::for_each(districts.begin(), districts.end(), Indexer<navimake::types::District>());

}

void Data::complete() {
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
    std::unordered_map<std::string, vjs> route_vj;
    for(auto it = vehicle_journeys.begin(); it != vehicle_journeys.end(); ++it) {
        route_vj[(*it)->route->uri].push_back((*it));
    }

    int erase_overlap = 0, erase_emptiness = 0;

    for(auto it1 = route_vj.begin(); it1 != route_vj.end(); ++it1) {

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

                    for(auto rp = (*vj1)->route->route_point_list.begin(); rp != (*vj1)->route->route_point_list.end();++rp) {
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



void Data::transform(navitia::type::PT_Data& data){
    data.stop_areas.resize(this->stop_areas.size());
    std::transform(this->stop_areas.begin(), this->stop_areas.end(), data.stop_areas.begin(), navimake::types::StopArea::Transformer());

    data.physical_modes.resize(this->modes.size());
    std::transform(this->modes.begin(), this->modes.end(), data.physical_modes.begin(), navimake::types::PhysicalMode::Transformer());

    data.commercial_modes.resize(this->mode_types.size());
    std::transform(this->mode_types.begin(), this->mode_types.end(), data.commercial_modes.begin(), navimake::types::CommercialMode::Transformer());

    data.stop_points.resize(this->stop_points.size());
    std::transform(this->stop_points.begin(), this->stop_points.end(), data.stop_points.begin(), navimake::types::StopPoint::Transformer());

    data.lines.resize(this->lines.size());
    std::transform(this->lines.begin(), this->lines.end(), data.lines.begin(), navimake::types::Line::Transformer());

    data.cities.resize(this->cities.size());
    std::transform(this->cities.begin(), this->cities.end(), data.cities.begin(), navimake::types::City::Transformer());

    data.networks.resize(this->networks.size());
    std::transform(this->networks.begin(), this->networks.end(), data.networks.begin(), navimake::types::Network::Transformer());

    data.routes.resize(this->routes.size());
    std::transform(this->routes.begin(), this->routes.end(), data.routes.begin(), navimake::types::Route::Transformer());

    data.stop_times.resize(this->stops.size());
    std::transform(this->stops.begin(), this->stops.end(), data.stop_times.begin(), navimake::types::StopTime::Transformer());

    data.connections.resize(this->connections.size());
    std::transform(this->connections.begin(), this->connections.end(), data.connections.begin(), navimake::types::Connection::Transformer());

    data.route_point_connections.resize(this->route_point_connections.size());
    std::transform(this->route_point_connections.begin(), this->route_point_connections.end(), data.route_point_connections.begin(), navimake::types::RoutePointConnection::Transformer());

    data.route_points.resize(this->route_points.size());
    std::transform(this->route_points.begin(), this->route_points.end(), data.route_points.begin(), navimake::types::RoutePoint::Transformer());

    data.vehicle_journeys.resize(this->vehicle_journeys.size());
    std::transform(this->vehicle_journeys.begin(), this->vehicle_journeys.end(), data.vehicle_journeys.begin(), navimake::types::VehicleJourney::Transformer());

    data.validity_patterns.resize(this->validity_patterns.size());
    std::transform(this->validity_patterns.begin(), this->validity_patterns.end(), data.validity_patterns.begin(), navimake::types::ValidityPattern::Transformer());

    data.districts.resize(this->districts.size());
    std::transform(this->districts.begin(), this->districts.end(), data.districts.begin(), navimake::types::District::Transformer());

    data.departments.resize(this->departments.size());
    std::transform(this->departments.begin(), this->departments.end(), data.departments.begin(), navimake::types::Department::Transformer());
    build_relations(data);

}

void Data::build_relations(navitia::type::PT_Data &data){
    //BOOST_FOREACH(navimake::types::StopArea & sa, data.stop_areas){}

    BOOST_FOREACH(navitia::type::PhysicalMode & physical_mode, data.physical_modes){
        data.commercial_modes[physical_mode.commercial_mode_idx].physical_mode_list.push_back(physical_mode.idx);
    }

    //BOOST_FOREACH(navitia::type::ModeType & mode_type, data.mode_types){}

    BOOST_FOREACH(navitia::type::StopPoint & sp, data.stop_points){
        if(sp.stop_area_idx != navitia::type::invalid_idx) {
            navitia::type::StopArea & sa = data.stop_areas[sp.stop_area_idx];
            sa.stop_point_list.push_back(sp.idx);
            if(sp.city_idx != navitia::type::invalid_idx) {
                navitia::type::City & city = data.cities.at(sp.city_idx);
                city.stop_point_list.push_back(sp.idx);
                if(std::find(city.stop_area_list.begin(), city.stop_area_list.end(),sa.idx) == city.stop_area_list.end())
                    city.stop_area_list.push_back(sa.idx);
            }
        }

    }

    BOOST_FOREACH(navitia::type::Line & line, data.lines){
        if(line.commercial_mode_idx != navitia::type::invalid_idx)
            data.commercial_modes.at(line.commercial_mode_idx).line_list.push_back(line.idx);
        if(line.network_idx != navitia::type::invalid_idx)
            data.networks.at(line.network_idx).line_list.push_back(line.idx);
    }

    BOOST_FOREACH(navitia::type::City & city, data.cities){
        if(city.department_idx != navitia::type::invalid_idx)
            data.departments.at(city.department_idx).city_list.push_back(city.idx);
    }

    BOOST_FOREACH(navitia::type::District & district, data.districts){
        if(district.country_idx != navitia::type::invalid_idx)
            data.countries.at(district.country_idx).district_list.push_back(district.idx);
    }

    BOOST_FOREACH(navitia::type::Department & department, data.departments) {
        if(department.district_idx != navitia::type::invalid_idx)
            data.districts.at(department.district_idx).department_list.push_back(department.idx);
    }

    //BOOST_FOREACH(navitia::type::Network & network, data.networks){}

    //BOOST_FOREACH(navitia::type::Connection & connection, data.connections){}

    BOOST_FOREACH(navitia::type::RoutePoint & route_point, data.route_points){
        data.routes.at(route_point.route_idx).route_point_list.push_back(route_point.idx);
        data.stop_points.at(route_point.stop_point_idx).route_point_list.push_back(route_point.idx);
    }

    BOOST_FOREACH(navitia::type::StopTime & st, data.stop_times){
        data.vehicle_journeys.at(st.vehicle_journey_idx).stop_time_list.push_back(st.idx);
    }

    BOOST_FOREACH(navitia::type::Route & route, data.routes){
        if(route.commercial_mode_idx != navitia::type::invalid_idx)
            data.commercial_modes.at(route.commercial_mode_idx).line_list.push_back(route.line_idx);
        if(route.line_idx != navitia::type::invalid_idx)
            data.lines.at(route.line_idx).route_list.push_back(route.idx);
        std::sort(route.route_point_list.begin(), route.route_point_list.end(), sort_route_points_list(data));
    }

    BOOST_FOREACH(navitia::type::VehicleJourney & vj, data.vehicle_journeys){
        data.routes[vj.route_idx].vehicle_journey_list.push_back(vj.idx);

        navitia::type::Line & line = data.lines.at(data.routes.at(vj.route_idx).line_idx);
        if(std::find(line.physical_mode_list.begin(), line.physical_mode_list.end(), vj.physical_mode_idx) == line.physical_mode_list.end())
            line.physical_mode_list.push_back(vj.physical_mode_idx);

        if(vj.company_idx != navitia::type::invalid_idx){
            navitia::type::Company & company = data.companies.at(vj.company_idx);
            if(std::find(line.company_list.begin(), line.company_list.end(), vj.company_idx) == line.company_list.end())
                line.company_list.push_back(vj.company_idx);
            if(std::find(company.line_list.begin(), company.line_list.end(), line.idx) == company.line_list.end())
                company.line_list.push_back(line.idx);
        }

        std::sort(vj.stop_time_list.begin(), vj.stop_time_list.end());
    }

    BOOST_FOREACH(navitia::type::Route & route, data.routes){
        std::sort(route.vehicle_journey_list.begin(), route.vehicle_journey_list.end(), sort_vehicle_journey_list(data));
    }


        // BOOST_FOREACH(navitia::type::Company & company, data.companies){}
}

std::string Data::find_shape(navitia::type::PT_Data &data) {

    std::vector<navitia::type::GeographicalCoord> bag;
    for(navitia::type::StopPoint sp : data.stop_points) {
        bag.push_back(sp.coord);
    }
    boost::geometry::model::box<navitia::type::GeographicalCoord> envelope, buffer;
    boost::geometry::envelope(bag, envelope);
    boost::geometry::buffer(envelope, buffer, 0.01);

    std::ostringstream os;
    os << boost::geometry::wkt(buffer);
    return os.str();
}
