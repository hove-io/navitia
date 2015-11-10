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

#include "type.h"
#include "pt_data.h"
#include "data.h"
#include "meta_data.h"
#include <iostream>
#include <boost/assign.hpp>
#include "utils/functions.h"
#include "utils/logger.h"

//they need to be included for the BOOST_CLASS_EXPORT_GUID macro
#include "third_party/eos_portable_archive/portable_iarchive.hpp"
#include "third_party/eos_portable_archive/portable_oarchive.hpp"
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

namespace bt = boost::posix_time;

namespace navitia { namespace type {

std::string VehicleJourney::get_direction() const {
    if (! this->stop_time_list.empty()) {
        const auto& st = this->stop_time_list.back();
        if (st.stop_point != nullptr) {
            for (const auto* admin: st.stop_point->admin_list) {
                if (admin->level == 8) {
                    return st.stop_point->name + " (" + admin->name + ")";
                }
            }
            return st.stop_point->name;
        }
    }
    return "";
}

std::vector<boost::shared_ptr<disruption::Impact>> HasMessages::get_applicable_messages(
        const boost::posix_time::ptime& current_time,
        const boost::posix_time::time_period& action_period) const {
    std::vector<boost::shared_ptr<disruption::Impact>> result;

    //we cleanup the released pointer (not in the loop for code clarity)
    clean_up_weak_ptr(impacts);

    for (auto impact : this->impacts) {
        auto impact_acquired = impact.lock();
        if (! impact_acquired) {
            continue; //pointer might still have become invalid
        }
        if (impact_acquired->is_valid(current_time, action_period)) {
            result.push_back(impact_acquired);
        }
    }

    return result;

}

std::vector<boost::shared_ptr<disruption::Impact>> HasMessages::get_publishable_messages(
            const boost::posix_time::ptime& current_time) const{
    std::vector<boost::shared_ptr<disruption::Impact>> result;

    //we cleanup the released pointer (not in the loop for code clarity)
    clean_up_weak_ptr(impacts);

    for (auto impact : this->impacts) {
        auto impact_acquired = impact.lock();
        if (! impact_acquired) {
            continue; //pointer might still have become invalid
        }
        if (impact_acquired->disruption->is_publishable(current_time)) {
            result.push_back(impact_acquired);
        }
    }
    return result;
}

bool HasMessages::has_applicable_message(
        const boost::posix_time::ptime& current_time,
        const boost::posix_time::time_period& action_period) const {
    //we cleanup the released pointer (not in the loop for code clarity)
    clean_up_weak_ptr(impacts);

    for (auto impact : this->impacts) {
        auto impact_acquired = impact.lock();
        if (! impact_acquired) {
            continue; //pointer might still have become invalid
        }
        if (impact_acquired->is_valid(current_time, action_period)) {
            return true;
        }
    }
    return false;
}

bool HasMessages::has_publishable_message(const boost::posix_time::ptime& current_time) const{
    //we cleanup the released pointer (not in the loop for code clarity)
    clean_up_weak_ptr(impacts);

    for (auto impact : this->impacts) {
        auto impact_acquired = impact.lock();
        if (! impact_acquired) {
            continue; //pointer might still have become invalid
        }
        if (impact_acquired->disruption->is_publishable(current_time)) {
            return true;
        }
    }
    return false;
}

bool StopTime::is_valid_day(u_int32_t day, const bool is_arrival, const RTLevel rt_level) const {
    if((is_arrival && arrival_time >= DateTimeUtils::SECONDS_PER_DAY)
       || (!is_arrival && departure_time >= DateTimeUtils::SECONDS_PER_DAY)) {
        if(day == 0)
            return false;
        --day;
    }
    return vehicle_journey->validity_patterns[rt_level]->check(day);
}

uint32_t StopTime::f_arrival_time(const u_int32_t hour, bool clockwise) const {
    // get the arrival time of a frequency stop time from the arrival time on the previous stop in the vj
    assert (is_frequency());
    if(clockwise) {
        if (this == &this->vehicle_journey->stop_time_list.front())
            return hour;
        const auto& prec_st = this->vehicle_journey->stop_time_list[order() - 1];
        return DateTimeUtils::hour_in_day(hour + this->arrival_time - prec_st.arrival_time);
    } else {
        if (this == &this->vehicle_journey->stop_time_list.back())
            return hour;
        const auto& next_st = this->vehicle_journey->stop_time_list[order() + 1];
        return DateTimeUtils::hour_in_day(hour - (next_st.arrival_time - this->arrival_time));
    }
}

uint32_t StopTime::f_departure_time(const u_int32_t hour, bool clockwise) const {
    // get the departure time of a frequency stop time from the departure time on the previous stop in the vj
    assert (is_frequency());
    if(clockwise) {
        if (this == &this->vehicle_journey->stop_time_list.front())
            return hour;
        const auto& prec_st = this->vehicle_journey->stop_time_list[order() - 1];
        return DateTimeUtils::hour_in_day(hour + this->departure_time - prec_st.departure_time);
    } else {
        if (this == &this->vehicle_journey->stop_time_list.back())
            return hour;
        const auto& next_st = this->vehicle_journey->stop_time_list[order() + 1];
        return DateTimeUtils::hour_in_day(hour - (next_st.departure_time - this->departure_time));
    }
}

bool FrequencyVehicleJourney::is_valid(int day, const RTLevel rt_level) const {
    if (day < 0)
        return false;
    return validity_patterns[rt_level]->check(day);
}

boost::posix_time::time_period VehicleJourney::execution_period(const boost::gregorian::date& date) const {
    uint32_t first_departure = std::numeric_limits<uint32_t>::max();
    uint32_t last_arrival = 0;
    for (const auto& st: stop_time_list) {
        if (st.pick_up_allowed() && first_departure > st.departure_time) {
            first_departure = st.departure_time;
        }
        if (st.drop_off_allowed() && last_arrival < st.arrival_time) {
            last_arrival = st.arrival_time;
        }
    }
    return bt::time_period(bt::ptime(date, bt::seconds(first_departure)),
            bt::ptime(date, bt::seconds(last_arrival)));
}

bool VehicleJourney::has_datetime_estimated() const {
    for (const StopTime& st : this->stop_time_list) {
        if (st.date_time_estimated()) { return true; }
    }
    return false;
}

bool VehicleJourney::has_zonal_stop_point() const {
    for (const StopTime& st : this->stop_time_list) {
        if (st.stop_point->is_zonal) { return true; }
    }
    return false;
}

bool VehicleJourney::has_odt() const {
    for (const StopTime& st : this->stop_time_list) {
        if (st.odt()) { return true; }
    }
    return false;
}

bool VehicleJourney::has_boarding() const{
    std::string physical_mode;
    if (this->physical_mode != nullptr)
        physical_mode = this->physical_mode->name;
    if (! physical_mode.empty()){
        boost::to_lower(physical_mode);
        return (physical_mode == "boarding");
    }
    return false;

}

bool VehicleJourney::has_landing() const{
    std::string physical_mode;
    if (this->physical_mode != nullptr)
        physical_mode = this->physical_mode->name;
    if (! physical_mode.empty()){
        boost::to_lower(physical_mode);
        return (physical_mode == "landing");
    }
    return false;

}

bool ValidityPattern::is_valid(int day) const {
    if(day < 0) {
        LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("log"), "Validity pattern not valid, the day "
                       << day << " is too early");
        return false;
    }
    if(size_t(day) >= days.size()) {
        LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("log"), "Validity pattern not valid, the day "
                       << day << " is late");
        return false;
    }
    return true;
}

