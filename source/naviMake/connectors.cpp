#include "connectors.h"

#include <boost/lexical_cast.hpp>
#include "utils/csv.h"


using namespace navimake::connectors;

CsvFusio::CsvFusio(const std::string& path): path(path){
}

CsvFusio::~CsvFusio(){}

void CsvFusio::fill(navimake::Data& data){
    fill_networks(data);
    fill_modes_type(data);
    fill_modes(data);
    fill_lines(data);
    fill_cities(data);
    fill_stop_areas(data);
    fill_stop_points(data);
    fill_routes(data);
    fill_vehicle_journeys(data);
    fill_route_points(data);
    fill_stops(data);
    fill_connections(data);
}

void CsvFusio::fill_networks(navimake::Data& data){
    
    CsvReader reader(path + "Network.csv", ';', "ISO-8859-1");
    std::vector<std::string> row;
    int counter = 0;
    for(row=reader.next(); !reader.eof(); row = reader.next()){
        if(row.size() < 11){
            throw BadFormated();
        }
        if(counter != 0){
            navimake::types::Network* network = new navimake::types::Network();
            network->id = row.at(0);
            network->name = row.at(2);
            network->uri = row.at(3);
            network->address_name = row.at(5);
            network->address_number = row.at(6);
            network->address_type_name = row.at(7);
            network->phone_number = row.at(7);
            network->mail = row.at(8);
            network->website = row.at(9);
            network->fax = row.at(1);
            data.networks.push_back(network);
            network_map[network->id] = network; 
        }
        counter++;
    }
    reader.close();
}


void CsvFusio::fill_modes_type(navimake::Data& data){
    
    CsvReader reader(path + "/ModeType.csv", ';', "ISO-8859-1");
    std::vector<std::string> row;
    int counter = 0;
    for(row=reader.next(); !reader.eof(); row = reader.next()){
        if(row.size() < 3){
            throw BadFormated();
        }
        if(counter != 0){
            navimake::types::CommercialMode* commercial_mode = new navimake::types::CommercialMode();
            commercial_mode->id = row.at(0);
            commercial_mode->name = row.at(1);
            commercial_mode->uri = row.at(2);
            data.commercial_modes.push_back(commercial_mode);
            commercial_mode_map[commercial_mode->id] = commercial_mode; 
        }
        counter++;       
    }
    reader.close();
}

void CsvFusio::fill_modes(navimake::Data& data){
    
    CsvReader reader(path + "/Mode.csv", ';', "ISO-8859-1");
    std::vector<std::string> row;
    int counter = 0;
    for(row=reader.next();!reader.eof(); row = reader.next()){
        if(row.size() < 5){
            throw BadFormated();
        }
        if(counter != 0){
            navimake::types::PhysicalMode* mode = new navimake::types::PhysicalMode();
            mode->id = row.at(0);
            mode->name = row.at(2);
            mode->uri = row.at(3);

            std::string commercial_mode_id = row.at(1);
            mode->commercial_mode = this->find(commercial_mode_map, commercial_mode_id);

            data.modes.push_back(mode);
            mode_map[mode->id] = mode; 
        }
        counter++;       
    }
    reader.close();
}


void CsvFusio::fill_lines(navimake::Data& data){
    
    CsvReader reader(path + "/Line.csv", ';', "ISO-8859-1");
    std::vector<std::string> row;
    int counter = 0;
    for(row=reader.next(); !reader.eof(); row = reader.next()){
        if(row.size() < 13){
            throw BadFormated();
        }
        if(counter != 0){
            navimake::types::Line* line = new navimake::types::Line();
            line->id = row.at(0);
            line->name = row.at(4);
            line->uri = row.at(3);
            line->code = row.at(2);

            std::string network_id = row.at(5);
            line->network = this->find(network_map, network_id);

            std::string commercial_mode_id = row.at(1);
            line->commercial_mode = this->find(commercial_mode_map, commercial_mode_id);

            data.lines.push_back(line);
            line_map[line->id] = line;
        }
        counter++;       
    }
    std::cout << counter -1 << " must be equals to " << data.lines.size() << std::endl;
    reader.close();
}

