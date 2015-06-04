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

#include "fill_disruption_from_chaos.h"
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

static boost::shared_ptr<nt::new_disruption::Tag>
make_tag(const chaos::Tag& chaos_tag, nt::new_disruption::DisruptionHolder& holder) {
    auto from_posix = navitia::from_posix_timestamp;

    auto& weak_tag = holder.tags[chaos_tag.id()];
    if (auto tag = weak_tag.lock()) { return std::move(tag); }

    auto tag = boost::make_shared<nt::new_disruption::Tag>();
    tag->uri = chaos_tag.id();
    tag->name = chaos_tag.name();
    tag->created_at = from_posix(chaos_tag.created_at());
    tag->updated_at = from_posix(chaos_tag.updated_at());

    weak_tag = tag;
    return std::move(tag);
}

static boost::shared_ptr<nt::new_disruption::Cause>
make_cause(const chaos::Cause& chaos_cause, nt::new_disruption::DisruptionHolder& holder) {
    auto from_posix = navitia::from_posix_timestamp;

    auto& weak_cause = holder.causes[chaos_cause.id()];
    if (auto cause = weak_cause.lock()) { return std::move(cause); }

    auto cause = boost::make_shared<nt::new_disruption::Cause>();
    cause->uri = chaos_cause.id();
    cause->wording = chaos_cause.wording();
    cause->created_at = from_posix(chaos_cause.created_at());
    cause->updated_at = from_posix(chaos_cause.updated_at());

    weak_cause = cause;
    return std::move(cause);

}


//return the time period of circulation of a vj for one day
static boost::posix_time::time_period execution_period(const boost::gregorian::date& date,
                                                          const nt::VehicleJourney& vj){
    uint32_t first_departure = std::numeric_limits<uint32_t>::max();
    uint32_t last_arrival = 0;
    for(const auto& st: vj.stop_time_list){
        if(st.pick_up_allowed() && first_departure > st.departure_time){
            first_departure = st.departure_time;
        }
        if(st.drop_off_allowed() && last_arrival < st.arrival_time){
            last_arrival = st.arrival_time;
        }
    }
    return bt::time_period(bt::ptime(date, bt::seconds(first_departure)),
            bt::ptime(date, bt::seconds(last_arrival)));
}

static boost::shared_ptr<nt::new_disruption::Severity>
make_severity(const chaos::Severity& chaos_severity, nt::new_disruption::DisruptionHolder& holder) {
    namespace tr = transit_realtime;
    namespace new_disr = nt::new_disruption;
    auto from_posix = navitia::from_posix_timestamp;

    auto& weak_severity = holder.severities[chaos_severity.id()];
    if (auto severity = weak_severity.lock()) { return std::move(severity); }

    auto severity = boost::make_shared<new_disr::Severity>();
    severity->uri = chaos_severity.id();
    severity->wording = chaos_severity.wording();
    severity->created_at = from_posix(chaos_severity.created_at());
    severity->updated_at = from_posix(chaos_severity.updated_at());
    severity->color = chaos_severity.color();
    severity->priority = chaos_severity.priority();
    switch (chaos_severity.effect()) {
#define EFFECT_ENUM_CONVERSION(e) \
        case tr::Alert_Effect_##e: severity->effect = new_disr::Effect::e; break

        EFFECT_ENUM_CONVERSION(NO_SERVICE);
        EFFECT_ENUM_CONVERSION(REDUCED_SERVICE);
        EFFECT_ENUM_CONVERSION(SIGNIFICANT_DELAYS);
        EFFECT_ENUM_CONVERSION(DETOUR);
        EFFECT_ENUM_CONVERSION(ADDITIONAL_SERVICE);
        EFFECT_ENUM_CONVERSION(MODIFIED_SERVICE);
        EFFECT_ENUM_CONVERSION(OTHER_EFFECT);
        EFFECT_ENUM_CONVERSION(UNKNOWN_EFFECT);
        EFFECT_ENUM_CONVERSION(STOP_MOVED);

#undef EFFECT_ENUM_CONVERSION
    }

    return std::move(severity);
}