int ValidityPattern::slide(boost::gregorian::date day) const {
    return (day - beginning_date).days();
}

void ValidityPattern::add(boost::gregorian::date day){
    long duration = slide(day);
    add(duration);
}

void ValidityPattern::add(int duration){
    if(is_valid(duration))
        days[duration] = true;
}

void ValidityPattern::add(boost::gregorian::date start, boost::gregorian::date end, std::bitset<7> active_days){
    for (auto current_date = start; current_date < end; current_date = current_date + boost::gregorian::days(1)) {
        navitia::weekdays week_day = navitia::get_weekday(current_date);
        if (active_days[week_day]) {
            add(current_date);
        } else {
            remove(current_date);
        }
    };
}

void ValidityPattern::remove(boost::gregorian::date date){
    long duration = slide(date);
    remove(duration);
}

void ValidityPattern::remove(int day){
    if(is_valid(day))
        days[day] = false;
}

std::string ValidityPattern::str() const {
    return days.to_string();
}

bool ValidityPattern::check(boost::gregorian::date day) const {
    long duration = slide(day);
    return ValidityPattern::check(duration);
}

bool ValidityPattern::check(unsigned int day) const {
//    BOOST_ASSERT(is_valid(day));
    return days[day];
}

bool ValidityPattern::check2(unsigned int day) const {
//    BOOST_ASSERT(is_valid(day));
    if(day == 0)
        return days[day] || days[day+1];
    else
        return days[day-1] || days[day] || days[day+1];
}

