/* Copyright © 2001-2022, Hove and/or its affiliates. All rights reserved.

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

#include "apply_disruption.h"

#include "type/datetime.h"
#include "type/base_pt_objects.h"
#include "type/network.h"
#include "type/physical_mode.h"
#include "type/message.h"
#include "type/meta_data.h"
#include "type/meta_vehicle_journey.h"
#include "type/pt_data.h"
#include "utils/logger.h"
#include "utils/map_find.h"
#include "utils/functions.h"

#include <boost/container/flat_set.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/make_shared.hpp>
#include <boost/range/algorithm/for_each.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <algorithm>
#include <utility>

namespace navitia {

namespace nt = navitia::type;
namespace nu = navitia::utils;
namespace ndtu = navitia::DateTimeUtils;

namespace {

nt::VehicleJourney* create_vj_from_old_vj(nt::MetaVehicleJourney* mvj,
                                          const nt::VehicleJourney* vj,
                                          const std::string& new_vj_uri,
                                          nt::RTLevel rt_level,
                                          const nt::ValidityPattern& canceled_vp,
                                          std::vector<nt::StopTime> new_stop_times,
                                          nt::PT_Data& pt_data) {
    auto* company = vj->company;
    auto vehicle_journey_type = vj->vehicle_journey_type;
    auto odt_message = vj->odt_message;
    auto vehicle_properties = vj->_vehicle_properties;

    auto* new_vj = mvj->create_discrete_vj(new_vj_uri, vj->name, vj->headsign, rt_level, canceled_vp, vj->route,
                                           std::move(new_stop_times), pt_data);
    vj = nullptr;  // after create_discrete_vj, the vj can have been deleted

    new_vj->company = company;
    new_vj->vehicle_journey_type = vehicle_journey_type;
    new_vj->odt_message = odt_message;
    new_vj->_vehicle_properties = vehicle_properties;

    if (!mvj->get_base_vj().empty()) {
        new_vj->physical_mode = mvj->get_base_vj().at(0)->physical_mode;
        new_vj->name = mvj->get_base_vj().at(0)->name;
        new_vj->headsign = mvj->get_base_vj().at(0)->headsign;
    } else {
        // If we set nothing for physical_mode, it'll crash when building raptor
        new_vj->physical_mode = pt_data.physical_modes[0];
    }
    /*
     * Properties manually added to guarantee the good behavior for raptor and consistency.
     * */
    new_vj->physical_mode->vehicle_journey_list.push_back(new_vj);
    return new_vj;
}

std::string make_new_vj_uri(const nt::MetaVehicleJourney* mvj, nt::RTLevel rt_level) {
    boost::uuids::random_generator gen;
    return "vehicle_journey:" + mvj->uri + ":" + type::get_string_from_rt_level(rt_level) + ":"
           + boost::uuids::to_string(gen());
}

struct apply_impacts_visitor : public boost::static_visitor<> {
    boost::shared_ptr<nt::disruption::Impact> impact;
    nt::PT_Data& pt_data;
    const nt::MetaData& meta;
    std::string action;
    nt::RTLevel rt_level;  // level of the impacts
    log4cplus::Logger log = log4cplus::Logger::getInstance("log");

    apply_impacts_visitor(boost::shared_ptr<nt::disruption::Impact> impact,
                          nt::PT_Data& pt_data,
                          const nt::MetaData& meta,
                          std::string action,
                          nt::RTLevel l)
        : impact(std::move(impact)), pt_data(pt_data), meta(meta), action(std::move(action)), rt_level(l) {}

    virtual ~apply_impacts_visitor() = default;
    apply_impacts_visitor(const apply_impacts_visitor&) = default;
    apply_impacts_visitor& operator=(const apply_impacts_visitor& other) = delete;
    apply_impacts_visitor& operator=(apply_impacts_visitor&& other) noexcept = delete;

    virtual void operator()(nt::MetaVehicleJourney*, nt::Route* = nullptr) = 0;

    void log_start_action(const std::string& uri) {
        LOG4CPLUS_TRACE(log, "Start to " << action << " impact " << impact->uri << " on object " << uri);
    }

    void log_end_action(const std::string& uri) {
        LOG4CPLUS_TRACE(log, "Finished to " << action << " impact " << impact->uri << " on object " << uri);
    }

    void operator()(nt::disruption::UnknownPtObj& /*unused*/) {}

    void operator()(nt::Network* network) {
        this->log_start_action(network->uri);
        for (auto line : network->line_list) {
            this->operator()(line);
        }
        this->log_end_action(network->uri);
    }

    void operator()(nt::Line* line) {
        this->log_start_action(line->uri);
        for (auto route : line->route_list) {
            this->operator()(route);
        }
        this->log_end_action(line->uri);
    }