static boost::optional<nt::new_disruption::LineSection>
make_line_section(const chaos::PtObject& chaos_section,
                  nt::PT_Data& pt_data,
                  const boost::shared_ptr<nt::new_disruption::Impact>& impact) {
    if (!chaos_section.has_pt_line_section()) {
        LOG4CPLUS_WARN(log4cplus::Logger::getInstance("log"),
                       "fill_disruption_from_chaos: LineSection invalid!");
        return boost::none;
    }
    const auto& pb_section = chaos_section.pt_line_section();
    nt::new_disruption::LineSection line_section;
    auto* line = find_or_default(pb_section.line().uri(), pt_data.lines_map);
    if (line) {
        line_section.line = line;
    } else {
        LOG4CPLUS_WARN(log4cplus::Logger::getInstance("log"),
                       "fill_disruption_from_chaos: line id "
                       << pb_section.line().uri() << " in LineSection invalid!");
        return boost::none;
    }
    if (const auto* start = find_or_default(pb_section.start_point().uri(), pt_data.stop_areas_map)) {
        line_section.start_point = start;
    } else {
        LOG4CPLUS_WARN(log4cplus::Logger::getInstance("log"),
                       "fill_disruption_from_chaos: start_point id "
                       << pb_section.start_point().uri() << " in LineSection invalid!");
        return boost::none;
    }
    if (const auto* end = find_or_default(pb_section.end_point().uri(), pt_data.stop_areas_map)) {
        line_section.end_point = end;
    } else {
        LOG4CPLUS_WARN(log4cplus::Logger::getInstance("log"),
                       "fill_disruption_from_chaos: end_point id "
                       << pb_section.end_point().uri() << " in LineSection invalid!");
        return boost::none;
    }
    if (impact) line->add_impact(impact);
    return line_section;
}
static std::vector<nt::new_disruption::PtObj>
make_pt_objects(const google::protobuf::RepeatedPtrField<chaos::PtObject>& chaos_pt_objects,
                nt::PT_Data& pt_data,
                const boost::shared_ptr<nt::new_disruption::Impact>& impact = {}) {
    using namespace nt::new_disruption;

    std::vector<PtObj> res;
    for (const auto& chaos_pt_object: chaos_pt_objects) {
        switch (chaos_pt_object.pt_object_type()) {
        case chaos::PtObject_Type_network:
            res.push_back(make_pt_obj(nt::Type_e::Network, chaos_pt_object.uri(), pt_data, impact));
            break;
        case chaos::PtObject_Type_stop_area:
            res.push_back(make_pt_obj(nt::Type_e::StopArea, chaos_pt_object.uri(), pt_data, impact));
            break;
        case chaos::PtObject_Type_line_section:
            if (auto line_section = make_line_section(chaos_pt_object, pt_data, impact)) {
                res.push_back(*line_section);
            }
            break;
        case chaos::PtObject_Type_line:
            res.push_back(make_pt_obj(nt::Type_e::Line, chaos_pt_object.uri(), pt_data, impact));
            break;
        case chaos::PtObject_Type_route:
            res.push_back(make_pt_obj(nt::Type_e::Route, chaos_pt_object.uri(), pt_data, impact));
            break;
        case chaos::PtObject_Type_unkown_type:
            res.push_back(UnknownPtObj());
            break;
        }
        // no created_at and updated_at?
    }
    return res;
}