bool ValidityPattern::uncheck2(unsigned int day) const {
//    BOOST_ASSERT(is_valid(day));
    if(day == 0)
        return !days[day] && !days[day+1];
    else
        return !days[day-1] && !days[day] && !days[day+1];
}

template <typename F>
static bool intersect(const VehicleJourney& vj, const std::vector<boost::posix_time::time_period>& periods,
                      RTLevel lvl, const nt::MetaData& meta, const F& fun) {
    bool intersect = false;
    for (const auto& period: periods) {
        //we can impact a vj with a departure the day before who past midnight
        namespace bg = boost::gregorian;
        bg::day_iterator titr(period.begin().date() - bg::days(1));
        for (; titr <= period.end().date(); ++titr) {
            if (! meta.production_date.contains(*titr)) { continue; }

            auto day = (*titr - meta.production_date.begin()).days();
            if (! vj.get_validity_pattern_at(lvl)->check(day)) { continue; }

            if (period.intersects(vj.execution_period(*titr))) {
                intersect = true;
                if (! fun(day)) {
                    return intersect;
                }
            }
        }
    }
    return intersect;
}

namespace {
template<typename VJ> std::vector<VJ*>& get_vjs(Route* r);
template<> std::vector<DiscreteVehicleJourney*>& get_vjs(Route* r) {
    return r->discrete_vehicle_journey_list;
}
template<> std::vector<FrequencyVehicleJourney*>& get_vjs(Route* r) {
    return r->frequency_vehicle_journey_list;
}
}// anonymous namespace

template<typename VJ>
VJ* MetaVehicleJourney::impl_create_vj(const std::string& uri,
                                       const RTLevel level,
                                       const ValidityPattern& vp,
                                       Route* route,
                                       std::vector<StopTime> sts,
                                       nt::PT_Data& pt_data) {
    // creating the vj
    auto vj_ptr = std::make_unique<VJ>();
    VJ* ret = vj_ptr.get();
    vj_ptr->meta_vj = this;
    vj_ptr->uri = uri;
    vj_ptr->idx = pt_data.vehicle_journeys.size();
    vj_ptr->realtime_level = level;
    auto* new_vp = pt_data.get_or_create_validity_pattern(vp);
    for (const auto l: enum_range<RTLevel>()) {
        if (l < level) {
            auto* empty_vp = pt_data.get_or_create_validity_pattern(ValidityPattern(vp.beginning_date));
            vj_ptr->validity_patterns[l] = empty_vp;
        } else {
            vj_ptr->validity_patterns[l] = new_vp;
        }
    }
    vj_ptr->route = route;
    for (auto& st: sts) {
        st.vehicle_journey = ret;
        st.set_is_frequency(std::is_same<VJ, FrequencyVehicleJourney>::value);
    }
    vj_ptr->stop_time_list = std::move(sts);

    // Desactivating the other vjs. The last creation has priority on
    // all the already existing vjs.
    const auto mask = ~vp.days;
    for_all_vjs([&] (VehicleJourney& vj) {
            for (const auto l: enum_range_from(level)) {
                auto new_vp = *vj.validity_patterns[l];
                new_vp.days &= mask;
                vj.validity_patterns[l] = pt_data.get_or_create_validity_pattern(new_vp);
             }
        });

    // inserting the vj in the model
    pt_data.vehicle_journeys.push_back(ret);
    pt_data.vehicle_journeys_map[ret->uri] = ret;
    if (route) {
        get_vjs<VJ>(route).push_back(ret);
    }
    rtlevel_to_vjs_map[level].emplace_back(std::move(vj_ptr));
    return ret;
}

FrequencyVehicleJourney*
MetaVehicleJourney::create_frequency_vj(const std::string& uri,
                                        const RTLevel level,
                                        const ValidityPattern& vp,
                                        Route* route,
                                        std::vector<StopTime> sts,
                                        nt::PT_Data& pt_data) {
    return impl_create_vj<FrequencyVehicleJourney>(uri, level, vp, route, std::move(sts), pt_data);
}