    void operator()(nt::Route* route) {
        this->log_start_action(route->uri);

        // we cannot ensure that all VJ of a MetaVJ are on the same route,
        // and since we want all actions to operate on MetaVJ, we collect all MetaVJ of the route
        // (but we'll change only the route's vj)
        std::unordered_set<nt::MetaVehicleJourney*> mvjs;
        route->for_each_vehicle_journey([&mvjs](nt::VehicleJourney& vj) {
            mvjs.insert(vj.meta_vj);
            return true;
        });
        for (auto* mvj : mvjs) {
            this->operator()(mvj, route);
        }
        this->log_end_action(route->uri);
    }
};

// Computes the vp corresponding to the days where base vj's are disrupted
type::ValidityPattern compute_base_disrupted_vp(const std::vector<boost::posix_time::time_period>& disrupted_vj_periods,
                                                const boost::gregorian::date_period& production_period) {
    type::ValidityPattern vp{production_period.begin()};  // bitset are all initialised to 0
    for (const auto& period : disrupted_vj_periods) {
        auto start_date = period.begin().date();
        if (!production_period.contains(start_date)) {
            continue;
        }
        // we may impact vj's passing midnight but all we care is start date
        auto day = (start_date - production_period.begin()).days();
        vp.add(day);
    }
    return vp;
}

nt::Route* get_or_create_route(const nt::disruption::Impact& impact, nt::PT_Data& pt_data) {
    nt::Network* network = pt_data.get_or_create_network("network:additional_service", "additional service");
    nt::CommercialMode* comm_mode =
        pt_data.get_or_create_commercial_mode("commercial_mode:additional_service", "additional service");

    // We get the first and last stop_area to create route and line
    const auto& st_depart = impact.aux_info.stop_times.front();
    const auto& sa_depart = st_depart.stop_time.stop_point->stop_area;
    const auto& st_arrival = impact.aux_info.stop_times.back();
    const auto& sa_arrival = st_arrival.stop_time.stop_point->stop_area;
    std::string line_uri = "line:" + sa_depart->uri + "_" + sa_arrival->uri;
    std::string line_name = sa_depart->name + " - " + sa_arrival->name;
    std::string route_uri = "route:" + sa_depart->uri + "_" + sa_arrival->uri;
    std::string route_name = sa_depart->name + " - " + sa_arrival->name;
    nt::Line* line = pt_data.get_or_create_line(line_uri, line_name, network, comm_mode);
    nt::Route* route = pt_data.get_or_create_route(route_uri, route_name, line, sa_arrival, "forward");

    return route;
}

nt::StopTime clone_stop_time_and_shift(const nt::StopTime& st, const nt::VehicleJourney* vj) {
    nt::StopTime new_st = st.clone();
    // Here the first arrival/departure time may be > 24hours.
    new_st.arrival_time = st.arrival_time + ndtu::SECONDS_PER_DAY * vj->shift;
    new_st.departure_time = st.departure_time + ndtu::SECONDS_PER_DAY * vj->shift;
    new_st.alighting_time = st.alighting_time + ndtu::SECONDS_PER_DAY * vj->shift;
    new_st.boarding_time = st.boarding_time + ndtu::SECONDS_PER_DAY * vj->shift;
    return new_st;
}

std::vector<nt::StopTime> handle_reduced_service(const nt::VehicleJourney* vj,
                                                 const nt::disruption::ImpactedVJ& impacted_vj,
                                                 const nt::disruption::RailSection& rs) {
    log4cplus::Logger logger = log4cplus::Logger::getInstance("log");
    std::vector<nt::StopTime> new_stop_times;
    const auto begin = impacted_vj.impacted_ranks.begin();
    const auto end = impacted_vj.impacted_ranks.rbegin();
    for (const auto& st : vj->stop_time_list) {
        // We need to get the associated base stop_time to compare its rank
        const auto base_st = st.get_base_stop_time();
        if (base_st == nullptr) {
            LOG4CPLUS_TRACE(logger, "Ignoring stop " << st.stop_point->uri << "on " << vj->uri);
            continue;
        }
        const auto& base_st_order = base_st->order();
        if (impacted_vj.impacted_ranks.count(base_st_order)) {
            if (*begin == base_st_order && rs.is_start_stop(base_st->stop_point->stop_area->uri)
                && rs.is_blocked_start_point()) {
                continue;
            }
            const auto& last_stop_time = vj->stop_time_list.back();
            if (*end == base_st_order && rs.is_end_stop(base_st->stop_point->stop_area->uri)
                && st.order() == last_stop_time.order() && rs.is_blocked_end_point()) {
                continue;
            }
            if (*begin != base_st_order && *end != base_st_order) {
                continue;
            }
        }
        nt::StopTime new_st = clone_stop_time_and_shift(st, vj);
        // A stop_time created by realtime should be initialized with skipped_stop = false
        new_st.set_skipped_stop(false);
        new_stop_times.push_back(std::move(new_st));
    }
    return new_stop_times;
}