static boost::shared_ptr<nt::new_disruption::Impact>
make_impact(const chaos::Impact& chaos_impact, nt::PT_Data& pt_data) {
    auto from_posix = navitia::from_posix_timestamp;
    nt::new_disruption::DisruptionHolder& holder = pt_data.disruption_holder;

    auto impact = boost::make_shared<nt::new_disruption::Impact>();
    impact->uri = chaos_impact.id();
    impact->created_at = from_posix(chaos_impact.created_at());
    impact->updated_at = from_posix(chaos_impact.updated_at());
    for (const auto& chaos_ap: chaos_impact.application_periods()) {
        impact->application_periods.emplace_back(from_posix(chaos_ap.start()), from_posix(chaos_ap.end()));
    }
    impact->severity = make_severity(chaos_impact.severity(), holder);
    impact->informed_entities = make_pt_objects(chaos_impact.informed_entities(), pt_data, impact);
    for (const auto& chaos_message: chaos_impact.messages()) {
        const auto& channel = chaos_message.channel();
        impact->messages.push_back({
            chaos_message.text(),
            channel.id(),
            channel.name(),
            channel.content_type(),
            from_posix(chaos_message.created_at()),
            from_posix(chaos_message.updated_at()),
        });
    }

    return std::move(impact);
}



struct apply_impacts_visitor : public boost::static_visitor<> {
    boost::shared_ptr<nt::new_disruption::Impact> impact;
    nt::PT_Data& pt_data;
    const nt::MetaData& meta;
    std::string action;

    apply_impacts_visitor(const boost::shared_ptr<nt::new_disruption::Impact>& impact,
            nt::PT_Data& pt_data, const nt::MetaData& meta, std::string action) :
        impact(impact), pt_data(pt_data), meta(meta), action(action) {}

    virtual bool func_on_vj(nt::VehicleJourney&) = 0;

    void log_start_action(std::string uri) {
        LOG4CPLUS_TRACE(log4cplus::Logger::getInstance("log"),
                        "Start to " << action << " impact " << impact.get()->uri << " on object " << uri);
    }

    void log_end_action(std::string uri) {
        LOG4CPLUS_TRACE(log4cplus::Logger::getInstance("log"),
                        "Finished to " << action << " impact " << impact.get()->uri << " on object " << uri);
    }

    void operator()(nt::new_disruption::UnknownPtObj&) {
    }

    void operator()(const nt::Network* network) {
        this->log_start_action(network->uri);
        for(auto line : network->line_list) {
            this->operator()(line);
        }
        this->log_end_action(network->uri);
    }

    void operator()(nt::new_disruption::LineSection & ls) {
        std::string uri = "line section (" +  ls.line->uri  + ")";
        this->log_start_action(uri);
        this->operator()(ls.line);
        this->log_end_action(uri);
    }

    void operator()(const nt::Line* line) {
        this->log_start_action(line->uri);
        for(auto route : line->route_list) {
            this->operator()(route);
        }
        this->log_end_action(line->uri);
    }

    void operator()(const nt::Route* route) {
        this->log_start_action(route->uri);
        for (auto journey_pattern : route->journey_pattern_list) {
            (*this)(journey_pattern);
        }
        this->log_end_action(route->uri);
    }

    void operator()(const nt::JourneyPattern* journey_pattern) {
        this->log_start_action(journey_pattern->uri);
        journey_pattern->for_each_vehicle_journey([&](nt::VehicleJourney& vj) {
                return func_on_vj(vj);
        });
        this->log_end_action(journey_pattern->uri);
    }

};

struct functor_add_vj {
    const boost::shared_ptr<nt::new_disruption::Impact>& impact;
    nt::JourneyPattern* jp;
    const nt::StopPoint* stop_point;
    nt::PT_Data& pt_data;
    const nt::MetaData& meta;

    functor_add_vj(const boost::shared_ptr<nt::new_disruption::Impact> impact,
            nt::JourneyPattern* jp, const nt::StopPoint* stop_point,
            nt::PT_Data& pt_data, const nt::MetaData& meta) :
        impact(impact), jp(jp), stop_point(stop_point), pt_data(pt_data), meta(meta) {}

