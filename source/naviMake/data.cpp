#include "data.h"
#include <iostream>

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
}

void Data::clean(){
}



void Data::transform(navitia::type::Data& data){
    data.stop_areas.resize(this->stop_areas.size());
    std::transform(this->stop_areas.begin(), this->stop_areas.end(), data.stop_areas.begin(), navimake::types::StopArea::Transformer());

    data.modes.resize(this->modes.size());
    std::transform(this->modes.begin(), this->modes.end(), data.modes.begin(), navimake::types::Mode::Transformer());
}