std::vector<nt::StopTime> handle_no_service(const nt::VehicleJourney* vj,
                                            const nt::disruption::ImpactedVJ& impacted_vj) {
    log4cplus::Logger logger = log4cplus::Logger::getInstance("log");
    std::vector<nt::StopTime> new_stop_times;
    const auto begin = impacted_vj.impacted_ranks.begin();
    for (const auto& st : vj->stop_time_list) {
        // We need to get the associated base stop_time to compare its rank
        const auto base_st = st.get_base_stop_time();
        if (base_st == nullptr) {
            LOG4CPLUS_TRACE(logger, "Ignoring stop " << st.stop_point->uri << "on " << vj->uri);
            continue;
        }
        const auto& base_st_order = base_st->order();
        nt::StopTime new_st = clone_stop_time_and_shift(st, vj);
        if (*begin == base_st_order) {
            new_st.set_pick_up_allowed(false);
        }
        // A stop_time created by realtime should be initialized with skipped_stop = false
        new_st.set_skipped_stop(false);
        new_stop_times.push_back(std::move(new_st));
        if (*begin == base_st_order) {
            break;
        }
    }
    return new_stop_times;
}

struct add_impacts_visitor : public apply_impacts_visitor {
    add_impacts_visitor(const boost::shared_ptr<nt::disruption::Impact>& impact,
                        nt::PT_Data& pt_data,
                        const nt::MetaData& meta,
                        nt::RTLevel l)
        : apply_impacts_visitor(impact, pt_data, meta, "add", l) {}

    using apply_impacts_visitor::operator();

    void operator()(nt::MetaVehicleJourney* mvj, nt::Route* r = nullptr) override {
        log_start_action(mvj->uri);
        if (impact->severity->effect == nt::disruption::Effect::NO_SERVICE) {
            LOG4CPLUS_TRACE(log, "canceling " << mvj->uri);
            mvj->cancel_vj(rt_level, impact->application_periods, pt_data, r);
            mvj->push_unique_impact(impact);
        } else if (  // we don't want to apply delay or detour without stoptime's information
                     // if there is no stoptimes it should be modeled as a NO_SERVICE
                     // else it is something else, like for example a SIGNIFICANT_DELAYS on a line
                     // and in this case we do not have enough information to apply the impact
            !impact->aux_info.stop_times.empty()) {
            LOG4CPLUS_TRACE(log, "modifying " << mvj->uri);
            auto canceled_vp = compute_base_disrupted_vp(impact->application_periods, meta.production_date);

            if (!r) {
                if (!mvj->get_base_vj().empty()) {
                    r = mvj->get_base_vj().at(0)->route;
                } else {
                    r = get_or_create_route(*impact, pt_data);
                }
            }

            auto new_vj_uri = make_new_vj_uri(mvj, rt_level);

            std::vector<type::StopTime> stoptimes;  // we copy all the stoptimes
            for (const auto& stu : impact->aux_info.stop_times) {
                stoptimes.push_back(stu.stop_time);
            }

            // Create new VJ (default name/headsign is empty)
            auto* vj = mvj->create_discrete_vj(new_vj_uri, "", "", type::RTLevel::RealTime, canceled_vp, r,
                                               std::move(stoptimes), pt_data);
            LOG4CPLUS_TRACE(log, "New vj has been created " << vj->uri);

            // Add company
            if (!impact->company_id.empty()) {
                nu::make_map_find(pt_data.companies_map, impact->company_id)
                    .if_found([&](navitia::type::Company* c) {
                        vj->company = c;
                        LOG4CPLUS_TRACE(
                            log, "[disruption] Associate company into new VJ. Company id : " << impact->company_id);
                    })
                    .if_not_found([&]() {
                        // for protection, use the companies[0]
                        // TODO : Create default company
                        vj->company = pt_data.companies[0];
                        LOG4CPLUS_WARN(
                            log, "[disruption] Associate random company to new VJ. Company doesn't exist with id : "
                                     << impact->company_id);
                    });
            } else {
                if (!mvj->get_base_vj().empty()) {
                    vj->company = mvj->get_base_vj().at(0)->company;
                } else {
                    // for protection, use the companies[0]
                    // TODO : Create default company
                    vj->company = pt_data.companies[0];
                    LOG4CPLUS_WARN(log,
                                   "[disruption] Associate random company to new VJ because base VJ doesn't exist");
                }
            }

            // Add physical mode:
            // Use base-VJ's mode if exists,
            // fallback on impact's mode
            // last resort is picking physical_modes[0]
            if (!mvj->get_base_vj().empty()) {
                vj->physical_mode = mvj->get_base_vj().at(0)->physical_mode;
            } else if (!impact->physical_mode_id.empty()) {  // fallback on impact's mode
                nu::make_map_find(pt_data.physical_modes_map, impact->physical_mode_id)
                    .if_found([&](navitia::type::PhysicalMode* p) {
                        vj->physical_mode = p;
                        LOG4CPLUS_TRACE(log, "[disruption] Associate physical mode into new VJ. Physical mode id : "
                                                 << impact->physical_mode_id);
                    })
                    .if_not_found([&]() {
                        // for protection, use the physical_modes[0]
                        // TODO : Create default physical mode
                        vj->physical_mode = pt_data.physical_modes[0];
                        LOG4CPLUS_WARN(log,
                                       "[disruption] Associate random physical mode to new VJ. Physical mode doesn't "
                                       "exist with id : "
                                           << impact->physical_mode_id);
                    });
            } else {
                // for protection, use the physical_modes[0] instead of creating a new one
                vj->physical_mode = pt_data.physical_modes[0];
                LOG4CPLUS_WARN(log,
                               "[disruption] Associate random physical mode to new VJ because base VJ doesn't exist");
            }

            // Use the corresponding base stop_time for boarding and alighting duration
            for (auto& st : vj->stop_time_list) {
                const auto base_st = st.get_base_stop_time();
                if (base_st) {
                    st.boarding_time = st.departure_time - base_st->get_boarding_duration();
                    st.alighting_time = st.arrival_time + base_st->get_alighting_duration();
                }
            }

            // name and dataset
            if (!mvj->get_base_vj().empty()) {
                vj->name = mvj->get_base_vj().at(0)->name;
                vj->headsign = mvj->get_base_vj().at(0)->headsign;
                vj->dataset = mvj->get_base_vj().at(0)->dataset;
            } else {
                // Affect the headsign to vj if present in gtfs-rt
                if (!impact->headsign.empty()) {
                    vj->headsign = impact->headsign;
                    vj->name = impact->headsign;
                    pt_data.headsign_handler.change_name_and_register_as_headsign(*vj, impact->headsign);
                }

                // for protection, use the datasets[0]
                // TODO : Create default data set
                vj->dataset = pt_data.datasets[0];
                LOG4CPLUS_WARN(
                    log, "[disruption] Associate random dataset to new VJ doesn't work because base VJ doesn't exist");
            }
            vj->physical_mode->vehicle_journey_list.push_back(vj);
            // we need to associate the stoptimes to the created vj
            for (auto& stu : impact->aux_info.stop_times) {
                stu.stop_time.vehicle_journey = vj;
            }
            mvj->push_unique_impact(impact);
        } else {
            LOG4CPLUS_DEBUG(log, "unhandled action on " << mvj->uri);
        }
        log_end_action(mvj->uri);
    }