    //return true only if the new vj circulate at least one day
    bool init_vj(nt::VehicleJourney* vj, nt::VehicleJourney& vj_ref) const {
        vj->journey_pattern = jp;
        vj->idx = pt_data.vehicle_journeys.size();
        vj->uri = make_adapted_uri_fast(vj_ref.uri, pt_data.vehicle_journeys.size());
        vj->is_adapted = true;
        vj->name = vj_ref.name;
        vj->company = vj_ref.company;
        //TODO: we loose stay_in on impacted vj, we will need to work on this.
        vj->next_vj = nullptr;
        vj->prev_vj = nullptr;
        vj->meta_vj = vj_ref.meta_vj;
        vj->utc_to_local_offset = vj_ref.utc_to_local_offset;
        // The validity_pattern is only active on the period of the impact
        nt::ValidityPattern tmp_vp;
        nt::ValidityPattern tmp_ref_vp(*vj_ref.adapted_validity_pattern);
        for (auto period : impact->application_periods) {
            //we can impact a vj with a departure the day before who past midnight
            bg::day_iterator titr(period.begin().date() - bg::days(1));
            for(;titr<=period.end().date(); ++titr) {
                if (!meta.production_date.contains(*titr)) {
                    continue;
                }
                const auto day = (*titr - meta.production_date.begin()).days();
                //if the ref is active this day, we active the new one too
                if(tmp_ref_vp.check(day) && execution_period(*titr, vj_ref).intersects(period)){
                    tmp_vp.add(day);
                    tmp_ref_vp.remove(day);
                }
            }
        }
        if(tmp_vp.days.none()){
            //this vehicle journey won't ever circulate
            return false;
        }
        vj->adapted_validity_pattern = pt_data.get_or_create_validity_pattern(tmp_vp);
        vj_ref.adapted_validity_pattern = pt_data.get_or_create_validity_pattern(tmp_ref_vp);

        pt_data.vehicle_journeys.push_back(vj);
        pt_data.vehicle_journeys_map[vj->uri] = vj;

        // The vehicle_journey is never active on theorical validity_pattern
        tmp_vp.reset();
        vj->validity_pattern = pt_data.get_or_create_validity_pattern(tmp_vp);
        size_t order = 0;
        // We skip the stop_time linked to impacted stop_point
        for (const auto& st_ref : vj_ref.stop_time_list) {
            if (st_ref.journey_pattern_point->stop_point == stop_point) {
                continue;
            }
            vj->stop_time_list.emplace_back(st_ref);
            auto& st = vj->stop_time_list.back();
            // We link the journey_pattern_point to the stop_time
            st.journey_pattern_point = jp->journey_pattern_point_list[order];
            st.vehicle_journey = vj;
            ++ order;
        }

        // we need to copy the vj's comments
        for (const auto& c: pt_data.comments.get(vj_ref)) {
            pt_data.comments.add(vj, c);
        }
        // We need to link the newly created vj with this impact
        vj->impacted_by.push_back(impact);
        return true;
    }

    bool operator()(nt::DiscreteVehicleJourney& vj_ref) const {
        auto vj = std::make_unique<nt::DiscreteVehicleJourney>();
        if(init_vj(vj.get(), vj_ref)){
            jp->discrete_vehicle_journey_list.push_back(std::move(vj));
        }
        return true;
    }

    bool operator()(nt::FrequencyVehicleJourney& vj_ref) const {
        auto vj = std::make_unique<nt::FrequencyVehicleJourney>();
        vj->start_time = vj_ref.start_time;
        vj->end_time = vj_ref.end_time;
        vj->headway_secs = vj_ref.headway_secs;
        if(init_vj(vj.get(), vj_ref)){
            jp->frequency_vehicle_journey_list.push_back(std::move(vj));
        }
        return true;
    }
};


