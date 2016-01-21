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
#include <set>
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

ValidityPattern VehicleJourney::get_base_canceled_validity_pattern() const {
    ValidityPattern base_canceled_vp = *validity_patterns[realtime_level];
    base_canceled_vp.days >>= shift;
    return base_canceled_vp;
}

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

std::vector<boost::shared_ptr<disruption::Impact>> HasMessages::get_impacts() const {
    clean_up_weak_ptr(impacts);
    std::vector<boost::shared_ptr<disruption::Impact>> result;
    for (const auto impact: impacts) {
        auto impact_sptr = impact.lock();
        if (impact_sptr == nullptr) { continue; }
        result.push_back(impact_sptr);
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


StopTime StopTime::clone() const{
    StopTime ret{arrival_time, departure_time, stop_point};
    ret.properties = properties;
    ret.local_traffic_zone = local_traffic_zone;
    ret.vehicle_journey = vehicle_journey;
    ret.shape_from_prev = shape_from_prev;
    return ret;
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


ValidityPattern VehicleJourney::get_vp_of_sp(const StopPoint& sp,
        RTLevel rt_level,
        const boost::posix_time::time_period& period) const {

    ValidityPattern vp_for_stop_time{validity_patterns[rt_level]->beginning_date};

    auto pass_by_the_sp = [&](const nt::StopTime& stop_time){
       if (stop_time.stop_point->uri != sp.uri) {
           return;
       }
       const auto& beginning_date = validity_patterns[rt_level]->beginning_date;
       for (size_t i  = 0; i < validity_patterns[rt_level]->days.size(); ++i) {
           if (! validity_patterns[rt_level]->days.test(i)) {
               // the VJ doesn't run at day i
               continue;
           }
           auto circulating_day = beginning_date + boost::gregorian::days{static_cast<int>(i)};
           auto arrival_time_utc = stop_time.get_arrival_utc(circulating_day);
           if (period.contains(arrival_time_utc)) {
               auto days = (circulating_day - vp_for_stop_time.beginning_date).days();
               vp_for_stop_time.add(days);
           }
       }
    };
    boost::for_each(stop_time_list, pass_by_the_sp);
    return vp_for_stop_time;

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

template <typename F>
static bool concerns_base_at_period(const VehicleJourney& vj,
                                    const std::vector<bt::time_period>& periods,
                                    const F& fun,
                                    bool check_past_midnight = true) {
    bool intersect = false;
    // we only need to check on the base canceled vp
    ValidityPattern concerned_vp = vj.get_base_canceled_validity_pattern();
    for (const auto& period: periods) {
        //we can impact a vj with a departure the day before who past midnight
        namespace bg = boost::gregorian;
        bg::day_iterator titr(period.begin().date() - bg::days(check_past_midnight ? 1 : 0));
        for (; titr <= period.end().date(); ++titr) {
            // check that day is not out of concerned_vp bound, then if day is concerned by concerned_vp
            if (*titr < concerned_vp.beginning_date) { continue; }
            size_t day((*titr - concerned_vp.beginning_date).days());
            if (day >= concerned_vp.days.size() || ! concerned_vp.check(day)) { continue; }
            // we check on the execution period of base-schedule vj, as impact are targeted on this period
            const auto base_vj = vj.meta_vj->get_base_vj_circulating_at_date(*titr);
            if (! base_vj) { continue; }
            if (period.intersects(base_vj->execution_period(*titr))) {
                intersect = true;
                // calling fun() on concerned day of base vj
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
                                       const ValidityPattern& canceled_vp,
                                       Route* route,
                                       std::vector<StopTime> sts,
                                       nt::PT_Data& pt_data) {
    namespace ndtu = navitia::DateTimeUtils;
    // creating the vj
    auto vj_ptr = std::make_unique<VJ>();
    VJ* ret = vj_ptr.get();
    vj_ptr->meta_vj = this;
    vj_ptr->uri = uri;
    vj_ptr->idx = pt_data.vehicle_journeys.size();
    vj_ptr->realtime_level = level;
    vj_ptr->shift = 0;
    if (!sts.empty()) {
        vj_ptr->shift = sts.front().arrival_time / (ndtu::SECONDS_PER_DAY);
    }
    ValidityPattern model_new_vp{canceled_vp};
    model_new_vp.days <<= vj_ptr->shift; // shift validity pattern
    auto* new_vp = pt_data.get_or_create_validity_pattern(model_new_vp);
    for (const auto l: enum_range<RTLevel>()) {
        if (l < level) {
            auto* empty_vp = pt_data.get_or_create_validity_pattern(ValidityPattern(new_vp->beginning_date));
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
    // as date management is taken care of in validity pattern,
    // we have to contain first stop_time in [00:00 ; 24:00[ (and propagate to other st)
    for (nt::StopTime& st: vj_ptr->stop_time_list) {
        st.arrival_time -= ndtu::SECONDS_PER_DAY * vj_ptr->shift;
        st.departure_time -= ndtu::SECONDS_PER_DAY * vj_ptr->shift;
        // a vj cannot be longer than 24h and its start is contained in [00:00 ; 24:00[
        assert(st.arrival_time >= 0);
        assert(st.arrival_time < ndtu::SECONDS_PER_DAY * 2);
        assert(st.departure_time >= 0);
        assert(st.departure_time < ndtu::SECONDS_PER_DAY * 2);
    }

    // Desactivating the other vjs. The last creation has priority on
    // all the already existing vjs.
    const auto mask = ~canceled_vp.days;
    for_all_vjs([&] (VehicleJourney& vj) {
            for (const auto l: enum_range_from(level)) {
                auto new_vp = *vj.validity_patterns[l];
                new_vp.days &= (mask << vj.shift);
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
                                        const ValidityPattern& canceled_vp,
                                        Route* route,
                                        std::vector<StopTime> sts,
                                        nt::PT_Data& pt_data) {
    return impl_create_vj<FrequencyVehicleJourney>(uri, level, canceled_vp, route, std::move(sts), pt_data);
}

DiscreteVehicleJourney*
MetaVehicleJourney::create_discrete_vj(const std::string& uri,
                                       const RTLevel level,
                                       const ValidityPattern& canceled_vp,
                                       Route* route,
                                       std::vector<StopTime> sts,
                                       nt::PT_Data& pt_data) {
    return impl_create_vj<DiscreteVehicleJourney>(uri, level, canceled_vp, route, std::move(sts), pt_data);
}


void MetaVehicleJourney::cancel_vj(RTLevel level,
                                   const std::vector<boost::posix_time::time_period>& periods,
                                   nt::PT_Data& pt_data,
                                   const Route* filtering_route) {
    for (auto vj_level: reverse_enum_range_from<RTLevel>(level)) {
        for (auto& vj: rtlevel_to_vjs_map[vj_level]) {
            // for each vj, we want to cancel vp at all levels above cancel level
            for (auto vp_level: enum_range_from<RTLevel>(level)) {
                // note: we might want to cancel only the vj of certain routes
                if (filtering_route && vj->route != filtering_route) { continue; }
                nt::ValidityPattern tmp_vp(*vj->get_validity_pattern_at(vp_level));
                auto vp_modifier = [&] (const unsigned day) {
                    // day concerned is computed from base vj,
                    // so we have to shift when canceling it on realtime vj
                    tmp_vp.remove(day + vj->shift);
                    return true; // we don't want to stop
                };

                if (concerns_base_at_period(*vj, periods, vp_modifier)) {
                    vj->validity_patterns[vp_level] = pt_data.get_or_create_validity_pattern(tmp_vp);
                }
            }
        }
    }
}

VehicleJourney*
MetaVehicleJourney::get_base_vj_circulating_at_date(const boost::gregorian::date& date) const {
    for (auto l: reverse_enum_range_from<RTLevel>(RTLevel::Base)) {
        for (const auto& vj: rtlevel_to_vjs_map[l]) {
            if (vj->get_validity_pattern_at(l)->check(date)) {
                return vj.get();
            }
        }
    }
    return nullptr;
}

int16_t VehicleJourney::utc_to_local_offset() const {
    const auto* vp = validity_patterns[realtime_level];
    if (! vp) {
        throw navitia::recoverable_exception("vehicle journey " + uri +
                                 " not valid, no validitypattern on " + get_string_from_rt_level(realtime_level));
    }
    return meta_vj->tz_handler->get_first_utc_offset(*vp);
}

const VehicleJourney* VehicleJourney::get_corresponding_base() const {
    auto shifted_vj = get_base_canceled_validity_pattern();
    for (const auto& vj: meta_vj->get_base_vj()) {
        // if the validity pattern intersects
        if ((shifted_vj.days & vj->base_validity_pattern()->days).any()) {
            return vj.get();
        }
    }
    return nullptr;
}

std::vector<idx_t> MetaVehicleJourney::get(Type_e type, const PT_Data& data) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::VehicleJourney:
        for_all_vjs([&](const VehicleJourney& vj) {
            result.push_back(vj.idx);
        });
    break;
    case Type_e::Impact: return data.get_impacts_idx(get_impacts());
    default: break;
    }
    return result;
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
                (Type_e::Calendar, "calendar")
                (Type_e::MetaVehicleJourney, "trip")
                (Type_e::Impact, "disruption")
                (Type_e::Frame, "frame");

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
    case Type_e::Impact: return data.get_impacts_idx(get_impacts());

    default: break;
    }
    return result;
}

std::vector<idx_t> Network::get(Type_e type, const PT_Data& data) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::Line: return indexes(line_list);
    case Type_e::Impact: return data.get_impacts_idx(get_impacts());
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

std::vector<idx_t> Line::get(Type_e type, const PT_Data& data) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::CommercialMode:
        if (this->commercial_mode){
            result.push_back(commercial_mode->idx);
        }
        break;
    case Type_e::PhysicalMode: return indexes(physical_mode_list);
    case Type_e::Company: return indexes(company_list);
    case Type_e::Network: result.push_back(network->idx); break;
    case Type_e::Route: return indexes(route_list);
    case Type_e::Calendar: return indexes(calendar_list);
    case Type_e::LineGroup: return indexes(line_group_list);
    case Type_e::Impact: return data.get_impacts_idx(get_impacts());
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

std::string Line::get_label() const {
    //LineName = NetworkName + ModeName + LineCode + (LineName)
    std::stringstream s;
    if (network) { s << network->name << " "; }
    if (commercial_mode) { s << commercial_mode->name << " "; }
    s << code << " (" << name << ")";
    return s.str();
}

std::string Route::get_label() const {
    //RouteName = NetworkName + ModeName + LineCode + (RouteName)
    std::stringstream s;
    if (line->network) { s << line->network->name << " "; }
    if (line->commercial_mode) { s << line->commercial_mode->name << " "; }
    s << line->code << " (" << name << ")";
    return s.str();
}

std::vector<idx_t> LineGroup::get(Type_e type, const PT_Data&) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::Line: return indexes(line_list);
    default: break;
    }
    return result;
}

std::vector<idx_t> Route::get(Type_e type, const PT_Data& data) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::Line: result.push_back(line->idx); break;
    case Type_e::VehicleJourney:
        for_each_vehicle_journey([&](const VehicleJourney& vj) {
                result.push_back(vj.idx);
                return true;
            });
        break;
    case Type_e::Impact: return data.get_impacts_idx(get_impacts());
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

std::vector<boost::shared_ptr<disruption::Impact>>
VehicleJourney::get_impacts() const {
    std::vector<boost::shared_ptr<disruption::Impact>> result;
    // considering which impact concerns this vj
    for (const auto impact: meta_vj->get_impacts()) {
        // checking if impact concerns the period where this vj is valid (base-schedule centric)
        auto vp_functor = [&] (const unsigned) {
            return false; // we don't need to carry on when we find a day concerned
        };
        if (concerns_base_at_period(*this, impact->application_periods, vp_functor, false)) {
            result.push_back(impact);
        }
    }
    return result;
}

std::vector<idx_t> VehicleJourney::get(Type_e type, const PT_Data& data) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::Route: result.push_back(route->idx); break;
    case Type_e::Company: result.push_back(company->idx); break;
    case Type_e::PhysicalMode: result.push_back(physical_mode->idx); break;
    case Type_e::ValidityPattern: result.push_back(base_validity_pattern()->idx); break;
    case Type_e::MetaVehicleJourney: result.push_back(meta_vj->idx); break;
    case Type_e::Frame: result.push_back(frame->idx); break;
    case Type_e::Impact: return data.get_impacts_idx(get_impacts());
    default: break;
    }
    return result;
}

VehicleJourney::~VehicleJourney() {}
FrequencyVehicleJourney::~FrequencyVehicleJourney() {}
DiscreteVehicleJourney::~DiscreteVehicleJourney() {}

std::vector<idx_t> StopPoint::get(Type_e type, const PT_Data& data) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::StopArea: result.push_back(stop_area->idx); break;
    case Type_e::Connection:
    case Type_e::StopPointConnection:
        for (const StopPointConnection* stop_cnx : stop_point_connection_list)
            result.push_back(stop_cnx->idx);
        break;
    case Type_e::Impact: return data.get_impacts_idx(get_impacts());
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

std::vector<idx_t> Frame::get(Type_e type, const PT_Data&) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::Contributor: result.push_back(contributor->idx); break;
    case Type_e::VehicleJourney: return indexes(vehiclejourney_list);
    default: break;
    }
    return result;
}

std::vector<idx_t> Contributor::get(Type_e type, const PT_Data&) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::Frame: return indexes(frame_list);
    default: break;
    }
    return result;
}

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

}} //namespace navitia::type

#if BOOST_VERSION <= 105700
BOOST_CLASS_EXPORT_GUID(navitia::type::DiscreteVehicleJourney, "DiscreteVehicleJourney")
BOOST_CLASS_EXPORT_GUID(navitia::type::FrequencyVehicleJourney, "FrequencyVehicleJourney")
#endif