    void operator()(const nt::disruption::RailSection& rs) {
        std::string uri = "new rail section, start: " + rs.start->uri + " - end: " + rs.end->uri;
        if (rs.line) {
            uri += ", line : " + rs.line->uri;
        } else if (!rs.routes.empty()) {
            for (const auto& r : rs.routes) {
                uri += ", route : " + r->uri;
            }
        } else {
            LOG4CPLUS_ERROR(log, "Unhandled " << uri << ". We need line or routes");
            return;
        }
        this->log_start_action(uri);

        auto blocking_effects = {nt::disruption::Effect::NO_SERVICE, nt::disruption::Effect::DETOUR,
                                 nt::disruption::Effect::REDUCED_SERVICE};
        if (!navitia::contains(blocking_effects, impact->severity->effect)) {
            LOG4CPLUS_DEBUG(log, "Unhandled action on " << uri);
            this->log_end_action(uri);
            return;
        }

        LOG4CPLUS_TRACE(log, "canceling " << uri);

        // Get all impacted VJs and compute the corresponding base_canceled vp
        auto impacted_vjs = nt::disruption::get_impacted_vehicle_journeys(rs, *impact, meta.production_date, rt_level);

        // Loop on each affected vj
        for (auto& impacted_vj : impacted_vjs) {
            std::vector<nt::StopTime> new_stop_times;
            const std::string& vj_uri = impacted_vj.vj_uri;
            LOG4CPLUS_TRACE(log, "Impacted vj : " << vj_uri);
            auto vj_iterator = pt_data.vehicle_journeys_map.find(vj_uri);
            if (vj_iterator == pt_data.vehicle_journeys_map.end()) {
                LOG4CPLUS_TRACE(log, "impacted vj : " << vj_uri << " not found in data. I ignore it.");
                continue;
            }
            if (impacted_vj.impacted_ranks.empty()) {
                LOG4CPLUS_TRACE(log, "impacted vj : " << vj_uri << " without impacted_ranks data. I ignore it.");
                continue;
            }

            nt::VehicleJourney* vj = vj_iterator->second;
            auto& new_vp = impacted_vj.new_vp;

            if (impact->severity->effect == nt::disruption::Effect::REDUCED_SERVICE
                || impact->severity->effect == nt::disruption::Effect::DETOUR) {
                // we remove all stop_times of vj with a rank that appears in impacted_vj.impacted_ranks
                // except start_stop and end_stop
                new_stop_times = handle_reduced_service(vj, impacted_vj, rs);
            } else {
                new_stop_times = handle_no_service(vj, impacted_vj);
            }

            auto mvj = vj->meta_vj;
            mvj->push_unique_impact(impact);

            // If all stop times have been ignored
            if (new_stop_times.empty()) {
                LOG4CPLUS_DEBUG(log, "All stop times has been ignored on " << vj->uri << ". Cancelling it.");
                mvj->cancel_vj(rt_level, impact->application_periods, pt_data);
                continue;
            }
            auto new_vj_uri = make_new_vj_uri(mvj, rt_level);

            new_vp.days = new_vp.days & (vj->validity_patterns[rt_level]->days >> vj->shift);

            LOG4CPLUS_TRACE(log, "meta_vj : " << mvj->uri << " \n  old_vj: " << vj->uri
                                              << " to be deleted \n new_vj_uri " << new_vj_uri);
            auto* new_vj =
                create_vj_from_old_vj(mvj, vj, new_vj_uri, rt_level, new_vp, std::move(new_stop_times), pt_data);
            vj = nullptr;  // after the call to create_vj, vj may have been deleted :(

            LOG4CPLUS_TRACE(log, "new_vj: " << new_vj->uri << " is created");
        }
        this->log_end_action(uri);
    }