struct add_impacts_visitor : public apply_impacts_visitor {
    add_impacts_visitor(const boost::shared_ptr<nt::new_disruption::Impact>& impact,
            nt::PT_Data& pt_data, const nt::MetaData& meta) :
        apply_impacts_visitor(impact, pt_data, meta, "add") {}

    using apply_impacts_visitor::operator();

    bool func_on_vj(nt::VehicleJourney& vj) {
        nt::ValidityPattern tmp_vp(*vj.adapted_validity_pattern);
        for (auto period : impact->application_periods) {
            //we can impact a vj with a departure the day before who past midnight
            bg::day_iterator titr(period.begin().date() - bg::days(1));
            for(;titr<=period.end().date(); ++titr) {
                if (!meta.production_date.contains(*titr)) {
                    continue;
                }
                auto day = (*titr - meta.production_date.begin()).days();
                if (!tmp_vp.check(day)){
                    continue;
                }
                if(period.intersects(execution_period(*titr, vj))) {
                    tmp_vp.remove(day);
                }
            }
        }
        vj.adapted_validity_pattern = pt_data.get_or_create_validity_pattern(tmp_vp);
        vj.impacted_by.push_back(impact);
        return true;
    }

    void operator()(const nt::StopPoint* stop_point) {
        this->log_start_action(stop_point->uri);
        if (stop_point->journey_pattern_point_list.empty()) {
            return;
        }
        // We copy this journey_pattern
        for (const auto jpp_ref : stop_point->journey_pattern_point_list) {
            const auto jp_ref = jpp_ref->journey_pattern;
            auto new_jp = new nt::JourneyPattern(*jp_ref);
            new_jp->uri = make_adapted_uri_fast(new_jp->uri, pt_data.journey_patterns.size());
            new_jp->idx = pt_data.journey_patterns.size();
            pt_data.journey_patterns.push_back(new_jp);
            pt_data.journey_patterns_map[new_jp->uri] = new_jp;
            new_jp->journey_pattern_point_list.clear();
            new_jp->discrete_vehicle_journey_list.clear();
            new_jp->frequency_vehicle_journey_list.clear();
            // We copy each journey_pattern_point but the ones which are linked
            // to the impacted stop_area
            size_t order = 0;
            for (const auto jpp_to_copy : jp_ref->journey_pattern_point_list) {
                if (jpp_to_copy->stop_point == stop_point) {
                    continue;
                }
                auto new_jpp = new nt::JourneyPatternPoint(*jpp_to_copy);
                new_jpp->idx = pt_data.journey_pattern_points.size();
                new_jpp->uri = make_adapted_uri_fast(new_jpp->uri, pt_data.journey_pattern_points.size());
                new_jp->journey_pattern_point_list.push_back(new_jpp);
                new_jpp->stop_point->journey_pattern_point_list.push_back(new_jpp);
                pt_data.journey_pattern_points.push_back(new_jpp);
                pt_data.journey_pattern_points_map[new_jpp->uri] = new_jpp;
                new_jpp->journey_pattern = new_jp;
                new_jpp->order = order;
                ++ order;
            }

            // We copy the vehicle_journeys of the journey_pattern_points
            functor_add_vj v(impact, new_jp, stop_point, pt_data, meta);
            jp_ref->for_each_vehicle_journey(v);
            // The new journey_pattern is linked to the impact
            impact->impacted_journey_patterns.push_back(new_jp);
        }
        this->log_end_action(stop_point->uri);
    }

    void operator()(const nt::StopArea* stop_area) {
        this->log_start_action(stop_area->uri);
        for (const auto stop_point : stop_area->stop_point_list) {
            this->operator()(stop_point);
        }
        this->log_end_action(stop_area->uri);
    }
};

