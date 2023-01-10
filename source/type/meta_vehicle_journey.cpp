/* Copyright �� 2001-2022, Hove and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Hove (www.hove.com).
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
channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#include "meta_vehicle_journey.h"

#include "type/concerns_base_at_period.h"
#include "type/pt_data.h"
#include "type/route.h"
#include "type/dataset.h"
#include "type/physical_mode.h"
#include "type/serialization.h"
#include "type/type_utils.h"
#include "utils/logger.h"

#include <boost/range/algorithm/find.hpp>

namespace nt = navitia::type;
namespace pt = boost::posix_time;

namespace navitia {
namespace type {

template <class Archive>
void MetaVehicleJourney::serialize(Archive& ar, const unsigned int /*unused*/) {
    ar& idx& uri& rtlevel_to_vjs_map& associated_calendars& impacts& modified_by& tz_handler;
}
SERIALIZABLE(MetaVehicleJourney)

namespace {

bool is_useless(const nt::VehicleJourney& vj) {
    // a vj is useless if all it's validity pattern are empty
    for (const auto l_vj : vj.validity_patterns) {
        if (!l_vj.second->empty()) {
            return false;
        }
    }
    return true;
}

template <typename C>
static void erase_vj_from_list(const nt::VehicleJourney* vj, C& vjs) {
    auto it = boost::range::find(vjs, vj);
    if (it == vjs.end()) {
        LOG4CPLUS_WARN(log4cplus::Logger::getInstance("logger"),
                       "impossible to find the vj " << vj->uri << " in the list, something is strange");
        return;
    }
    vjs.erase(it);
}

template <typename VJ>
std::vector<VJ*>& get_vjs(Route* r);
template <>
std::vector<DiscreteVehicleJourney*>& get_vjs(Route* r) {
    return r->discrete_vehicle_journey_list;
}
template <>
std::vector<FrequencyVehicleJourney*>& get_vjs(Route* r) {
    return r->frequency_vehicle_journey_list;
}

void cleanup_useless_vj_link(const nt::VehicleJourney* vj, nt::PT_Data& pt_data) {
    // clean all backref to a vehicle journey before deleting it
    // need to be thorough !!
    LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("logger"), "we are going to cleanup the vj " << vj->uri);

    if (vj->dataset) {
        erase_vj_from_list(vj, vj->dataset->vehiclejourney_list);
    }
    if (vj->physical_mode) {
        erase_vj_from_list(vj, vj->physical_mode->vehicle_journey_list);
    }
    if (dynamic_cast<const nt::FrequencyVehicleJourney*>(vj)) {
        erase_vj_from_list(vj, vj->route->frequency_vehicle_journey_list);
    } else if (dynamic_cast<const nt::DiscreteVehicleJourney*>(vj)) {
        erase_vj_from_list(vj, vj->route->discrete_vehicle_journey_list);
    }

    pt_data.headsign_handler.forget_vj(vj);
    pt_data.disruption_holder.forget_vj(vj);

    // remove the vj from the global list/map
    erase_vj_from_list(vj, pt_data.vehicle_journeys);
    // afterward, we MUST reindex all vehicle journeys
    std::for_each(pt_data.vehicle_journeys.begin() + vj->idx, pt_data.vehicle_journeys.end(), Indexer<nt::idx_t>(vj));

    pt_data.vehicle_journeys_map.erase(vj->uri);
}
}  // anonymous namespace

void MetaVehicleJourney::clean_up_useless_vjs(nt::PT_Data& pt_data) {
    std::vector<std::pair<RTLevel, size_t>> vj_idx_to_remove;
    for (const auto rt_vjs : rtlevel_to_vjs_map) {
        auto& vjs = rt_vjs.second;
        if (vjs.empty()) {
            continue;
        }
        // reverse iteration for later deletion
        for (int i = vjs.size() - 1; i >= 0; --i) {
            auto& vj = vjs[i];
            if (is_useless(*vj)) {
                vj_idx_to_remove.emplace_back(rt_vjs.first, i);
            }
        }
    }

    for (const auto& level_and_vj_idx_to_remove : vj_idx_to_remove) {
        auto rt_level = level_and_vj_idx_to_remove.first;
        auto vj_idx = level_and_vj_idx_to_remove.second;
        auto& vj = this->rtlevel_to_vjs_map[rt_level][vj_idx];

        cleanup_useless_vj_link(vj.get(), pt_data);

        // once all the links to the vj have been cleaned we can destroy the object
        // (by removing the unique_ptr from the vector)
        this->rtlevel_to_vjs_map[rt_level].erase(this->rtlevel_to_vjs_map[rt_level].begin() + vj_idx);
    }
}

