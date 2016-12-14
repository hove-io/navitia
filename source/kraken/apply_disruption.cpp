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

#include "apply_disruption.h"
#include "utils/logger.h"
#include "type/datetime.h"

#include <boost/make_shared.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/range/algorithm/for_each.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <algorithm>

namespace navitia {

namespace nt = navitia::type;
namespace ndtu = navitia::DateTimeUtils;
namespace bt = boost::posix_time;
namespace bg = boost::gregorian;

namespace {

struct apply_impacts_visitor : public boost::static_visitor<> {
    boost::shared_ptr<nt::disruption::Impact> impact;
    nt::PT_Data& pt_data;
    const nt::MetaData& meta;
    std::string action;
    nt::RTLevel rt_level; // level of the impacts
    log4cplus::Logger log = log4cplus::Logger::getInstance("log");

    apply_impacts_visitor(const boost::shared_ptr<nt::disruption::Impact>& impact,
            nt::PT_Data& pt_data, const nt::MetaData& meta, std::string action, nt::RTLevel l) :
        impact(impact), pt_data(pt_data), meta(meta), action(action), rt_level(l) {}

    virtual ~apply_impacts_visitor() {}
    apply_impacts_visitor(const apply_impacts_visitor&) = default;

    virtual void operator()(nt::MetaVehicleJourney*, nt::Route* = nullptr) = 0;

    void log_start_action(std::string uri) {
        LOG4CPLUS_TRACE(log, "Start to " << action << " impact " << impact.get()->uri << " on object " << uri);
    }

    void log_end_action(std::string uri) {
        LOG4CPLUS_TRACE(log, "Finished to " << action << " impact " << impact.get()->uri << " on object " << uri);
    }

    void operator()(nt::disruption::UnknownPtObj&) { }

    void operator()(nt::Network* network) {
        this->log_start_action(network->uri);
        for (auto line : network->line_list) {
            this->operator()(line);
        }
        this->log_end_action(network->uri);
    }

    void operator()(nt::Line* line) {
        this->log_start_action(line->uri);
        for(auto route : line->route_list) {
            this->operator()(route);
        }
        this->log_end_action(line->uri);
    }

    void operator()(nt::Route* route) {
        this->log_start_action(route->uri);

        // we cannot ensure that all VJ of a MetaVJ are on the same route,
        // and since we want all actions to operate on MetaVJ, we collect all MetaVJ of the route
        // (but we'll change only the route's vj)
        std::set<nt::MetaVehicleJourney*> mvjs;
        route->for_each_vehicle_journey([&mvjs](nt::VehicleJourney& vj) {
            mvjs.insert(vj.meta_vj); return true;
        });
        for (auto* mvj: mvjs) {
            this->operator()(mvj, route);
        }
        this->log_end_action(route->uri);
    }
};

// Computes the vp corresponding to the days where base vj's are disrupted
static type::ValidityPattern compute_base_disrupted_vp(
        const std::vector<boost::posix_time::time_period>& disrupted_vj_periods,
        const boost::gregorian::date_period& production_period) {
    type::ValidityPattern vp{production_period.begin()}; // bitset are all initialised to 0
    for (const auto& period: disrupted_vj_periods) {
        auto start_date = period.begin().date();
        if (! production_period.contains(start_date)) { continue; }
        // we may impact vj's passing midnight but all we care is start date
        auto day = (start_date - production_period.begin()).days();
        vp.add(day);
    }
    return vp;
}

static std::string concatenate_impact_uris(const nt::MetaVehicleJourney& mvj) {
    std::stringstream impacts_uris;
    for (auto& mvj_impacts : mvj.impacted_by) {
        if (auto i = mvj_impacts.lock()) {
            if (impacts_uris.str().find(i->disruption->uri) == std::string::npos) {
                impacts_uris << ":" << i->disruption->uri;
            }
        }
    }
    return impacts_uris.str();
}

struct add_impacts_visitor : public apply_impacts_visitor {
    add_impacts_visitor(const boost::shared_ptr<nt::disruption::Impact>& impact,
            nt::PT_Data& pt_data, const nt::MetaData& meta, nt::RTLevel l) :
        apply_impacts_visitor(impact, pt_data, meta, "add", l) {}

    ~add_impacts_visitor() {}
    add_impacts_visitor(const add_impacts_visitor&) = default;

    using apply_impacts_visitor::operator();