DiscreteVehicleJourney*
MetaVehicleJourney::create_discrete_vj(const std::string& uri,
                                       const RTLevel level,
                                       const ValidityPattern& vp,
                                       Route* route,
                                       std::vector<StopTime> sts,
                                       nt::PT_Data& pt_data) {
    return impl_create_vj<DiscreteVehicleJourney>(uri, level, vp, route, std::move(sts), pt_data);
}


void MetaVehicleJourney::cancel_vj(RTLevel level,
        const std::vector<boost::posix_time::time_period>& periods,
        nt::PT_Data& pt_data, const nt::MetaData& meta, const Route* filtering_route) {
    for (auto l: reverse_enum_range_from<RTLevel>(level)) {
        for (auto& vj: rtlevel_to_vjs_map[l]) {
            // note: we might want to cancel only the vj of certain routes
            if (filtering_route && vj->route != filtering_route) { continue; }
            nt::ValidityPattern tmp_vp(*vj->get_validity_pattern_at(l));
            auto vp_modifier = [&tmp_vp] (const unsigned day) {
                tmp_vp.remove(day);
                return true; // we don't want to stop
            };

            if (intersect(*vj, periods, l, meta, vp_modifier)) {
                vj->validity_patterns[level] = pt_data.get_or_create_validity_pattern(tmp_vp);
            }
        }
    }
}

VehicleJourney*
MetaVehicleJourney::get_vj_at_date(RTLevel level, const boost::gregorian::date& date) const{
    for (auto l : reverse_enum_range_from<RTLevel>(level)){
        for (auto& vj: rtlevel_to_vjs_map[l]) {
            if(vj->get_validity_pattern_at(l)->check(date)){
                return vj.get();
            };
        }
    }
    return nullptr;
}

std::vector<VehicleJourney*>
MetaVehicleJourney::get_vjs_in_period(RTLevel level,
                                      const std::vector<boost::posix_time::time_period>& periods,
                                      const MetaData& meta,
                                      const Route* filtering_route) const {
    std::vector<VehicleJourney*> res;
    for (auto l: reverse_enum_range_from<RTLevel>(level)) {
        for (auto& vj: rtlevel_to_vjs_map[l]) {
            if (filtering_route && vj->route != filtering_route) { continue; }
            auto func = [] (const unsigned /*day*/) {
                return false; // we want to stop as soon as we know the vj intersec the period
            };
            if (intersect(*vj, periods, l, meta, func)) {
                res.push_back(vj.get());
            }
        }
    }
    return res;
}


static_data * static_data::instance = 0;
static_data * static_data::get() {
    if (instance == 0) {
        static_data* temp = new static_data();

        boost::assign::insert(temp->types_string)
                (Type_e::ValidityPattern, "validity_pattern")
                (Type_e::Line, "line")
                (Type_e::LineGroup, "line_group")
                (Type_e::JourneyPattern, "journey_pattern")
                (Type_e::VehicleJourney, "vehicle_journey")
                (Type_e::StopPoint, "stop_point")
                (Type_e::StopArea, "stop_area")
                (Type_e::Network, "network")
                (Type_e::PhysicalMode, "physical_mode")
                (Type_e::CommercialMode, "commercial_mode")
                (Type_e::Connection, "connection")
                (Type_e::JourneyPatternPoint, "journey_pattern_point")
                (Type_e::Company, "company")
                (Type_e::Way, "way")
                (Type_e::Coord, "coord")
                (Type_e::Address, "address")
                (Type_e::Route, "route")
                (Type_e::POI, "poi")
                (Type_e::POIType, "poi_type")
                (Type_e::Contributor, "contributor")
                (Type_e::Calendar, "calendar");

        boost::assign::insert(temp->modes_string)
                (Mode_e::Walking, "walking")
                (Mode_e::Bike, "bike")
                (Mode_e::Car, "car")
                (Mode_e::Bss, "bss");
        instance = temp;

    }
    return instance;
}

Type_e static_data::typeByCaption(const std::string & type_str) {
    return instance->types_string.right.at(type_str);
}

std::string static_data::captionByType(Type_e type){
    return instance->types_string.left.at(type);
}

