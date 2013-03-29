#pragma once
#include "naviMake/types.h"
#include "naviMake/data.h"
#include "type/message.h"

namespace navimake{

void delete_vj(types::VehicleJourney* vehicle_journey, const nt::Message& message, Data& data);
std::vector<types::StopTime*> get_stop_from_impact(const navitia::type::Message& message, boost::gregorian::date current_date, std::vector<types::StopTime*> stoplist);
void duplicate_vj(types::VehicleJourney* vehicle_journey, const nt::Message& message, Data& data);

struct AtAdaptedLoader{
private:
    std::unordered_map<std::string, types::VehicleJourney*> vj_map;
    std::unordered_map<std::string, std::vector<types::VehicleJourney*>> line_vj_map;
    std::unordered_map<std::string, std::vector<types::VehicleJourney*>> network_vj_map;


    std::unordered_map<types::VehicleJourney*, std::vector<navitia::type::Message>> duplicate_vj_map;
    std::unordered_map<types::VehicleJourney*, std::vector<navitia::type::Message>> update_vj_map;



    void init_map(const Data& data);
    std::vector<types::VehicleJourney*> reconcile_impact_with_vj(const navitia::type::Message& messages, const Data& data);
    void apply_on_vj(types::VehicleJourney* vehicle_journey, const std::vector<navitia::type::Message>& messages, Data& data, const bool& duplicatevj);

    std::vector<types::VehicleJourney*> get_vj_from_stoppoint(std::string stoppoint_uri, const Data& data);
    std::vector<types::VehicleJourney*> get_vj_from_impact(const navitia::type::Message& message, const Data& data);
    void dispatch_message(const std::map<std::string, std::vector<navitia::type::Message>>& messages, const Data& data);

public:
    void apply(const std::map<std::string, std::vector<navitia::type::Message>>& messages, Data& data);
    void apply_b(const std::map<std::string, std::vector<navitia::type::Message>>& messages, Data& data);


};

}//namespace