template <typename VJ>
VJ* MetaVehicleJourney::impl_create_vj(const std::string& uri,
                                       const std::string& name,
                                       const std::string& headsign,
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
    vj_ptr->name = name;
    vj_ptr->headsign = headsign;
    vj_ptr->realtime_level = level;
    vj_ptr->shift = 0;
    if (!sts.empty()) {
        const auto& first_st = navitia::earliest_stop_time(sts);
        vj_ptr->shift = std::min(first_st.boarding_time, first_st.arrival_time) / (ndtu::SECONDS_PER_DAY);
    }
    ValidityPattern model_new_vp{canceled_vp};
    model_new_vp.days <<= vj_ptr->shift;  // shift validity pattern
    auto* new_vp = pt_data.get_or_create_validity_pattern(model_new_vp);
    for (const auto l : enum_range<RTLevel>()) {
        if (l < level) {
            auto* empty_vp = pt_data.get_or_create_validity_pattern(ValidityPattern(new_vp->beginning_date));
            vj_ptr->validity_patterns[l] = empty_vp;
        } else {
            vj_ptr->validity_patterns[l] = new_vp;
        }
    }
    vj_ptr->route = route;
    for (auto& st : sts) {
        st.vehicle_journey = ret;
        st.set_is_frequency(std::is_same<VJ, FrequencyVehicleJourney>::value);
    }
    vj_ptr->stop_time_list = std::move(sts);
    // as date management is taken care of in validity pattern,
    // we have to contain first stop_time in [00:00 ; 24:00[ (and propagate to other st)
    for (nt::StopTime& st : vj_ptr->stop_time_list) {
        st.arrival_time -= ndtu::SECONDS_PER_DAY * vj_ptr->shift;
        st.departure_time -= ndtu::SECONDS_PER_DAY * vj_ptr->shift;
        st.alighting_time -= ndtu::SECONDS_PER_DAY * vj_ptr->shift;
        st.boarding_time -= ndtu::SECONDS_PER_DAY * vj_ptr->shift;
        // a vj cannot be longer than 24h and its start is contained in [00:00 ; 24:00[
        assert(st.arrival_time >= 0);
        assert(st.arrival_time < ndtu::SECONDS_PER_DAY * 2);
        assert(st.departure_time >= 0);
        assert(st.departure_time < ndtu::SECONDS_PER_DAY * 2);
        assert(st.alighting_time >= 0);
        assert(st.alighting_time < ndtu::SECONDS_PER_DAY * 2);
        assert(st.boarding_time >= 0);
        assert(st.boarding_time < ndtu::SECONDS_PER_DAY * 2);
    }

    // Desactivating the other vjs. The last creation has priority on
    // all the already existing vjs.
    // Patch: we protect the BaseLevel as we should never have to modify VPs of this level.
    // Context: Sometimes from a unique trip we can have two vehiclejourney active the same day while
    // splitting a vehiclejourney with a period divided by DST and stop_times between 01:00 and 02:00.
    // We must keep the base VP as-is, as it's legitimate, but we used to remove duplicate days from VP.
    // After the VP cleanup, it can be empty, thus we eliminate the vehicle journey.
    // This elimination provokes some memory error if it's a BaseLevel VJ.
    // TODO: We could also have the same bug for other two levels and hence it has to be checked.
    // We must allow (todo) two VJ from the same trip on the same UTC day.
    const auto mask = ~canceled_vp.days;
    for_all_vjs([&](VehicleJourney& vj) {
        for (const auto l : enum_range_from(level)) {
            if (l != RTLevel::Base) {
                auto new_vp = *vj.validity_patterns[l];
                new_vp.days &= (mask << vj.shift);
                vj.validity_patterns[l] = pt_data.get_or_create_validity_pattern(new_vp);
            }
        }
    });

    pt::ptime begin = pt::microsec_clock::universal_time();
    // we clean up all the now useless vehicle journeys
    clean_up_useless_vjs(pt_data);
    LOG4CPLUS_INFO(log4cplus::Logger::getInstance("logger"),
                   "it took " << (pt::microsec_clock::universal_time() - begin).total_milliseconds()
                              << " ms to clean up useless vj");

    // inserting the vj in the model
    vj_ptr->idx = pt_data.vehicle_journeys.size();
    pt_data.vehicle_journeys.push_back(ret);
    pt_data.vehicle_journeys_map[ret->uri] = ret;
    if (route) {
        get_vjs<VJ>(route).push_back(ret);
    }
    rtlevel_to_vjs_map[level].emplace_back(std::move(vj_ptr));
    return ret;
}