Mode_e static_data::modeByCaption(const std::string & mode_str) {
    return instance->modes_string.right.at(mode_str);
}

template<typename T> std::vector<idx_t> indexes(const std::vector<T*>& elements){
    std::vector<idx_t> result;
    for(T* element : elements){
        result.push_back(element->idx);
    }
    return result;
}

Calendar::Calendar(boost::gregorian::date beginning_date) : validity_pattern(beginning_date) {}

void Calendar::build_validity_pattern(boost::gregorian::date_period production_period) {
    //initialisation of the validity pattern from the active periods and the exceptions
    for (boost::gregorian::date_period period : this->active_periods) {
        auto intersection_period = production_period.intersection(period);
        if (intersection_period.is_null()) {
            continue;
        }
        validity_pattern.add(intersection_period.begin(), intersection_period.end(), week_pattern);
    }
    for (navitia::type::ExceptionDate exd : this->exceptions) {
        if (!production_period.contains(exd.date)) {
            continue;
        }
        if (exd.type == ExceptionDate::ExceptionType::sub) {
            validity_pattern.remove(exd.date);
        } else if (exd.type == ExceptionDate::ExceptionType::add) {
            validity_pattern.add(exd.date);
        }
    }
}

bool VehicleJourney::operator<(const VehicleJourney& other) const {
    if (this->route->uri != other.route->uri) {
        return this->route->uri < other.route->uri;
    }
    return this->uri < other.uri;
}

std::vector<idx_t> Calendar::get(Type_e type, const PT_Data & data) const{
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::Line:{
        // if the method is slow, adding a list of lines in calendar
        for(Line* line: data.lines) {
            for(Calendar* cal : line->calendar_list) {
                if(cal == this) {
                    result.push_back(line->idx);
                    break;
                }
            }
        }
    }
    break;
    default : break;
    }
    return result;
}
std::vector<idx_t> StopArea::get(Type_e type, const PT_Data & data) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::StopPoint: return indexes(this->stop_point_list);
    case Type_e::CommercialMode:
    case Type_e::PhysicalMode:{
         std::set<idx_t> tmp_result;
        for(const auto& stp: stop_point_list){
            std::vector<idx_t> tmp = stp->get(type, data);
            for(const idx_t idx: tmp){
                tmp_result.insert(idx);
            }
        }
        if(tmp_result.size() > 0){
            for(const idx_t idx: tmp_result){
                result.push_back(idx);
            }
        }
    }
        break;
    default: break;
    }
    return result;
}

std::vector<idx_t> Network::get(Type_e type, const PT_Data &) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::Line: return indexes(line_list);
    default: break;
    }
    return result;
}


std::vector<idx_t> Company::get(Type_e type, const PT_Data &) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::Line: return indexes(line_list);
    default: break;
    }
    return result;
}

std::vector<idx_t> CommercialMode::get(Type_e type, const PT_Data &) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::Line: return indexes(line_list);
    default: break;
    }
    return result;
}

std::vector<idx_t> PhysicalMode::get(Type_e type, const PT_Data & data) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::VehicleJourney:
        for (const auto* vj: data.vehicle_journeys) {
            if (vj->physical_mode != this) { continue; }
            result.push_back(vj->idx);
        }
        break;
    default: break;
    }
    return result;
}

std::vector<idx_t> Line::get(Type_e type, const PT_Data&) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::CommercialMode: result.push_back(commercial_mode->idx); break;
    case Type_e::Company: return indexes(company_list);
    case Type_e::Network: result.push_back(network->idx); break;
    case Type_e::Route: return indexes(route_list);
    case Type_e::Calendar: return indexes(calendar_list);
    case Type_e::LineGroup: return indexes(line_group_list);
    default: break;
    }
    return result;
}

type::hasOdtProperties Line::get_odt_properties() const{
    type::hasOdtProperties result;
    if (!this->route_list.empty()){
        for (const auto route : this->route_list) {
            result |= route->get_odt_properties();
        }
    }
    return result;
}

std::vector<idx_t> LineGroup::get(Type_e type, const PT_Data&) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::Line: return indexes(line_list);
    default: break;
    }
    return result;
}