    void operator()(nt::MetaVehicleJourney* mvj, nt::Route* r = nullptr) {
        log_start_action(mvj->uri);
        if (impact->severity->effect == nt::disruption::Effect::NO_SERVICE) {
            LOG4CPLUS_TRACE(log, "canceling " << mvj->uri);
            mvj->cancel_vj(rt_level, impact->application_periods, pt_data, r);
            mvj->push_unique_impact(impact);
        } else if (impact->severity->effect == nt::disruption::Effect::SIGNIFICANT_DELAYS) {
            LOG4CPLUS_TRACE(log, "modifying " << mvj->uri);
            auto canceled_vp = compute_base_disrupted_vp(impact->application_periods,
                                                         meta.production_date);
            if (! r && ! mvj->get_base_vj().empty()) {
                r = mvj->get_base_vj().at(0)->route;
            }
            auto nb_rt_vj = mvj->get_rt_vj().size();
            std::string new_vj_uri = mvj->uri + ":modified:" + std::to_string(nb_rt_vj) + ":"
                    + impact->disruption->uri;
            std::vector<type::StopTime> stoptimes;  // we copy all the stoptimes
            for (const auto& stu: impact->aux_info.stop_times) {
                stoptimes.push_back(stu.stop_time);
            }
            auto* vj = mvj->create_discrete_vj(new_vj_uri,
                type::RTLevel::RealTime,
                canceled_vp,
                r,
                std::move(stoptimes),
                pt_data);
            LOG4CPLUS_TRACE(log, "New vj has been created " << vj->uri);
            if (! mvj->get_base_vj().empty()) {
                vj->physical_mode = mvj->get_base_vj().at(0)->physical_mode;
                vj->name = mvj->get_base_vj().at(0)->name; 
            } else {
                // If we set nothing for physical_mode, it'll crash when building raptor
                vj->physical_mode = pt_data.physical_modes[0];
                vj->name = new_vj_uri;
            }
            vj->physical_mode->vehicle_journey_list.push_back(vj);
            // we need to associate the stoptimes to the created vj
            for (auto& stu: impact->aux_info.stop_times) {
                stu.stop_time.vehicle_journey = vj;
            }
            mvj->push_unique_impact(impact);
        } else {
            LOG4CPLUS_DEBUG(log, "unhandled action on " << mvj->uri);
        }
        log_end_action(mvj->uri);
    }