void apply_impact(boost::shared_ptr<nt::new_disruption::Impact>impact,
        nt::PT_Data& pt_data, const nt::MetaData& meta) {
    if (impact->severity->effect != nt::new_disruption::Effect::NO_SERVICE) {
        return;
    }
    LOG4CPLUS_TRACE(log4cplus::Logger::getInstance("log"),
                    "Adding impact: " << impact.get()->uri);
    add_impacts_visitor v(impact, pt_data, meta);
    boost::for_each(impact->informed_entities, boost::apply_visitor(v));
    LOG4CPLUS_TRACE(log4cplus::Logger::getInstance("log"),
                    impact.get()->uri << " impact added");
}


struct delete_impacts_visitor : public apply_impacts_visitor {
    delete_impacts_visitor(boost::shared_ptr<nt::new_disruption::Impact> impact,
            nt::PT_Data& pt_data, const nt::MetaData& meta) :
        apply_impacts_visitor(impact, pt_data, meta, "delete") {}
    size_t nb_vj_reassigned = 0;
    using apply_impacts_visitor::operator();

    // We set all the validity pattern to the theorical one, we will re-apply
    // other disruptions after
    bool func_on_vj(nt::VehicleJourney& vj) {
        vj.adapted_validity_pattern = vj.validity_pattern;
        ++ nb_vj_reassigned;
        const auto& impact = this->impact;
        boost::range::remove_erase_if(vj.impacted_by,
            [&impact](const boost::weak_ptr<nt::new_disruption::Impact>& i) {
                auto spt = i.lock();
                return (spt) ? spt == impact : true;
        });
        for (auto i: vj.impacted_by) {
            if (auto spt = i.lock()) {
                auto v = add_impacts_visitor(spt, pt_data, meta);
                v.func_on_vj(vj);
            }
        }
        return true;
    }

    void operator()(const nt::StopArea* stop_area) {
        this->log_start_action(stop_area->uri);
        // We need to erase vehicle journeys from the collections and the map
        // We do not remove the link journey_pattern->vj, because we will
        // remove the journey_pattern_after

        std::set<nt::idx_t> vehicle_journeys_to_erase;
        std::set<nt::idx_t> jpp_to_erase;
        LOG4CPLUS_TRACE(log4cplus::Logger::getInstance("log"),
                        "Removing jpp from stop_points");
        std::vector<const nt::JourneyPattern*> jp_to_delete = impact->impacted_journey_patterns;
        for (auto journey_pattern : jp_to_delete) {
            journey_pattern->for_each_vehicle_journey(
                        [&vehicle_journeys_to_erase](const type::VehicleJourney& vj) {
                vehicle_journeys_to_erase.insert(vj.idx);
                return true;
            });

            // We need to remove the journey_pattern_point from its stop_point
            // because we will remove the journey_pattern_point after.
            for (auto journey_pattern_point : journey_pattern->journey_pattern_point_list) {
                journey_pattern_point->stop_point->journey_pattern_point_list.erase(
                            boost::remove_if(journey_pattern_point->stop_point->journey_pattern_point_list,
                                [&](nt::JourneyPatternPoint* jpp) {return journey_pattern_point->uri == jpp->uri;}),
                        journey_pattern_point->stop_point->journey_pattern_point_list.end());
                jpp_to_erase.insert(journey_pattern_point->idx);
            }
        }
        //we delete all impacted journey pattern, so there is no more impated journey pattern,
        //if we don't do that we will try to delete them multiple times
        impact->impacted_journey_patterns.clear();

        LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("log"),
                        "Removing vehicle_journeys");
        for(auto it = vehicle_journeys_to_erase.rbegin(); it != vehicle_journeys_to_erase.rend(); ++it) {
            pt_data.remove_from_collections(*pt_data.vehicle_journeys[*it]);
        }
        pt_data.reindex_vehicle_journeys();

        LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("log"),
                        "Removing journey_pattern_points");
        for(auto it = jpp_to_erase.rbegin(); it != jpp_to_erase.rend(); ++it) {
            pt_data.erase_obj(pt_data.journey_pattern_points[*it]);
        }
        pt_data.reindex_journey_pattern_points();

        LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("log"),
                        "Removing journey_patterns");
        for (auto journey_pattern : jp_to_delete) {
            // Now we can erase journey_patterns from the dataset
            pt_data.erase_obj(journey_pattern);
            pt_data.reindex_journey_patterns();
        }

        LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("log"),
                        "Reassigning validity_pattern");
        for (auto stop_point : stop_area->stop_point_list) {
            for (auto journey_pattern_point : stop_point->journey_pattern_point_list) {
                apply_impacts_visitor::operator ()(journey_pattern_point->journey_pattern);
            }
        }

        LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("log"),
                        nb_vj_reassigned << " validity pattern reassigned");
        this->log_end_action(stop_area->uri);
    }
};

void delete_impact(boost::shared_ptr<nt::new_disruption::Impact>impact,
        nt::PT_Data& pt_data, const nt::MetaData& meta) {
    if (impact->severity->effect != nt::new_disruption::Effect::NO_SERVICE) {
        return;
    }
    LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("log"),
                    "Deleting impact: " << impact.get()->uri);
    delete_impacts_visitor v(impact, pt_data, meta);
    boost::for_each(impact->informed_entities, boost::apply_visitor(v));
    LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("log"),
                    impact.get()->uri << " deleted");
}

void delete_disruption(const std::string& disruption_id,
                       nt::PT_Data& pt_data,
                       const nt::MetaData& meta) {
    LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("log"),
                    "Deleting disruption: " << disruption_id);
    nt::new_disruption::DisruptionHolder &holder = pt_data.disruption_holder;

    auto it = find_if(holder.disruptions.begin(), holder.disruptions.end(),
            [&disruption_id](const std::unique_ptr<nt::new_disruption::Disruption>& disruption){
                return disruption->uri == disruption_id;
            });
    if(it != holder.disruptions.end()) {
        std::vector<nt::new_disruption::PtObj> informed_entities;
        for (const auto& impact : (*it)->get_impacts()) {
            informed_entities.insert(informed_entities.end(),
                              impact->informed_entities.begin(),
                              impact->informed_entities.end());
            delete_impact(impact, pt_data, meta);
        }
        holder.disruptions.erase(it);
    }
    LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("log"),
                    disruption_id << " disruption deleted");
}

void add_disruption(const chaos::Disruption& chaos_disruption, nt::PT_Data& pt_data,
                    const navitia::type::MetaData &meta) {
    LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("log"),
                    "Adding disruption: " << chaos_disruption.id());
    auto from_posix = navitia::from_posix_timestamp;
    nt::new_disruption::DisruptionHolder &holder = pt_data.disruption_holder;

    //we delete the disrupion before adding the new one
    delete_disruption(chaos_disruption.id(), pt_data, meta);

    auto disruption = std::make_unique<nt::new_disruption::Disruption>();
    disruption->uri = chaos_disruption.id();
    disruption->reference = chaos_disruption.reference();
    disruption->publication_period = {
        from_posix(chaos_disruption.publication_period().start()),
        from_posix(chaos_disruption.publication_period().end())
    };
    disruption->created_at = from_posix(chaos_disruption.created_at());
    disruption->updated_at = from_posix(chaos_disruption.updated_at());
    disruption->cause = make_cause(chaos_disruption.cause(), holder);
    for (const auto& chaos_impact: chaos_disruption.impacts()) {
        auto impact = make_impact(chaos_impact, pt_data);
        disruption->add_impact(impact);
        apply_impact(impact, pt_data, meta);
    }
    disruption->localization = make_pt_objects(chaos_disruption.localization(), pt_data);
    for (const auto& chaos_tag: chaos_disruption.tags()) {
        disruption->tags.push_back(make_tag(chaos_tag, holder));
    }
    disruption->note = chaos_disruption.note();

    holder.disruptions.push_back(std::move(disruption));
    LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("log"),
                    chaos_disruption.id() << " disruption added");
}

} // namespace navitia