FrequencyVehicleJourney* MetaVehicleJourney::create_frequency_vj(const std::string& uri,
                                                                 const std::string& name,
                                                                 const std::string& headsign,
                                                                 const RTLevel level,
                                                                 const ValidityPattern& canceled_vp,
                                                                 Route* route,
                                                                 std::vector<StopTime> sts,
                                                                 nt::PT_Data& pt_data) {
    return impl_create_vj<FrequencyVehicleJourney>(uri, name, headsign, level, canceled_vp, route, std::move(sts),
                                                   pt_data);
}

DiscreteVehicleJourney* MetaVehicleJourney::create_discrete_vj(const std::string& uri,
                                                               const std::string& name,
                                                               const std::string& headsign,
                                                               const RTLevel level,
                                                               const ValidityPattern& canceled_vp,
                                                               Route* route,
                                                               std::vector<StopTime> sts,
                                                               nt::PT_Data& pt_data) {
    return impl_create_vj<DiscreteVehicleJourney>(uri, name, headsign, level, canceled_vp, route, std::move(sts),
                                                  pt_data);
}

void MetaVehicleJourney::cancel_vj(RTLevel level,
                                   const std::vector<boost::posix_time::time_period>& periods,
                                   nt::PT_Data& pt_data,
                                   const Route* filtering_route) {
    for (auto vj_level : reverse_enum_range_from<RTLevel>(level)) {
        for (auto& vj : rtlevel_to_vjs_map[vj_level]) {
            // for each vj, we want to cancel vp at all levels above cancel level
            for (auto vp_level : enum_range_from<RTLevel>(level)) {
                // note: we might want to cancel only the vj of certain routes
                if (filtering_route && vj->route != filtering_route) {
                    continue;
                }
                nt::ValidityPattern tmp_vp(*vj->get_validity_pattern_at(vp_level));
                auto vp_modifier = [&](const unsigned day) {
                    // day concerned is computed from base vj,
                    // so we have to shift when canceling it on realtime vj
                    tmp_vp.remove(day + vj->shift);
                    return true;  // we don't want to stop
                };

                if (concerns_base_at_period(*vj, vp_level, periods, vp_modifier)) {
                    vj->validity_patterns[vp_level] = pt_data.get_or_create_validity_pattern(tmp_vp);
                }
            }
        }
    }
}

VehicleJourney* MetaVehicleJourney::get_base_vj_circulating_at_date(const boost::gregorian::date& date) const {
    for (auto l : reverse_enum_range_from<RTLevel>(RTLevel::Base)) {
        for (const auto& vj : rtlevel_to_vjs_map[l]) {
            if (vj->get_validity_pattern_at(l)->check(date)) {
                return vj.get();
            }
        }
    }
    return nullptr;
}

bool MetaVehicleJourney::is_already_modified_by(const boost::shared_ptr<disruption::Impact>& impact) {
    return std::any_of(std::begin(modified_by), std::end(modified_by),
                       [&](const boost::weak_ptr<nt::disruption::Impact>& i) { return i.lock() == impact; });
}

void MetaVehicleJourney::push_unique_impact(const boost::shared_ptr<disruption::Impact>& impact) {
    if (!is_already_modified_by(impact)) {
        modified_by.emplace_back(impact);
    }
}

}  // namespace type

}  // namespace navitia