void CsvFusio::fill_cities(navimake::Data& data){
    CsvReader reader(path + "/city.csv", ';', "ISO-8859-1");
    std::vector<std::string> row;
    int counter = 0;
    for(row=reader.next(); !reader.eof(); row = reader.next()){
        if(row.size() < 8){
            throw BadFormated();
        }
        if(counter != 0){
            navimake::types::City* city = new navimake::types::City();
            city->id = counter;//boost::lexical_cast<int>(row[0]);
            city->uri = row.at(1);
            city->name = row.at(2);

            city->coord.set_lon(boost::lexical_cast<double>(row.at(3)));
            city->coord.set_lat(boost::lexical_cast<double>(row.at(4)));

            city->main_postal_code = row.at(5);

            data.cities.push_back(city);
            city_map[city->uri] = city;
        }
        counter++;       
    }
    reader.close();
}



void CsvFusio::fill_stop_areas(navimake::Data& data){
    CsvReader reader(path + "/StopArea.csv", ';', "ISO-8859-1");
    std::vector<std::string> row;
    int counter = 0;
    for(row=reader.next(); !reader.eof(); row = reader.next()){
        if(row.size() < 9){
            throw BadFormated();
        }
        if(counter != 0){
            navimake::types::StopArea* stop_area = new navimake::types::StopArea();
            stop_area->id = boost::lexical_cast<int>(row.at(0));
            stop_area->uri = row.at(2);
            stop_area->name = row.at(1);

            if(row.at(4) == "True") stop_area->main_stop_area = true;
            if(row.at(5) == "True") stop_area->main_connection = true;

            stop_area->coord.set_lon(boost::lexical_cast<double>(row.at(6)));
            stop_area->coord.set_lat(boost::lexical_cast<double>(row.at(7)));


            data.stop_areas.push_back(stop_area);
            stop_area_map[stop_area->id] = stop_area;
        }
        counter++;       
    }
    reader.close();
}


void CsvFusio::fill_stop_points(navimake::Data& data){
    CsvReader reader(path + "/StopPoint.csv", ';', "ISO-8859-1");
    std::vector<std::string> row;
    int counter = 0;
    for(row=reader.next(); !reader.eof(); row = reader.next()){
        if(row.size() < 15){
            throw BadFormated();
        }
        if(counter != 0){
            navimake::types::StopPoint* stop_point = new navimake::types::StopPoint();
            stop_point->id = boost::lexical_cast<int>(row.at(0));
            stop_point->uri = row.at(2);
            stop_point->name = row.at(3);

            stop_point->address_name = row.at(6);
            stop_point->address_number = row.at(7);
            stop_point->address_type_name = row.at(8);

            stop_point->coord.set_lon(boost::lexical_cast<double>(row.at(12)));
            stop_point->coord.set_lat(boost::lexical_cast<double>(row.at(13)));
            stop_point->fare_zone = boost::lexical_cast<int>(row.at(14));

            std::string stop_area_id = row.at(4);
            stop_point->stop_area = this->find(stop_area_map, stop_area_id);

            std::string city_uri = row.at(5);
            stop_point->city = this->find(city_map, city_uri);

            std::string mode_id = row.at(11);
            stop_point->physical_mode = this->find(mode_map, mode_id);

            data.stop_points.push_back(stop_point);
            stop_point_map[stop_point->id] = stop_point;
        }
        counter++;       
    }
    reader.close();
}



void CsvFusio::fill_routes(navimake::Data& data){
    CsvReader reader(path + "/Route.csv", ';', "ISO-8859-1");
    std::vector<std::string> row;
    int counter = 0;
    for(row=reader.next(); !reader.eof(); row = reader.next()){
        if(row.size() < 9){
            throw BadFormated();
        }
        if(counter != 0){
            navimake::types::Route* route = new navimake::types::Route();
            route->id = boost::lexical_cast<int>(row.at(0));
            route->uri = row.at(3);
            route->name = row.at(2);

            if(row.at(1) == "True") route->is_frequence = true;
            if(row.at(5) == "True") route->is_forward = true;

            std::string line_id = row.at(4);
            route->line = this->find(line_map, line_id);

            std::string mode_id = row.at(8);
            route->physical_mode = this->find(mode_map, mode_id);

            data.routes.push_back(route);
            route_map[route->id] = route;
        }
        counter++;       
    }
    reader.close();
}