    void operator()(nt::disruption::LineSection& ls) {
        std::string uri =
            "line section (" +  ls.line->uri  + " : " + ls.start_point->uri + "/" + ls.end_point->uri + ")";
        this->log_start_action(uri);

        if (impact->severity->effect != nt::disruption::Effect::NO_SERVICE) {
            LOG4CPLUS_DEBUG(log, "Unhandled action on " << uri);
            this->log_end_action(uri);
            return;
        }

        LOG4CPLUS_TRACE(log, "canceling " << uri);

        // Get all impacted VJs and compute the corresponding base_canceled vp
        using SectionBounds = const std::pair<boost::optional<uint16_t>, boost::optional<uint16_t>>;
        std::vector<std::tuple<const nt::VehicleJourney*, nt::ValidityPattern, SectionBounds>> vj_vp_pairs;

        // Computing a validity_pattern of impact used to pre-filter concerned vjs later
        type::ValidityPattern impact_vp = impact->get_impact_vp(meta.production_date);

        // Loop on impacted routes of the line section
        for(auto* route: ls.routes) {
            // Loop on each vj
            bool has_affected_vj(false);
            route->for_each_vehicle_journey([&](nt::VehicleJourney& vj) {
                /*
                 * Pre-filtering by validity pattern, which allows us to check if the vj is impacted quickly
                 *
                 * Since the validity pattern runs only by day not by hour, we'll compute in detail to
                 * check if the vj is really impacted or not.
                 *
                 * */
                if ((vj.validity_patterns[rt_level]->days & impact_vp.days).none()) {
                    return true;
                }

                // Filtering each journey to see if it's impacted by the section.
                SectionBounds bounds_st = vj.get_bounds_orders_for_section(ls.start_point, ls.end_point);
                // If the vj pass by both stops both elements will be different than nullptr, otherwise
                // it's not passing by both stops and should not be impacted
                if(bounds_st.first && bounds_st.second) {
                    // Once we know the line section is part of the vj we compute the vp for the adapted_vj
                    LOG4CPLUS_TRACE(log, "vj "<< vj.uri << " pass by both stops, might be affected.");
                    nt::ValidityPattern new_vp{vj.validity_patterns[rt_level]->beginning_date};
                    for(const auto& period : impact->application_periods) {
                        // get the vp of the section
                        new_vp.days |= vj.get_vp_for_section(bounds_st, rt_level, period).days;
                    }
                    // If there is effective days for the adapted vp we're keeping it
                    if(!new_vp.days.none()){
                        LOG4CPLUS_TRACE(log, "vj "<< vj.uri << " is affected, keeping it.");
                        new_vp.days >>= vj.shift;
                        has_affected_vj = true;
                        vj_vp_pairs.emplace_back(&vj, new_vp, bounds_st);
                    }
                }
                return true;
            });

            // The route isn't really impacted since no vj pass by the section
            if(!has_affected_vj) {
                LOG4CPLUS_DEBUG(log, "Route  "<< route->uri << " has no affected vj, removing impact.");
                route->remove_impact(impact);
            }
        }

        // Loop on each affected vj
        for (auto& vj_vp_section : vj_vp_pairs) {
            std::vector<nt::StopTime> new_stop_times;
            const auto* vj = std::get<0>(vj_vp_section);
            auto& new_vp = std::get<1>(vj_vp_section);
            auto& bounds_st = std::get<2>(vj_vp_section);

            bool ignore_stop(false);
            for (const auto& st : vj->stop_time_list) {
                // Ignore stop if it's the range of impacted stop_times
                ignore_stop |= (st.order() == *(bounds_st.first));
                if(ignore_stop) {
                    LOG4CPLUS_TRACE(log, "Ignoring stop " << st.stop_point->uri << "on " << vj->uri);
                    // Reset ignore_stop if it's the end stop_time
                    ignore_stop = !(st.order() == *(bounds_st.second));
                    continue;
                }
                nt::StopTime new_st = st.clone();
                new_st.arrival_time = st.arrival_time + ndtu::SECONDS_PER_DAY * vj->shift;
                new_st.departure_time = st.departure_time + ndtu::SECONDS_PER_DAY * vj->shift;
                new_stop_times.push_back(std::move(new_st));
            }

            auto mvj = vj->meta_vj;
            mvj->push_unique_impact(impact);

            // If all stop times have been ignored
            if(new_stop_times.empty()) {
                LOG4CPLUS_DEBUG(log, "All stop times has been ignored on "<< vj->uri << ". Cancelling it.");
                mvj->cancel_vj(rt_level, impact->application_periods, pt_data);
                continue;
            }
            auto nb_rt_vj = mvj->get_vjs_at(rt_level).size();
            std::string new_vj_uri = vj->uri + ":" +
                    type::get_string_from_rt_level(rt_level) + ":" +
                    std::to_string(nb_rt_vj) + ":" + impact->disruption->uri;

            new_vp.days = new_vp.days & (vj->validity_patterns[rt_level]->days >> vj->shift);

            auto* new_vj = mvj->create_discrete_vj(new_vj_uri,
                    rt_level,
                    new_vp,
                    vj->route,
                    std::move(new_stop_times),
                    pt_data);

            LOG4CPLUS_TRACE(log, "new_vj: "<< new_vj->uri << " is created");

            if (! mvj->get_base_vj().empty()) {
                new_vj->physical_mode = mvj->get_base_vj().at(0)->physical_mode;
                new_vj->name = mvj->get_base_vj().at(0)->name;
            } else {
                // If we set nothing for physical_mode, it'll crash when building raptor
                new_vj->physical_mode = pt_data.physical_modes[0];
                new_vj->name = new_vj_uri;
            }
            /*
             * Properties manually added to guarantee the good behavior for raptor and consistency.
             * */
            new_vj->physical_mode->vehicle_journey_list.push_back(new_vj);
            new_vj->company = vj->company;
            new_vj->vehicle_journey_type = vj->vehicle_journey_type;
            new_vj->odt_message = vj->odt_message;
            new_vj->_vehicle_properties = vj->_vehicle_properties;
        }
        this->log_end_action(uri);
    }

