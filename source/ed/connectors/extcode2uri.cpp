#include "extcode2uri.h"


namespace ed{ namespace connectors{

ExtCode2Uri::ExtCode2Uri(const std::string& redis_connection){
    std::vector<std::string> params = split_string(redis_connection, " ");
    std::map<std::string, std::string> key_value;

    for(std::string param : params){
        auto split = split_string(param, "=");
        if (split.size() > 0){
            key_value[split[0]] = split[1];
        }
    }

    if (key_value.find("host") != key_value.end()){
        this->host = key_value.at("host");
    }

    if (key_value.find("password") != key_value.end()){
        this->password = key_value.at("password");
    }

    if (key_value.find("port") != key_value.end()){
        this->port = str_to_int(key_value.at("port"));
    }

    if (key_value.find("db") != key_value.end()){
        this->db = str_to_int(key_value.at("db"));
    }

    this->separator = ":";
    init_logger();
    logger = log4cplus::Logger::getInstance("log");
}

bool ExtCode2Uri::connected(){
    try{       
        connect = redisConnect(this->host.c_str(),this->port);
        if(connect != NULL && connect->err){
            LOG4CPLUS_FATAL(logger, "Connecting redis failed, error : "+std::string(connect->errstr));
            return false;
        }
    }catch(...){
        LOG4CPLUS_FATAL(logger, "Connecting redis failed");
        return false;
    }
    return true;
}

ExtCode2Uri::~ExtCode2Uri(){
    try{
        redisFree(this->connect);
    }catch(...){
        LOG4CPLUS_FATAL(logger, "Disconnecting redis failed");
    }
}

bool ExtCode2Uri::set(const std::string& key, const std::string& value){

    std::string command_set = "SET " + this->prefix + this->separator + key + " " + value;
     bool to_return;
    try{
        to_return = redisCommand(this->connect, command_set.c_str());
    }catch(...){
         to_return = false;
    }
    return to_return;
}

bool ExtCode2Uri::set_lists(const ed::Data & data){
    if (this->connected()){
        // StopArea
        this->prefix = navitia::type::static_data::get()->captionByType(navitia::type::Type_e::StopArea);
        for(ed::types::StopArea* stop_area: data.stop_areas){
            if (! this->set(stop_area->external_code, stop_area->uri)){
                LOG4CPLUS_WARN(logger, "ExtCode2Uri : impossible d'ajouter la correspondance <"+stop_area->external_code+","+ stop_area->uri+"> de l'objet "+this->prefix);
            }
        }

        // StopPoint
        this->prefix = navitia::type::static_data::get()->captionByType(navitia::type::Type_e::StopPoint);
        for(ed::types::StopPoint* stop_point: data.stop_points){
            if (! this->set(stop_point->external_code, stop_point->uri)){
                LOG4CPLUS_WARN(logger, "ExtCode2Uri : impossible d'ajouter la correspondance <"+stop_point->external_code+","+ stop_point->uri+"> de l'objet "+this->prefix);
            }
        }

        // Line
        this->prefix = navitia::type::static_data::get()->captionByType(navitia::type::Type_e::Line);
        for(ed::types::Line* line: data.lines){
            if (! this->set(line->external_code, line->uri)){
                LOG4CPLUS_WARN(logger, "ExtCode2Uri : impossible d'ajouter la correspondance <"+line->external_code+","+ line->uri+"> de l'objet "+this->prefix);
            }
        }

        // VehicleJourney
        this->prefix = navitia::type::static_data::get()->captionByType(navitia::type::Type_e::VehicleJourney);
        for(ed::types::VehicleJourney* vehicle_journey: data.vehicle_journeys){
            if (! this->set(vehicle_journey->external_code, vehicle_journey->uri)){
                LOG4CPLUS_WARN(logger, "ExtCode2Uri : impossible d'ajouter la correspondance <"+vehicle_journey->external_code+","+ vehicle_journey->uri+"> de l'objet "+this->prefix);
            }
        }

        // Network
        this->prefix = navitia::type::static_data::get()->captionByType(navitia::type::Type_e::Network);
        for(ed::types::Network* network: data.networks){
            if (! this->set(network->external_code, network->uri)){
                LOG4CPLUS_WARN(logger, "ExtCode2Uri : impossible d'ajouter la correspondance <"+network->external_code+","+ network->uri+"> de l'objet "+this->prefix);
            }
        }

        return true;
    }else{
        return false;
    }
}

}}