void CsvFusio::fill_vehicle_journeys(navimake::Data& data){
    CsvReader reader(path + "/VehicleJourney.csv", ';', "ISO-8859-1");
    std::vector<std::string> row;
    int counter = 0;
    for(row=reader.next(); !reader.eof(); row = reader.next()){
        if(row.size() < 14){
            throw BadFormated();
        }
        if(counter != 0){
            navimake::types::VehicleJourney* vehicle_journey = new navimake::types::VehicleJourney();
            vehicle_journey->id = row.at(0);
            vehicle_journey->uri = row.at(8);
            vehicle_journey->name = row.at(7);

            if(row.at(12) == "True") vehicle_journey->is_adapted = true;

            std::string route_id = row.at(2);
            vehicle_journey->route = this->find(route_map, route_id);

            std::string mode_id = row.at(10);
            vehicle_journey->physical_mode = this->find(mode_map, mode_id);

            data.vehicle_journeys.push_back(vehicle_journey);
            vehicle_journey_map[vehicle_journey->id] = vehicle_journey;
        }
        counter++;       
    }
    reader.close();
}


void CsvFusio::fill_stops(navimake::Data& data){
    CsvReader reader(path + "/Stop.csv", ';', "ISO-8859-1");
    std::vector<std::string> row;
    int counter = 0;
    for(row=reader.next(); !reader.eof(); row = reader.next()){
        if(row.size() < 9){
            throw BadFormated();
        }
        if(counter != 0){
            navimake::types::StopTime* stop = new navimake::types::StopTime();
            //stop->zone = boost::lexical_cast<int>(row.at(1));

            stop->arrival_time = boost::lexical_cast<int>(row.at(2));
            stop->departure_time = boost::lexical_cast<int>(row.at(3));

            stop->order = boost::lexical_cast<int>(row.at(8));

            std::string vehicle_journey_id = row.at(4);
            stop->vehicle_journey = this->find(vehicle_journey_map, vehicle_journey_id);

            std::string stop_point_id = row.at(7);
            navimake::types::StopPoint* stop_point = this->find(stop_point_map, stop_point_id);
            std::string code = stop_point->uri + ":" + stop->vehicle_journey->route->uri;
            
            stop->route_point = this->find(route_point_map, code);
            

            data.stops.push_back(stop);
        }
        counter++;       
    }
    reader.close();
}

void CsvFusio::fill_connections(navimake::Data& data){
    CsvReader reader(path + "/Connection.csv", ';', "ISO-8859-1");
    std::vector<std::string> row;
    int counter = 0;
    for(row=reader.next(); !reader.eof(); row = reader.next()){
        if(row.size() < 6){
            throw BadFormated();
        }
        if(counter != 0){
            navimake::types::Connection* connection = new navimake::types::Connection();
            connection->id = boost::lexical_cast<int>(row.at(0));

            std::string departure = row.at(2);
            connection->departure_stop_point = this->find(stop_point_map, departure);


            std::string destination = row.at(3);
            connection->destination_stop_point = this->find(stop_point_map, destination);

            data.connections.push_back(connection);
        }
        counter++;       
    }
    reader.close();
}

void CsvFusio::fill_route_points(navimake::Data& data){
    CsvReader reader(path + "/RoutePoint.csv", ';', "ISO-8859-1");
    std::vector<std::string> row;
    int counter = 0;
    for(row=reader.next(); !reader.eof(); row = reader.next()){
        if(row.size() < 6){
            throw BadFormated();
        }
        if(counter != 0){
            navimake::types::RoutePoint* route_point = new navimake::types::RoutePoint();
            route_point->id = boost::lexical_cast<int>(row.at(0));
            route_point->fare_section = boost::lexical_cast<int>(row.at(1));
            route_point->order = boost::lexical_cast<int>(row.at(2));

            if(row.at(4) == "True") route_point->main_stop_point = true;
            
            std::string route_id = row.at(3);
            route_point->route = this->find(route_map, route_id);

            std::string stop_point_id = row.at(5);
            route_point->stop_point = this->find(stop_point_map, stop_point_id);

            data.route_points.push_back(route_point);
            std::string code = route_point->route->uri + ":" + route_point->stop_point->uri;
            route_point_map[code] = route_point;
        }
        counter++;       
    }
    reader.close();
}