    void operator()(nt::StopPoint* stop_point) {
        log_start_action(stop_point->uri);

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
        for (const auto* vj: pt_data.vehicle_journeys) {

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

            LOG4CPLUS_TRACE(log,  "VJ: "<< vj->uri << " may be impacted");

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
            for(const auto& period : impact->application_periods) {
                new_vp.days |= vj->get_vp_of_sp(*stop_point, rt_level, period).days;
            }

            if(new_vp.days.none()){
                // The vj doesn't stop at the impacted stop_point during all the given impact periods
                LOG4CPLUS_TRACE(log,  "VJ: "<< vj->uri << " is not impacted");
                continue;
            }
            LOG4CPLUS_TRACE(log,  "VJ: "<< vj->uri << " is impacted");

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
                nt::StopTime new_st = st.clone();
                // Here the first arrival/departure time may be > 24hours.
                // Check the test case: apply_disruption_test/test_shift_of_a_disrupted_delayed_train for more details
                new_st.arrival_time = st.arrival_time + ndtu::SECONDS_PER_DAY * vj->shift;
                new_st.departure_time = st.departure_time + ndtu::SECONDS_PER_DAY * vj->shift;
                new_stop_times.push_back(std::move(new_st));
            }

            auto mvj = vj->meta_vj;
            mvj->push_unique_impact(impact);

            auto nb_rt_vj = mvj->get_vjs_at(rt_level).size();
            std::string new_vj_uri = mvj->uri + ":" +
                    type::get_string_from_rt_level(rt_level) + ":" +
                    std::to_string(nb_rt_vj) + concatenate_impact_uris(*mvj);

            new_vp.days = new_vp.days & (vj->validity_patterns[rt_level]->days >> vj->shift);

            auto* new_vj = mvj->create_discrete_vj(new_vj_uri,
                    rt_level,
                    new_vp,
                    vj->route,
                    std::move(new_stop_times),
                    pt_data);
            
            LOG4CPLUS_TRACE(log,  "new_vj: "<< new_vj->uri << " is created");

            if (! mvj->get_base_vj().empty()) {
                new_vj->physical_mode = mvj->get_base_vj().at(0)->physical_mode;
                new_vj->name = mvj->get_base_vj().at(0)->name;
            } else {
                // If we set nothing for physical_mode, it'll crash when building raptor
                new_vj->physical_mode = pt_data.physical_modes[0];
                new_vj->name = new_vj_uri;
            }
            /*
             * Properties manually added to guarantee the good behavior for raptor and consistency.
             * */
            new_vj->physical_mode->vehicle_journey_list.push_back(new_vj);
            new_vj->company = vj->company;
            new_vj->vehicle_journey_type = vj->vehicle_journey_type;
            new_vj->odt_message = vj->odt_message;
            new_vj->_vehicle_properties = vj->_vehicle_properties;

        }
        log_end_action(stop_point->uri);
    }

    void operator()(nt::StopArea* stop_area) {
        log_start_action(stop_area->uri);
        for (auto* stop_point: stop_area->stop_point_list) {
            LOG4CPLUS_TRACE(log,  "Dispatching stop_area impact to stop_point: "<< stop_point->uri);
            (*this)(stop_point);
        }
        log_end_action(stop_area->uri);
    }
};

static bool is_modifying_effect(nt::disruption::Effect e) {
    // check is the effect needs to modify the model
    return in(e, {nt::disruption::Effect::NO_SERVICE,
                  nt::disruption::Effect::SIGNIFICANT_DELAYS});
}

void apply_impact(boost::shared_ptr<nt::disruption::Impact> impact,
                         nt::PT_Data& pt_data, const nt::MetaData& meta) {
    if (! is_modifying_effect(impact->severity->effect)) {
        return;
    }
    LOG4CPLUS_TRACE(log4cplus::Logger::getInstance("log"), "Adding impact: " << impact->uri);

    add_impacts_visitor v(impact, pt_data, meta, impact->disruption->rt_level);
    boost::for_each(impact->informed_entities, boost::apply_visitor(v));
    LOG4CPLUS_TRACE(log4cplus::Logger::getInstance("log"), impact->uri << " impact added");
}

using impact_wptr = boost::weak_ptr<nt::disruption::Impact>;

static auto comp = [](const impact_wptr& lhs, const impact_wptr& rhs){
    auto i_lhs = lhs.lock();
    auto i_rhs = rhs.lock();

    // lexical sort by pointer then update datetime then uri
    if(!i_lhs || !i_rhs) {
       return i_lhs.get() < i_rhs.get();
    }
    if (i_lhs->updated_at != i_rhs->updated_at) {
        return i_lhs->updated_at < i_rhs->updated_at;
    }
    return i_lhs->uri < i_rhs->uri;
};

struct delete_impacts_visitor : public apply_impacts_visitor {
    size_t nb_vj_reassigned = 0;
    std::set<impact_wptr, decltype(comp)> disruptions_collection{comp};
    delete_impacts_visitor(boost::shared_ptr<nt::disruption::Impact> impact,
            nt::PT_Data& pt_data, const nt::MetaData& meta, nt::RTLevel l) :
        apply_impacts_visitor(impact, pt_data, meta, "delete", l) {}