    void operator()(nt::disruption::LineSection& ls) {
        std::string uri = "line section (" + ls.line->uri + " : " + ls.start_point->uri + "/" + ls.end_point->uri + ")";
        this->log_start_action(uri);

        auto blocking_effects = {nt::disruption::Effect::NO_SERVICE, nt::disruption::Effect::DETOUR,
                                 nt::disruption::Effect::REDUCED_SERVICE};
        if (!navitia::contains(blocking_effects, impact->severity->effect)) {
            LOG4CPLUS_DEBUG(log, "Unhandled action on " << uri);
            this->log_end_action(uri);
            return;
        }

        LOG4CPLUS_TRACE(log, "canceling " << uri);

        // Get all impacted VJs and compute the corresponding base_canceled vp
        auto impacted_vjs = nt::disruption::get_impacted_vehicle_journeys(ls, *impact, meta.production_date, rt_level);

        // Loop on each affected vj
        for (auto& impacted_vj : impacted_vjs) {
            std::vector<nt::StopTime> new_stop_times;
            const std::string& vj_uri = impacted_vj.vj_uri;
            LOG4CPLUS_TRACE(log, "Impacted vj : " << vj_uri);
            auto vj_iterator = pt_data.vehicle_journeys_map.find(vj_uri);
            if (vj_iterator == pt_data.vehicle_journeys_map.end()) {
                LOG4CPLUS_TRACE(log, "impacted vj : " << vj_uri << " not found in data. I ignore it.");
                continue;
            }
            nt::VehicleJourney* vj = vj_iterator->second;
            auto& new_vp = impacted_vj.new_vp;

            for (const auto& st : vj->stop_time_list) {
                // We need to get the associated base stop_time to compare its rank
                const auto base_st = st.get_base_stop_time();
                // stop is ignored if its stop_point is not in impacted_stops
                // if we don't find an associated base we keep it
                if (base_st && impacted_vj.impacted_ranks.count(base_st->order())) {
                    LOG4CPLUS_TRACE(log, "Ignoring stop " << st.stop_point->uri << "on " << vj->uri);
                    continue;
                }
                nt::StopTime new_st = clone_stop_time_and_shift(st, vj);
                new_stop_times.push_back(std::move(new_st));
            }

            auto mvj = vj->meta_vj;
            mvj->push_unique_impact(impact);

            // If all stop times have been ignored
            if (new_stop_times.empty()) {
                LOG4CPLUS_DEBUG(log, "All stop times has been ignored on " << vj->uri << ". Cancelling it.");
                mvj->cancel_vj(rt_level, impact->application_periods, pt_data);
                continue;
            }
            auto new_vj_uri = make_new_vj_uri(mvj, rt_level);

            new_vp.days = new_vp.days & (vj->validity_patterns[rt_level]->days >> vj->shift);

            LOG4CPLUS_TRACE(log, "meta_vj : " << mvj->uri << " \n  old_vj: " << vj->uri
                                              << " to be deleted \n new_vj_uri " << new_vj_uri);
            auto* new_vj =
                create_vj_from_old_vj(mvj, vj, new_vj_uri, rt_level, new_vp, std::move(new_stop_times), pt_data);
            vj = nullptr;  // after the call to create_vj, vj may have been deleted :(

            LOG4CPLUS_TRACE(log, "new_vj: " << new_vj->uri << " is created");
        }
        this->log_end_action(uri);
    }

