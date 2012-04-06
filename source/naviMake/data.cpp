#include "data.h"
#include <iostream>
#include "ptreferential/where.h"

using namespace navimake;

void Data::sort(){

    std::sort(networks.begin(), networks.end(), Less<navimake::types::Network>());
    std::for_each(networks.begin(), networks.end(), Indexer<navimake::types::Network>());

    std::sort(mode_types.begin(), mode_types.end(), Less<navimake::types::ModeType>());
    std::for_each(mode_types.begin(), mode_types.end(), Indexer<navimake::types::ModeType>());

    std::sort(modes.begin(), modes.end(), Less<navimake::types::Mode>());
    std::for_each(modes.begin(), modes.end(), Indexer<navimake::types::Mode>());

    std::sort(networks.begin(), networks.end(), Less<navimake::types::Network>());
    std::for_each(networks.begin(), networks.end(), Indexer<navimake::types::Network>());

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

    std::sort(route_points.begin(), route_points.end(), Less<navimake::types::RoutePoint>());
    std::for_each(route_points.begin(), route_points.end(), Indexer<navimake::types::RoutePoint>());

    std::sort(validity_patterns.begin(), validity_patterns.end(), Less<navimake::types::ValidityPattern>());
    std::for_each(validity_patterns.begin(), validity_patterns.end(), Indexer<navimake::types::ValidityPattern>());


}

void Data::clean(){
}



void Data::transform(navitia::type::PT_Data& data){
    data.stop_areas.resize(this->stop_areas.size());
    std::transform(this->stop_areas.begin(), this->stop_areas.end(), data.stop_areas.begin(), navimake::types::StopArea::Transformer());

    data.modes.resize(this->modes.size());
    std::transform(this->modes.begin(), this->modes.end(), data.modes.begin(), navimake::types::Mode::Transformer());

    data.mode_types.resize(this->mode_types.size());
    std::transform(this->mode_types.begin(), this->mode_types.end(), data.mode_types.begin(), navimake::types::ModeType::Transformer());

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

    data.route_points.resize(this->route_points.size());
    std::transform(this->route_points.begin(), this->route_points.end(), data.route_points.begin(), navimake::types::RoutePoint::Transformer());

    data.vehicle_journeys.resize(this->vehicle_journeys.size());
    std::transform(this->vehicle_journeys.begin(), this->vehicle_journeys.end(), data.vehicle_journeys.begin(), navimake::types::VehicleJourney::Transformer());

    data.validity_patterns.resize(this->validity_patterns.size());
    std::transform(this->validity_patterns.begin(), this->validity_patterns.end(), data.validity_patterns.begin(), navimake::types::ValidityPattern::Transformer());

    build_relations(data);
}

void Data::build_relations(navitia::type::PT_Data &data){
    //BOOST_FOREACH(navimake::types::StopArea & sa, data.stop_areas){}

    BOOST_FOREACH(navitia::type::Mode & mode, data.modes){
        data.mode_types[mode.mode_type_idx].mode_list.push_back(mode.idx);
    }

    //BOOST_FOREACH(navitia::type::ModeType & mode_type, data.mode_types){}

    BOOST_FOREACH(navitia::type::StopPoint & sp, data.stop_points){
        navitia::type::StopArea & sa = data.stop_areas[sp.stop_area_idx];
        try{
            navitia::type::City & city = data.cities.at(sp.city_idx);
            sa.stop_point_list.push_back(sp.idx);
            city.stop_point_list.push_back(sp.idx);
            if(std::find(city.stop_area_list.begin(), city.stop_area_list.end(),sa.idx) == city.stop_area_list.end())
                city.stop_area_list.push_back(sa.idx);
        }catch(std::out_of_range ex){//aucune city lié au stopPoint
            continue;
        }
    }

    BOOST_FOREACH(navitia::type::Line & line, data.lines){
        try{
            data.mode_types.at(line.mode_type_idx).line_list.push_back(line.idx);
        }catch(std::out_of_range ex){}
        try{
            data.networks.at(line.network_idx).line_list.push_back(line.idx);
        }catch(std::out_of_range ex){}
    }

    BOOST_FOREACH(navitia::type::City & city, data.cities){
        try{
            data.departments.at(city.department_idx).city_list.push_back(city.idx);
        }catch(std::out_of_range ex){}
    }

    BOOST_FOREACH(navitia::type::District & district, data.districts){
        try{
            data.countries.at(district.country_idx).district_list.push_back(district.idx);
        }catch(std::out_of_range ex){}
    }

    BOOST_FOREACH(navitia::type::Department & department, data.departments) {
        try{
            data.districts.at(department.district_idx).department_list.push_back(department.idx);
        }catch(std::out_of_range ex){}
    }

    //BOOST_FOREACH(navitia::type::Network & network, data.networks){}

    BOOST_FOREACH(navitia::type::Route & route, data.routes){
        try{
            data.mode_types.at(route.mode_type_idx).line_list.push_back(route.line_idx);
        }catch(std::out_of_range ex){}
    }

    //BOOST_FOREACH(navitia::type::StopTime & stop_time, data.stop_times){}

    //BOOST_FOREACH(navitia::type::Connection & connection, data.connections){}

    BOOST_FOREACH(navitia::type::RoutePoint & route_point, data.route_points){
        try{
            data.routes.at(route_point.route_idx).route_point_list.push_back(route_point.idx);
        }catch(std::out_of_range ex){}
        data.stop_points.at(route_point.stop_point_idx).route_point_list.push_back(route_point.idx);
    }



    BOOST_FOREACH(navitia::type::VehicleJourney & vj, data.vehicle_journeys){
        try{
            navitia::type::Line & line = data.lines.at(data.routes.at(vj.route_idx).line_idx);


            line.validity_pattern_list.push_back(vj.validity_pattern_idx);

            data.routes[vj.route_idx].vehicle_journey_list.push_back(vj.idx);

            if(std::find(line.mode_list.begin(), line.mode_list.end(), vj.mode_idx) == line.mode_list.end())
                line.mode_list.push_back(vj.mode_idx);

            navitia::type::Company & company = data.companies.at(vj.company_idx);
            if(std::find(line.company_list.begin(), line.company_list.end(), vj.company_idx) == line.company_list.end())
                line.company_list.push_back(vj.company_idx);

            if(std::find(company.line_list.begin(), company.line_list.end(), line.idx) == company.line_list.end())
                company.line_list.push_back(line.idx);
        }catch(std::out_of_range ex){}

    }

    // ICI VLA remplir les stop_time_list des vj
    BOOST_FOREACH(navitia::type::StopTime & st, data.stop_times){
        try {
            data.vehicle_journeys.at(st.vehicle_journey_idx).stop_time_list.push_back(st.idx);
        }catch(std::out_of_range ex){}
    }



    // BOOST_FOREACH(navitia::type::Company & company, data.companies){}
}