std::vector<idx_t> Route::get(Type_e type, const PT_Data &) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::Line: result.push_back(line->idx); break;
    case Type_e::VehicleJourney:
        for_each_vehicle_journey([&](const VehicleJourney& vj) {
                result.push_back(vj.idx);
                return true;
            });
        break;
    default: break;
    }
    return result;
}

type::hasOdtProperties Route::get_odt_properties() const{
    type::hasOdtProperties result;
    for_each_vehicle_journey([&](const VehicleJourney& vj) {
            type::hasOdtProperties cur;
            cur.set_estimated(vj.has_datetime_estimated());
            cur.set_zonal(vj.has_zonal_stop_point());
            result.odt_properties |= cur.odt_properties;
            return true;
        });
    return result;
}

std::vector<idx_t> VehicleJourney::get(Type_e type, const PT_Data &) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::Route: result.push_back(route->idx); break;
    case Type_e::Company: result.push_back(company->idx); break;
    case Type_e::PhysicalMode: result.push_back(physical_mode->idx); break;
    case Type_e::ValidityPattern: result.push_back(base_validity_pattern()->idx); break;
    default: break;
    }
    return result;
}

VehicleJourney::~VehicleJourney() {}
FrequencyVehicleJourney::~FrequencyVehicleJourney() {}
DiscreteVehicleJourney::~DiscreteVehicleJourney() {}

std::vector<idx_t> StopPoint::get(Type_e type, const PT_Data&) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::StopArea: result.push_back(stop_area->idx); break;
    case Type_e::Connection:
    case Type_e::StopPointConnection:
        for (const StopPointConnection* stop_cnx : stop_point_connection_list)
            result.push_back(stop_cnx->idx);
        break;
    default: break;
    }
    return result;
}

std::vector<idx_t> StopPointConnection::get(Type_e type, const PT_Data & ) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::StopPoint:
        result.push_back(this->departure->idx);
        result.push_back(this->destination->idx);
        break;
    default: break;
    }
    return result;
}
bool StopPointConnection::operator<(const StopPointConnection& other) const { return this < &other; }

std::string to_string(ExceptionDate::ExceptionType t) {
    switch (t) {
    case ExceptionDate::ExceptionType::add:
        return "Add";
    case ExceptionDate::ExceptionType::sub:
        return "Sub";
    default:
        throw navitia::exception("unhandled exception type");
    }
}

EntryPoint::EntryPoint(const Type_e type, const std::string &uri, int access_duration) : type(type), uri(uri), access_duration(access_duration) {
   // Gestion des adresses
   if (type == Type_e::Address){
       std::vector<std::string> vect;
       vect = split_string(uri, ":");
       if(vect.size() == 3){
           this->uri = vect[0] + ":" + vect[1];
           this->house_number = str_to_int(vect[2]);
       }
   }
   if(type == Type_e::Coord){
       size_t pos2 = uri.find(":", 6);
       try{
           if(pos2 != std::string::npos) {
               this->coordinates.set_lon(boost::lexical_cast<double>(uri.substr(6, pos2 - 6)));
               this->coordinates.set_lat(boost::lexical_cast<double>(uri.substr(pos2+1)));
           }
       }catch(boost::bad_lexical_cast){
           this->coordinates.set_lon(0);
           this->coordinates.set_lat(0);
       }
   }
}

EntryPoint::EntryPoint(const Type_e type, const std::string &uri) : EntryPoint(type, uri, 0) { }

void StreetNetworkParams::set_filter(const std::string &param_uri){
    size_t pos = param_uri.find(":");
    if(pos == std::string::npos)
        type_filter = Type_e::Unknown;
    else {
        uri_filter = param_uri;
        type_filter = static_data::get()->typeByCaption(param_uri.substr(0,pos));
    }
}

std::ostream& operator<<(std::ostream& os, const Mode_e& mode) {
    switch (mode) {
    case Mode_e::Walking: return os << "walking";
    case Mode_e::Bike: return os << "bike";
    case Mode_e::Car: return os << "car";
    case Mode_e::Bss: return os << "bss";
    default: return os << "[unknown mode]";
    }
}

}} //namespace navitia::type

#if BOOST_VERSION <= 105700
BOOST_CLASS_EXPORT_GUID(navitia::type::DiscreteVehicleJourney, "DiscreteVehicleJourney")
BOOST_CLASS_EXPORT_GUID(navitia::type::FrequencyVehicleJourney, "FrequencyVehicleJourney")
#endif