    void operator()(nt::StopPoint* stop_point) {
        log_start_action(stop_point->uri);

        if (impact->severity->effect != nt::disruption::Effect::NO_SERVICE
            && impact->severity->effect != nt::disruption::Effect::DETOUR) {
            LOG4CPLUS_DEBUG(log, "Unhandled action on " << stop_point->uri << " for stop point");
            this->log_end_action(stop_point->uri);
            return;
        }

        using namespace boost::posix_time;
        using namespace boost::gregorian;

        // Computing a validity_pattern of impact used to pre-filter concerned vjs later
        type::ValidityPattern impact_vp = impact->get_impact_vp(meta.production_date);

        // Get all impacted VJs and compute the corresponding base_canceled vp
        std::vector<std::pair<const nt::VehicleJourney*, nt::ValidityPattern>> vj_vp_pairs;

        /*
         * In this loop, we'are going to find all Vjs that are impacted by the closure of the stop point
         * and the validity pattern of the new Vj to be created in the next step
         *
         * */
        for (const auto* vj : pt_data.vehicle_journeys) {
            /*
             * Pre-filtering by validity pattern, which allows us to check if the vj is impacted quickly
             *
             * Since the validity pattern runs only by day not by hour, we'll compute in detail to
             * check if the vj is really impacted or not.
             *
             * */
            if ((vj->validity_patterns[rt_level]->days & impact_vp.days).none()) {
                continue;
            }

            LOG4CPLUS_TRACE(log, "VJ: " << vj->uri << " may be impacted");

            bool stop_point_found = false;
            for (const nt::StopTime& stop_time : vj->stop_time_list) {
                if (stop_time.stop_point->uri == stop_point->uri) {
                    stop_point_found = true;
                    break;
                }
            }
            if (!stop_point_found) {
                continue;
            }

            nt::ValidityPattern new_vp{vj->validity_patterns[rt_level]->beginning_date};

            /*
             * In this loop, we check in detail if the vj is impacted.
             *
             * If the stop time corresponding to the impacted stop point falls in the impact period,
             * we say that this vj is impacted and the computed validity pattern will be the vp of the new vj.
             *
             *
             *  Day     1              2               3               4               5               6        ...
             *          ---------------------------------------------------------------------------------------------
             * SP_bob         8:30           8:30             8:30           8:30           8:30           8:30 ...(vj)
             *
             * Period_bob           |--------------|
             *                    17:00          14:00
             *
             * Period_pop                            |------|
             *                                     17:00   8:00
             *
             * Let's say we have a vj passes on stop point SP_bob at 8:30 every day.
             *
             * Here comes the first impact bob which will have SP_bob closed, like it's figured in the comment,
             * even though this impact begins on Day1, it impacts only on Day2 actually. So the new vj's validity
             * pattern will be like "...00010"
             *
             * Here comes another impact pop on SP_bob, this time the impact pop won't make any effects on vj, because
             * none of corresponding stop time falls in its period, the new vj's validity pattern will be like "..0000".
             *
             * */
            for (const auto& period : impact->application_periods) {
                new_vp.days |= vj->get_vp_of_sp(*stop_point, rt_level, period).days;
            }

            if (new_vp.days.none()) {
                // The vj doesn't stop at the impacted stop_point during all the given impact periods
                LOG4CPLUS_TRACE(log, "VJ: " << vj->uri << " is not impacted");
                continue;
            }
            LOG4CPLUS_TRACE(log, "VJ: " << vj->uri << " is impacted");

            /*
             * >> A shift? WTF is this? <<<
             *
             *  Day     1              2               3               4               5               6      ...
             *          ------------------------------------------------------------------------------------------
             *   VJ               |------------|
             *                  23:00         12:00
             *
             *  Delayed_VJ                         |------------|
             *                                   23:00         12:00
             *
             *
             *  impacted_vj                              |-----|
             *                                          1:00  12:00
             *
             *
             * Like it's showed in the figure, a vj circulates from 23:00 Day1 to 12:00 Day2, its vp is "...0001"
             *
             * We get a delay message and the vj is delayed for 24 hours, the dalayed_vj has a vp "...010" with a
             * shift equals to 1.(The 1 means the Delayed_VJ has shift one day regarding to it's base vj, this is
             * important for computing the base vj's vp at adapted/realtime level.)
             *
             * Now we have to close some stop point of Delayed_vj on Day2 for some stupid reasons, the new impacted
             * vj circulates actually only on Day3 ("....00100").
             *
             * This case is testd in apply_disruption_test/test_shift_of_a_disrupted_delayed_train. One can play with
             * that test for a better understanding.
             *
             * */
            new_vp.days >>= vj->shift;

            vj_vp_pairs.emplace_back(vj, new_vp);
        }

        for (auto& vj_vp : vj_vp_pairs) {
            std::vector<nt::StopTime> new_stop_times;
            const auto* vj = vj_vp.first;
            auto& new_vp = vj_vp.second;

            for (const auto& st : vj->stop_time_list) {
                if (st.stop_point == stop_point) {
                    continue;
                }
                nt::StopTime new_st = clone_stop_time_and_shift(st, vj);
                new_stop_times.push_back(std::move(new_st));
            }

            auto mvj = vj->meta_vj;
            mvj->push_unique_impact(impact);

            auto new_vj_uri = make_new_vj_uri(mvj, rt_level);

            new_vp.days = new_vp.days & (vj->validity_patterns[rt_level]->days >> vj->shift);

            auto* new_vj =
                create_vj_from_old_vj(mvj, vj, new_vj_uri, rt_level, new_vp, std::move(new_stop_times), pt_data);
            vj = nullptr;  // after the call to create_vj, vj may have been deleted :(

            LOG4CPLUS_TRACE(log, "new_vj: " << new_vj->uri << " is created");
        }
        log_end_action(stop_point->uri);
    }

