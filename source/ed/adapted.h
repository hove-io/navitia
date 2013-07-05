#pragma once
#include "ed/types.h"
#include "ed/data.h"
#include "type/message.h"

namespace ed{

void delete_vj(types::VehicleJourney* vehicle_journey, const nt::Message& message, Data& data);
std::vector<types::StopTime*> get_stop_from_impact(const navitia::type::Message& message, boost::gregorian::date current_date, std::vector<types::StopTime*> stoplist);

/// retourne la liste des StopTime à supprimer
std::vector<types::StopTime*> duplicate_vj(types::VehicleJourney* vehicle_journey, const nt::Message& message, Data& data);

std::string make_adapted_uri(const types::VehicleJourney* vj, const Data& data);

types::VehicleJourney* create_adapted_vj(types::VehicleJourney* current_vj, types::VehicleJourney* theorical_vj,  Data& data);
std::pair<bool, types::VehicleJourney*> find_reference_vj(types::VehicleJourney* vehicle_journey, int day_index);

boost::posix_time::time_period build_stop_period(const types::StopTime& stop, const boost::gregorian::date& date);

struct AtAdaptedLoader{
private:
    std::unordered_map<std::string, types::VehicleJourney*> vj_map;
    std::unordered_map<std::string, std::vector<types::VehicleJourney*>> line_vj_map;
    std::unordered_map<std::string, std::vector<types::VehicleJourney*>> network_vj_map;
    std::unordered_map<std::string, std::vector<types::VehicleJourney*>> stop_point_vj_map;
    std::unordered_map<std::string, std::vector<types::StopPoint*>> stop_area_to_stop_point_map;


    std::unordered_map<types::VehicleJourney*, std::set<navitia::type::Message>> duplicate_vj_map;
    std::unordered_map<types::VehicleJourney*, std::set<navitia::type::Message>> update_vj_map;

    std::vector<types::StopTime*> stop_to_delete;


    void init_map(const Data& data);
    std::vector<types::VehicleJourney*> reconcile_impact_with_vj(const navitia::type::Message& messages, const Data& data);
    void apply_deletion_on_vj(types::VehicleJourney* vehicle_journey, const std::set<navitia::type::Message>& messages, Data& data);
    void apply_update_on_vj(types::VehicleJourney* vehicle_journey, const std::set<navitia::type::Message>& messages, Data& data);

    std::vector<types::VehicleJourney*> get_vj_from_stop_area(std::string stop_area_uri);
    std::vector<types::VehicleJourney*> get_vj_from_stoppoint(std::string stoppoint_uri);
    std::vector<types::VehicleJourney*> get_vj_from_impact(const navitia::type::Message& message);
    void dispatch_message(const std::map<std::string, std::vector<navitia::type::Message>>& messages, const Data& data);

    void clean(Data& data);

public:
    void apply(const std::map<std::string, std::vector<navitia::type::Message>>& messages, Data& data);


};

}//namespace


