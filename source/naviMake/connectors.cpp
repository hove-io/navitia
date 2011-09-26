#include "connectors.h"

#include <boost/lexical_cast.hpp>
#include "csv.h"


using namespace navimake::connectors;

CsvFusio::CsvFusio(const std::string& path, const std::string &): path(path){
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
    for(row=reader.next(); row != reader.end(); row = reader.next()){
        if(row.size() < 11){
            throw BadFormated();
        }
        if(counter != 0){
            navimake::types::Network* network = new navimake::types::Network();
            network->id = boost::lexical_cast<int>(row.at(0));
            network->name = row.at(2);
            network->external_code = row.at(3);
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
    for(row=reader.next(); row != reader.end(); row = reader.next()){
        if(row.size() < 3){
            throw BadFormated();
        }
        if(counter != 0){
            navimake::types::ModeType* mode_type = new navimake::types::ModeType();
            mode_type->id = boost::lexical_cast<int>(row.at(0));
            mode_type->name = row.at(1);
            mode_type->external_code = row.at(2);
            data.mode_types.push_back(mode_type);
            mode_type_map[mode_type->id] = mode_type; 
        }
        counter++;       
    }
    reader.close();
}

void CsvFusio::fill_modes(navimake::Data& data){
    
    CsvReader reader(path + "/Mode.csv", ';', "ISO-8859-1");
    std::vector<std::string> row;
    int counter = 0;
    for(row=reader.next(); row != reader.end(); row = reader.next()){
        if(row.size() < 5){
            throw BadFormated();
        }
        if(counter != 0){
            navimake::types::Mode* mode = new navimake::types::Mode();
            mode->id = boost::lexical_cast<int>(row.at(0));
            mode->name = row.at(2);
            mode->external_code = row.at(3);

            int mode_type_id = boost::lexical_cast<int>(row.at(1));
            mode->mode_type = this->find(mode_type_map, mode_type_id);

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
    for(row=reader.next(); row != reader.end(); row = reader.next()){
        if(row.size() < 13){
            throw BadFormated();
        }
        if(counter != 0){
            navimake::types::Line* line = new navimake::types::Line();
            line->id = boost::lexical_cast<int>(row.at(0));
            line->name = row.at(4);
            line->external_code = row.at(3);
            line->code = row.at(2);

            int network_id = boost::lexical_cast<int>(row.at(5));
            line->network = this->find(network_map, network_id);

            int mode_type_id = boost::lexical_cast<int>(row.at(1));
            line->mode_type = this->find(mode_type_map, mode_type_id);

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
    for(row=reader.next(); row != reader.end(); row = reader.next()){
        if(row.size() < 8){
            throw BadFormated();
        }
        if(counter != 0){
            navimake::types::City* city = new navimake::types::City();
            city->id = counter;//boost::lexical_cast<int>(row[0]);
            city->external_code = row.at(1);
            city->name = row.at(2);

            city->coord.x = boost::lexical_cast<double>(row.at(3));
            city->coord.y = boost::lexical_cast<double>(row.at(4));

            city->main_postal_code = row.at(5);

            data.cities.push_back(city);
            city_map[city->external_code] = city;
        }
        counter++;       
    }
    reader.close();
}



void CsvFusio::fill_stop_areas(navimake::Data& data){
    CsvReader reader(path + "/StopArea.csv", ';', "ISO-8859-1");
    std::vector<std::string> row;
    int counter = 0;
    for(row=reader.next(); row != reader.end(); row = reader.next()){
        if(row.size() < 9){
            throw BadFormated();
        }
        if(counter != 0){
            navimake::types::StopArea* stop_area = new navimake::types::StopArea();
            stop_area->id = boost::lexical_cast<int>(row.at(0));
            stop_area->external_code = row.at(2);
            stop_area->name = row.at(1);

            if(row.at(4) == "True") stop_area->main_stop_area = true;
            if(row.at(5) == "True") stop_area->main_connection = true;

            stop_area->coord.x = boost::lexical_cast<double>(row.at(6));
            stop_area->coord.y = boost::lexical_cast<double>(row.at(7));


            std::string city_external_code = row.at(3);
            navimake::types::City* city = this->find(city_map, city_external_code);
            if(city != NULL){
                city->stop_area_list.push_back(stop_area);
            }


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
    for(row=reader.next(); row != reader.end(); row = reader.next()){
        if(row.size() < 15){
            throw BadFormated();
        }
        if(counter != 0){
            navimake::types::StopPoint* stop_point = new navimake::types::StopPoint();
            stop_point->id = boost::lexical_cast<int>(row.at(0));
            stop_point->external_code = row.at(2);
            stop_point->name = row.at(3);

            stop_point->address_name = row.at(6);
            stop_point->address_number = row.at(7);
            stop_point->address_type_name = row.at(8);

            stop_point->coord.x = boost::lexical_cast<double>(row.at(12));
            stop_point->coord.y = boost::lexical_cast<double>(row.at(13));
            stop_point->fare_zone = boost::lexical_cast<int>(row.at(14));

            int stop_area_id = boost::lexical_cast<int>(row.at(4));
            stop_point->stop_area = this->find(stop_area_map, stop_area_id);

            std::string city_external_code = row.at(5);
            stop_point->city = this->find(city_map, city_external_code);

            int mode_id = boost::lexical_cast<int>(row.at(11));
            stop_point->mode = this->find(mode_map, mode_id);

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
    for(row=reader.next(); row != reader.end(); row = reader.next()){
        if(row.size() < 9){
            throw BadFormated();
        }
        if(counter != 0){
            navimake::types::Route* route = new navimake::types::Route();
            route->id = boost::lexical_cast<int>(row.at(0));
            route->external_code = row.at(3);
            route->name = row.at(2);

            if(row.at(1) == "True") route->is_frequence = true;
            if(row.at(5) == "True") route->is_forward = true;

            int line_id = boost::lexical_cast<int>(row.at(4));
            route->line = this->find(line_map, line_id);

            int mode_id = boost::lexical_cast<int>(row.at(8));
            route->mode = this->find(mode_map, mode_id);

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
    for(row=reader.next(); row != reader.end(); row = reader.next()){
        if(row.size() < 14){
            throw BadFormated();
        }
        if(counter != 0){
            navimake::types::VehicleJourney* vehicle_journey = new navimake::types::VehicleJourney();
            vehicle_journey->id = boost::lexical_cast<int>(row.at(0));
            vehicle_journey->external_code = row.at(8);
            vehicle_journey->name = row.at(7);

            if(row.at(12) == "True") vehicle_journey->is_adapted = true;

            int route_id = boost::lexical_cast<int>(row.at(2));
            vehicle_journey->route = this->find(route_map, route_id);

            int mode_id = boost::lexical_cast<int>(row.at(10));
            vehicle_journey->mode = this->find(mode_map, mode_id);

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
    for(row=reader.next(); row != reader.end(); row = reader.next()){
        if(row.size() < 9){
            throw BadFormated();
        }
        if(counter != 0){
            navimake::types::StopTime* stop = new navimake::types::StopTime();
            stop->id = boost::lexical_cast<int>(row.at(0));
            stop->zone = boost::lexical_cast<int>(row.at(1));

            stop->arrival_time = boost::lexical_cast<int>(row.at(2));
            stop->departure_time = boost::lexical_cast<int>(row.at(3));

            stop->order = boost::lexical_cast<int>(row.at(8));


            int vehicle_journey_id = boost::lexical_cast<int>(row.at(4));
            stop->vehicle_journey = this->find(vehicle_journey_map, vehicle_journey_id);

            int stop_point_id = boost::lexical_cast<int>(row.at(7));
            navimake::types::StopPoint* stop_point = this->find(stop_point_map, stop_point_id);
            std::string code = stop_point->external_code + ":" + stop->vehicle_journey->route->external_code;
            
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
    for(row=reader.next(); row != reader.end(); row = reader.next()){
        if(row.size() < 6){
            throw BadFormated();
        }
        if(counter != 0){
            navimake::types::Connection* connection = new navimake::types::Connection();
            connection->id = boost::lexical_cast<int>(row.at(0));

            int departure = boost::lexical_cast<int>(row.at(2));
            connection->departure_stop_point = this->find(stop_point_map, departure);


            int destination = boost::lexical_cast<int>(row.at(3));
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
    for(row=reader.next(); row != reader.end(); row = reader.next()){
        if(row.size() < 6){
            throw BadFormated();
        }
        if(counter != 0){
            navimake::types::RoutePoint* route_point = new navimake::types::RoutePoint();
            route_point->id = boost::lexical_cast<int>(row.at(0));
            route_point->fare_section = boost::lexical_cast<int>(row.at(1));
            route_point->order = boost::lexical_cast<int>(row.at(2));

            if(row.at(4) == "True") route_point->main_stop_point = true;
            
            int route_id = boost::lexical_cast<int>(row.at(3));
            route_point->route = this->find(route_map, route_id);

            int stop_point_id = boost::lexical_cast<int>(row.at(5));
            route_point->stop_point = this->find(stop_point_map, stop_point_id);

            data.route_points.push_back(route_point);
            std::string code = route_point->route->external_code + ":" + route_point->stop_point->external_code;
            route_point_map[code] = route_point;
        }
        counter++;       
    }
    reader.close();
}