    void operator()(nt::StopArea* stop_area) {
        log_start_action(stop_area->uri);
        for (auto* stop_point : stop_area->stop_point_list) {
            LOG4CPLUS_TRACE(log, "Dispatching stop_area impact to stop_point: " << stop_point->uri);
            (*this)(stop_point);
        }
        log_end_action(stop_area->uri);
    }
};

bool is_modifying_effect(nt::disruption::Effect e) {
    // check if the effect needs to modify the model
    return contains({nt::disruption::Effect::NO_SERVICE, nt::disruption::Effect::UNKNOWN_EFFECT,
                     nt::disruption::Effect::SIGNIFICANT_DELAYS, nt::disruption::Effect::MODIFIED_SERVICE,
                     nt::disruption::Effect::DETOUR, nt::disruption::Effect::REDUCED_SERVICE,
                     nt::disruption::Effect::ADDITIONAL_SERVICE},
                    e);
}

void apply_impact(const boost::shared_ptr<nt::disruption::Impact>& impact,
                  nt::PT_Data& pt_data,
                  const nt::MetaData& meta) {
    if (!is_modifying_effect(impact->severity->effect)) {
        LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("log"),
                        "Ingoring impact: " << impact->uri << " the effect is not handled");
        return;
    }
    LOG4CPLUS_TRACE(log4cplus::Logger::getInstance("log"), "Adding impact: " << impact->uri);

    add_impacts_visitor v(impact, pt_data, meta, impact->disruption->rt_level);
    boost::for_each(impact->mut_informed_entities(), boost::apply_visitor(v));
    LOG4CPLUS_TRACE(log4cplus::Logger::getInstance("log"), impact->uri << " impact added");
}

using impact_sptr = boost::shared_ptr<nt::disruption::Impact>;

auto comp = [](const impact_sptr& lhs, const impact_sptr& rhs) {
    // lexical sort by update datetime then uri
    if (lhs->updated_at != rhs->updated_at) {
        return lhs->updated_at < rhs->updated_at;
    }
    return lhs->uri < rhs->uri;
};

struct delete_impacts_visitor : public apply_impacts_visitor {
    size_t nb_vj_reassigned = 0;
    std::set<impact_sptr, decltype(comp)> disruptions_collection{comp};
    delete_impacts_visitor(const boost::shared_ptr<nt::disruption::Impact>& impact,
                           nt::PT_Data& pt_data,
                           const nt::MetaData& meta,
                           nt::RTLevel l)
        : apply_impacts_visitor(impact, pt_data, meta, "delete", l) {}

    delete_impacts_visitor(const delete_impacts_visitor&) = delete;
    delete_impacts_visitor& operator=(const delete_impacts_visitor&) = delete;

    delete_impacts_visitor(delete_impacts_visitor&&) = delete;
    delete_impacts_visitor& operator=(delete_impacts_visitor&&) = delete;

    ~delete_impacts_visitor() override {
        for (const auto& i : disruptions_collection) {
            if (i) {
                apply_disruption(*i->disruption, pt_data, meta);
            }
        }
    }

    using apply_impacts_visitor::operator();

    // We set all the validity pattern to the theorical one, we will re-apply
    // other disruptions after
    void operator()(nt::MetaVehicleJourney* mvj, nt::Route* /*r*/ = nullptr) override {
        mvj->remove_impact(impact);
        for (auto& vj : mvj->get_base_vj()) {
            // Time to reset the vj
            // We re-activate base vj for every realtime level by reseting base vj's vp to base
            vj->validity_patterns[type::RTLevel::RealTime] = vj->validity_patterns[type::RTLevel::Adapted] =
                vj->validity_patterns[type::RTLevel::Base];
        }
        auto* empty_vp_ptr = pt_data.get_or_create_validity_pattern({meta.production_date.begin()});

        auto set_empty_vp = [empty_vp_ptr](const std::unique_ptr<type::VehicleJourney>& vj) {
            vj->validity_patterns[type::RTLevel::RealTime] = vj->validity_patterns[type::RTLevel::Adapted] =
                vj->validity_patterns[type::RTLevel::Base] = empty_vp_ptr;
        };
        // We deactivate adapted/realtime vj by setting vp to empty vp
        boost::for_each(mvj->get_adapted_vj(), set_empty_vp);
        boost::for_each(mvj->get_rt_vj(), set_empty_vp);

        const auto& impact = this->impact;
        boost::range::remove_erase_if(mvj->modified_by, [&impact](const boost::weak_ptr<nt::disruption::Impact>& i) {
            auto spt = i.lock();
            return (spt) ? spt == impact : true;
        });

        // add_impacts_visitor populate mvj->modified_by, thus we swap
        // it with an empty vector.
        decltype(mvj->modified_by) modified_by_moved;
        boost::swap(modified_by_moved, mvj->modified_by);

        for (const auto& wptr : modified_by_moved) {
            if (auto share_ptr = wptr.lock()) {
                /*
                 * Do not reapply the same disruption. If we have more than one impact in it
                 * we would reapply all of its impacts (even the one we are deleting).
                 * We need to keep the link to remaining impacts of the disruption though, because
                 * they will be deleted after this one. They must stay in modified_by for that.
                 *
                 * /!\ WARNING /!\
                 * This is working because we never delete a single impact of a disruption.
                 * delete_impact is only called by delete_disruption which delete every impacts.
                 * If this was not the case we would need to find a way to reapply part of a
                 * disruption's impacts, and not all of them, after each deletion of an impact.
                 */
                if (share_ptr->disruption->uri != impact->disruption->uri) {
                    disruptions_collection.insert(share_ptr);
                } else {
                    mvj->push_unique_impact(share_ptr);
                }
            }
        }
        // we check if we now have useless vehicle_journeys to cleanup
        mvj->clean_up_useless_vjs(pt_data);
    }

