#pragma once
#include "ed/types.h"
#include "ed/data.h"
#include "type/message.h"

namespace ed{

nt::ValidityPattern* get_validity_pattern(nt::ValidityPattern* validity_pattern,
                          const nt::AtPerturbation& pert,
                          nt::PT_Data& data, uint32_t time);
void update_stop_times(nt::VehicleJourney* vehicle_journey, const nt::AtPerturbation& pert, nt::PT_Data& data);
void delete_vj(nt::VehicleJourney* vehicle_journey, const nt::AtPerturbation& perturbation, nt::PT_Data& data);
std::vector<nt::StopTime*> get_stop_from_impact(const nt::AtPerturbation& perturbation, boost::gregorian::date current_date, std::vector<nt::StopTime*> stoplist);

/// retourne la liste des StopTime à supprimer
std::vector<nt::StopTime*> duplicate_vj(nt::VehicleJourney* vehicle_journey, const nt::AtPerturbation& perturbation, nt::PT_Data& data);

std::string make_adapted_uri(const nt::VehicleJourney* vj, const nt::PT_Data& data);

nt::VehicleJourney* create_adapted_vj(nt::VehicleJourney* current_vj, nt::VehicleJourney* theorical_vj,  nt::PT_Data& data);
std::pair<bool, nt::VehicleJourney*> find_reference_vj(types::VehicleJourney* vehicle_journey, int day_index);

boost::posix_time::time_period build_stop_period(const nt::StopTime& stop, const boost::gregorian::date& date);

struct AtAdaptedLoader{
private:
    std::unordered_map<std::string, nt::VehicleJourney*> vj_map;
    std::unordered_map<std::string, std::vector<nt::VehicleJourney*>> line_vj_map;
    std::unordered_map<std::string, std::vector<nt::VehicleJourney*>> network_vj_map;
    std::unordered_map<std::string, std::vector<nt::VehicleJourney*>> stop_point_vj_map;
    std::unordered_map<std::string, std::vector<nt::StopPoint*>> stop_area_to_stop_point_map;


    std::unordered_map<nt::VehicleJourney*, std::set<nt::AtPerturbation>> duplicate_vj_map;
    std::unordered_map<nt::VehicleJourney*, std::set<nt::AtPerturbation>> update_vj_map;

    std::vector<nt::StopTime*> stop_to_delete;


    void init_map(const nt::PT_Data& data);
    std::vector<nt::VehicleJourney*> reconcile_impact_with_vj(const nt::AtPerturbation& perturbations, const nt::PT_Data& data);
    void apply_deletion_on_vj(nt::VehicleJourney* vehicle_journey, const std::set<nt::AtPerturbation>& perturbations, nt::PT_Data& data);
    void apply_update_on_vj(nt::VehicleJourney* vehicle_journey, const std::set<nt::AtPerturbation>& perturbations, nt::PT_Data& data);

    std::vector<nt::VehicleJourney*> get_vj_from_stop_area(const std::string& stop_area_uri);
    std::vector<nt::VehicleJourney*> get_vj_from_stoppoint(const std::string& stoppoint_uri);
    std::vector<nt::VehicleJourney*> get_vj_from_impact(const nt::AtPerturbation& perturbation);
    void dispatch_perturbations(const std::vector<nt::AtPerturbation>& perturbations, const nt::PT_Data& data);

    void clean(nt::PT_Data& data);
    void clean_stop_time(nt::PT_Data& data);
    void clean_journey_pattern_point(nt::PT_Data& data);

public:
    void apply(const std::vector<nt::AtPerturbation>& perturbations, nt::PT_Data& data);


};

}//namespace