    ~delete_impacts_visitor() override {
        for (const auto i : disruptions_collection) {
            if (auto spt = i.lock()) {
                apply_disruption(*spt->disruption, pt_data, meta);
            }
        }
    }

    using apply_impacts_visitor::operator();

    // We set all the validity pattern to the theorical one, we will re-apply
    // other disruptions after
    void operator()(nt::MetaVehicleJourney* mvj, nt::Route* /*r*/ = nullptr) {
        mvj->remove_impact(impact);
        for (auto& vj: mvj->get_base_vj()) {
            // Time to reset the vj
            // We re-activate base vj for every realtime level by reseting base vj's vp to base
            vj->validity_patterns[type::RTLevel::RealTime] =
                    vj->validity_patterns[type::RTLevel::Adapted] =
                            vj->validity_patterns[type::RTLevel::Base];
        }
        auto* empty_vp_ptr = pt_data.get_or_create_validity_pattern({meta.production_date.begin()});

        auto set_empty_vp = [empty_vp_ptr](const std::unique_ptr<type::VehicleJourney>& vj){
            vj->validity_patterns[type::RTLevel::RealTime] =
                    vj->validity_patterns[type::RTLevel::Adapted] =
                            vj->validity_patterns[type::RTLevel::Base] = empty_vp_ptr;
        };
        // We deactivate adapted/realtime vj by setting vp to empty vp
        boost::for_each(mvj->get_adapted_vj(), set_empty_vp);
        boost::for_each(mvj->get_rt_vj(), set_empty_vp);

        const auto& impact = this->impact;
        boost::range::remove_erase_if(mvj->impacted_by,
            [&impact](const boost::weak_ptr<nt::disruption::Impact>& i) {
                auto spt = i.lock();
                return (spt) ? spt == impact : true;
        });

        // add_impacts_visitor populate mvj->impacted_by, thus we swap
        // it with an empty vector.
        decltype(mvj->impacted_by) impacted_by_moved;
        boost::swap(impacted_by_moved, mvj->impacted_by);
        
        // note: insert(it_begin, it_end) crashed...
        for(const auto& i: impacted_by_moved) {
            disruptions_collection.insert(i);
        }
    }

    void operator()(nt::StopPoint* stop_point) {
        stop_point->remove_impact(impact);
        auto find_impact = [&](const boost::weak_ptr<nt::disruption::Impact>& weak_ptr) {
            if (auto i = weak_ptr.lock()){
                return i->uri == impact->uri;
            }
            return false;
        };
        for (auto& mvj: pt_data.meta_vjs) {
            if (std::any_of(std::begin(mvj->impacted_by), std::end(mvj->impacted_by), find_impact)) {
                (*this)(mvj.get());
            };
        }
    }

    void operator()(nt::StopArea* stop_area) {
        stop_area->remove_impact(impact);
        for (auto* sp: stop_area->stop_point_list) {
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

    void operator()(nt::disruption::LineSection&) {
        auto find_impact = [&](const boost::weak_ptr<nt::disruption::Impact>& weak_ptr) {
            if (auto i = weak_ptr.lock()){
                return i->uri == impact->uri;
            }
            return false;
        };
        for (auto& mvj: pt_data.meta_vjs) {
            if (std::any_of(std::begin(mvj->impacted_by), std::end(mvj->impacted_by), find_impact)) {
                (*this)(mvj.get());
            };
        }
    }
};

void delete_impact(boost::shared_ptr<nt::disruption::Impact> impact,
                          nt::PT_Data& pt_data, const nt::MetaData& meta) {
    if (! is_modifying_effect(impact->severity->effect)) {
        return;
    }
    auto log = log4cplus::Logger::getInstance("log");
    LOG4CPLUS_DEBUG(log, "Deleting impact: " << impact.get()->uri);
    delete_impacts_visitor v(impact, pt_data, meta, impact->disruption->rt_level);
    boost::for_each(impact->informed_entities, boost::apply_visitor(v));
    LOG4CPLUS_DEBUG(log, impact.get()->uri << " deleted");
}

} // anonymous namespace

void delete_disruption(const std::string& disruption_id,
                       nt::PT_Data& pt_data,
                       const nt::MetaData& meta) {
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

void apply_disruption(const type::disruption::Disruption& disruption, nt::PT_Data& pt_data,
                    const navitia::type::MetaData &meta) {
    LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("log"), "applying disruption: " << disruption.uri);
    for (const auto& impact: disruption.get_impacts()) {
        apply_impact(impact, pt_data, meta);
    }
}

} // namespace navitia
