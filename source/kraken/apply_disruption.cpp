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
#include "boost/date_time/gregorian/gregorian.hpp"

namespace navitia {

namespace nt = navitia::type;
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

    void operator()(nt::disruption::LineSection& ls) {
        std::string uri = "line section (" +  ls.line->uri  + ")";
        this->log_start_action(uri);
        this->operator()(ls.line);
        this->log_end_action(uri);
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

static type::ValidityPattern compute_vp(const std::vector<boost::posix_time::time_period>& periods,
        const boost::gregorian::date_period& production_period) {
    type::ValidityPattern vp; // bitset are all initialised to 0
    for (const auto& period: periods){
        // we may impact vj's passed midnight
        bg::day_iterator titr(period.begin().date() - bg::days(1));
        for (; titr <= period.end().date(); ++titr) {
            if (! production_period.contains(*titr)) { continue; }
            auto day = (*titr - production_period.begin()).days();
            vp.add(day);
        }
    }
    return vp;
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
            mvj->cancel_vj(rt_level, impact->application_periods, pt_data, meta, r);
            mvj->impacted_by.push_back(impact);
        } else if (impact->severity->effect == nt::disruption::Effect::MODIFIED_SERVICE) {
            LOG4CPLUS_TRACE(log, "modifying " << mvj->uri);
            auto vp = compute_vp(impact->application_periods, meta.production_date);
            mvj->create_discrete_vj(mvj->uri,
                type::RTLevel::RealTime,
                vp,
                r,
                std::move(impact->aux_info.stop_times),
                pt_data);
            mvj->impacted_by.push_back(impact);
        } else {
            LOG4CPLUS_DEBUG(log, "unhandled action on " << mvj->uri);
        }
        log_end_action(mvj->uri);
    }

    void operator()(nt::StopPoint* stop_point) {
        LOG4CPLUS_INFO(log, "Disruption on stop point:" << stop_point->uri << " is not handled");
    }

    void operator()(nt::StopArea* stop_area) {
        LOG4CPLUS_INFO(log, "Disruption on stop area:" << stop_area->uri << " is not handled");
    }
};

static bool is_modifying_effect(nt::disruption::Effect e) {
    // check is the effect needs to modify the model
    return in(e, {nt::disruption::Effect::NO_SERVICE,
                  nt::disruption::Effect::MODIFIED_SERVICE});
}

void apply_impact(boost::shared_ptr<nt::disruption::Impact>impact,
                         nt::PT_Data& pt_data, const nt::MetaData& meta) {
    if (! is_modifying_effect(impact->severity->effect)) {
        return;
    }
    LOG4CPLUS_TRACE(log4cplus::Logger::getInstance("log"),
                    "Adding impact: " << impact.get()->uri);
    add_impacts_visitor v(impact, pt_data, meta, impact->disruption->rt_level);
    boost::for_each(impact->informed_entities, boost::apply_visitor(v));
    LOG4CPLUS_TRACE(log4cplus::Logger::getInstance("log"),
                    impact.get()->uri << " impact added");
}


struct delete_impacts_visitor : public apply_impacts_visitor {
    size_t nb_vj_reassigned = 0;

    delete_impacts_visitor(boost::shared_ptr<nt::disruption::Impact> impact,
            nt::PT_Data& pt_data, const nt::MetaData& meta, nt::RTLevel l) :
        apply_impacts_visitor(impact, pt_data, meta, "delete", l) {}

    ~delete_impacts_visitor() {}

    using apply_impacts_visitor::operator();

    // We set all the validity pattern to the theorical one, we will re-apply
    // other disruptions after
    void operator()(nt::MetaVehicleJourney* mvj, nt::Route* r = nullptr) {
        if (impact->severity->effect == nt::disruption::Effect::NO_SERVICE) {
            for (auto* vj: mvj->get_vjs_in_period(rt_level, impact->application_periods, meta, r)) {
                vj->validity_patterns[rt_level] = vj->validity_patterns[nt::RTLevel::Base];
                ++ nb_vj_reassigned;
            }
        } else {
            LOG4CPLUS_DEBUG(log, "unhandled delete action on " << mvj->uri);
        }
        const auto& impact = this->impact;
        boost::range::remove_erase_if(mvj->impacted_by,
            [&impact](const boost::weak_ptr<nt::disruption::Impact>& i) {
                auto spt = i.lock();
                return (spt) ? spt == impact : true;
        });

        for (auto i: mvj->impacted_by) {
            if (auto spt = i.lock()) {
                auto v = add_impacts_visitor(spt, pt_data, meta, rt_level);
                v(mvj);
            }
        }
    }

    void operator()(const nt::StopPoint* stop_point) {
        LOG4CPLUS_INFO(log, "Deletion of disruption on stop point:" << stop_point->uri << " is not handled");

    }

    void operator()(const nt::StopArea* stop_area) {
        LOG4CPLUS_INFO(log, "Deletion of disruption on stop point:" << stop_area->uri << " is not handled");
    }
};

void delete_impact(boost::shared_ptr<nt::disruption::Impact>impact,
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
    std::unique_ptr<nt::disruption::Disruption> disruption = holder.pop_disruption(disruption_id);
    if (disruption) {
        std::vector<nt::disruption::PtObj> informed_entities;
        for (const auto& impact : disruption->get_impacts()) {
            informed_entities.insert(informed_entities.end(),
                              impact->informed_entities.begin(),
                              impact->informed_entities.end());
            delete_impact(impact, pt_data, meta);
        }
    }
    LOG4CPLUS_DEBUG(log, disruption_id << " disruption deleted");
}

void apply_disruption(const type::disruption::Disruption& disruption, nt::PT_Data& pt_data,
                    const navitia::type::MetaData &meta) {
    LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("log"), "applying disruption: " << disruption.uri);
    for (const auto& impact: disruption.get_impacts()) {
        apply_impact(impact, pt_data, meta);
    }
}

} // namespace navitia