    void operator()(nt::StopPoint* stop_point) {
        stop_point->remove_impact(impact);
        auto find_impact = [&](const boost::weak_ptr<nt::disruption::Impact>& weak_ptr) {
            if (auto i = weak_ptr.lock()) {
                return i->uri == impact->uri;
            }
            return false;
        };
        for (auto& mvj : pt_data.meta_vjs) {
            if (std::any_of(std::begin(mvj->modified_by), std::end(mvj->modified_by), find_impact)) {
                (*this)(mvj.get());
            };
        }
    }

    void operator()(nt::StopArea* stop_area) {
        stop_area->remove_impact(impact);
        for (auto* sp : stop_area->stop_point_list) {
            (*this)(sp);
        }
    }

    void operator()(nt::Network* network) {
        network->remove_impact(impact);
        apply_impacts_visitor::operator()(network);
    }

    void operator()(nt::Line* line) {
        line->remove_impact(impact);
        apply_impacts_visitor::operator()(line);
    }

    void operator()(nt::Route* route) {
        route->remove_impact(impact);
        apply_impacts_visitor::operator()(route);
    }

    void operator()(nt::disruption::LineSection& /*unused*/) {
        auto find_impact = [&](const boost::weak_ptr<nt::disruption::Impact>& weak_ptr) {
            if (auto i = weak_ptr.lock()) {
                return i->uri == impact->uri;
            }
            return false;
        };
        for (auto& mvj : pt_data.meta_vjs) {
            if (std::any_of(std::begin(mvj->modified_by), std::end(mvj->modified_by), find_impact)) {
                (*this)(mvj.get());
            };
        }
    }

    void operator()(nt::disruption::RailSection& /*unused*/) {
        auto find_impact = [&](const boost::weak_ptr<nt::disruption::Impact>& weak_ptr) {
            if (auto i = weak_ptr.lock()) {
                return i->uri == impact->uri;
            }
            return false;
        };
        for (auto& mvj : pt_data.meta_vjs) {
            if (std::any_of(std::begin(mvj->modified_by), std::end(mvj->modified_by), find_impact)) {
                (*this)(mvj.get());
            };
        }
    }
};

void delete_impact(const boost::shared_ptr<nt::disruption::Impact>& impact,
                   nt::PT_Data& pt_data,
                   const nt::MetaData& meta) {
    if (!is_modifying_effect(impact->severity->effect)) {
        return;
    }
    auto log = log4cplus::Logger::getInstance("log");
    LOG4CPLUS_DEBUG(log, "Deleting impact: " << impact->uri);
    delete_impacts_visitor v(impact, pt_data, meta, impact->disruption->rt_level);
    boost::for_each(impact->mut_informed_entities(), boost::apply_visitor(v));
    LOG4CPLUS_DEBUG(log, impact->uri << " deleted");
}
}  // anonymous namespace

void delete_disruption(const std::string& disruption_id, nt::PT_Data& pt_data, const nt::MetaData& meta) {
    auto log = log4cplus::Logger::getInstance("log");
    LOG4CPLUS_DEBUG(log, "Deleting disruption: " << disruption_id);

    nt::disruption::DisruptionHolder& holder = pt_data.disruption_holder;

    // the disruption is deleted by RAII
    if (auto disruption = holder.pop_disruption(disruption_id)) {
        for (const auto& impact : disruption->get_impacts()) {
            delete_impact(impact, pt_data, meta);
        }
    }
    holder.clean_weak_impacts();
    LOG4CPLUS_DEBUG(log, "disruption " << disruption_id << " deleted");
}

void apply_disruption(const type::disruption::Disruption& disruption,
                      nt::PT_Data& pt_data,
                      const navitia::type::MetaData& meta) {
    LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("log"), "applying disruption: " << disruption.uri);
    for (const auto& impact : disruption.get_impacts()) {
        apply_impact(impact, pt_data, meta);
    }
}

}  // namespace navitia
