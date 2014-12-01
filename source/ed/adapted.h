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

#pragma once
#include "ed/types.h"
#include "ed/data.h"
#include "ed/at_perturbation.h"

namespace ed{

nt::ValidityPattern* get_validity_pattern(nt::ValidityPattern* validity_pattern,
                          const AtPerturbation& pert,
                          nt::PT_Data& data, uint32_t time);
nt::ValidityPattern* get_validity_pattern(nt::ValidityPattern* validity_pattern,
                          nt::PT_Data& data, boost::gregorian::date day, bool add);
void update_stop_times(nt::VehicleJourney* vehicle_journey, const AtPerturbation& pert, nt::PT_Data& data);
void delete_vj(nt::VehicleJourney* vehicle_journey, const AtPerturbation& perturbation, nt::PT_Data& data);
std::vector<nt::StopTime*> get_stop_from_impact(const AtPerturbation& perturbation, boost::gregorian::date current_date, std::vector<nt::StopTime*> stoplist);

void duplicate_vj(nt::VehicleJourney* vehicle_journey, const AtPerturbation& perturbation, nt::PT_Data& data);
std::string make_adapted_uri(const nt::VehicleJourney* vj, const nt::PT_Data& data);

nt::VehicleJourney* create_adapted_vj(nt::VehicleJourney* current_vj, nt::VehicleJourney* theorical_vj, std::vector<navitia::type::StopTime *> impacted_st,  nt::PT_Data& data);
std::pair<bool, nt::VehicleJourney*> find_reference_vj(types::VehicleJourney* vehicle_journey, int day_index);

boost::posix_time::time_period build_stop_period(const nt::StopTime& stop, const boost::gregorian::date& date);

struct AtAdaptedLoader{
private:
    std::unordered_map<std::string, nt::VehicleJourney*> vj_map;
    std::unordered_map<std::string, std::vector<nt::VehicleJourney*>> line_vj_map;
    std::unordered_map<std::string, std::vector<nt::VehicleJourney*>> network_vj_map;
    std::unordered_map<std::string, std::vector<nt::VehicleJourney*>> stop_point_vj_map;
    std::unordered_map<std::string, std::vector<nt::StopPoint*>> stop_area_to_stop_point_map;

    std::unordered_map<nt::VehicleJourney*, std::set<AtPerturbation>> duplicate_vj_map;
    std::unordered_map<nt::VehicleJourney*, std::set<AtPerturbation>> update_vj_map;

    void init_map(const nt::PT_Data& data);
    std::vector<nt::VehicleJourney*> reconcile_impact_with_vj(const AtPerturbation& perturbations, const nt::PT_Data& data);
    void apply_deletion_on_vj(nt::VehicleJourney* vehicle_journey, const std::set<AtPerturbation>& perturbations, nt::PT_Data& data);
    void apply_update_on_vj(nt::VehicleJourney* vehicle_journey, const std::set<AtPerturbation>& perturbations, nt::PT_Data& data);

    std::vector<nt::VehicleJourney*> get_vj_from_stop_area(const std::string& stop_area_uri);
    std::vector<nt::VehicleJourney*> get_vj_from_stoppoint(const std::string& stoppoint_uri);
    std::vector<nt::VehicleJourney*> get_vj_from_impact(const AtPerturbation& perturbation);
    void dispatch_perturbations(const std::vector<AtPerturbation>& perturbations, const nt::PT_Data& data);

public:
    void apply(const std::vector<AtPerturbation>& perturbations, nt::PT_Data& data);


};

}//namespace


