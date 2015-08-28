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
#include "utils/functions.h"
#include "type/datetime.h"
#include <algorithm>
#include <boost/make_shared.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/range/algorithm/for_each.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

namespace navitia {

namespace nt = navitia::type;
namespace bt = boost::posix_time;
namespace bg = boost::gregorian;

namespace {

template<typename T>
std::vector<std::unique_ptr<T>>& get_vj_list_of_jp_helper(nt::JourneyPattern* jp);

template<>
auto get_vj_list_of_jp_helper<nt::DiscreteVehicleJourney>(nt::JourneyPattern* jp)
-> decltype((jp->discrete_vehicle_journey_list))
{ return jp->discrete_vehicle_journey_list; }

template<>
auto get_vj_list_of_jp_helper<nt::FrequencyVehicleJourney>(nt::JourneyPattern* jp)
-> decltype((jp->frequency_vehicle_journey_list))
{ return jp->frequency_vehicle_journey_list; }

template<typename Container, typename Value>
static void push_back_unique(Container& c, const Value& v) {
    if (! contains(c, v)) { c.push_back(v); }
}

}

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
        case chaos::PtObject_Type_stop_point:
            res.push_back(make_pt_obj(nt::Type_e::StopPoint, chaos_pt_object.uri(), pt_data, impact));
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


namespace {

struct apply_impacts_visitor : public boost::static_visitor<> {
    boost::shared_ptr<nt::new_disruption::Impact> impact;
    nt::PT_Data& pt_data;
    const nt::MetaData& meta;
    std::string action;

    apply_impacts_visitor(const boost::shared_ptr<nt::new_disruption::Impact>& impact,
            nt::PT_Data& pt_data, const nt::MetaData& meta, std::string action) :
        impact(impact), pt_data(pt_data), meta(meta), action(action) {}

    virtual ~apply_impacts_visitor() {}
    apply_impacts_visitor(const apply_impacts_visitor&) = default;

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

    void operator()(const nt::new_disruption::LineSection& ls) {
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

struct functor_copy_vj_to_jp {
    const boost::shared_ptr<nt::new_disruption::Impact>& impact;
    nt::JourneyPattern* jp;
    const std::set<const nt::StopPoint*>& impacted_stop_points;
    nt::PT_Data& pt_data;
    const nt::MetaData& meta;

    //return true only if the new vj circulate at least one day
    bool init_vj(nt::VehicleJourney& adapted_vj, nt::VehicleJourney& vj_ref) const {

        adapted_vj.journey_pattern = jp;
        adapted_vj.idx = pt_data.vehicle_journeys.size();
        adapted_vj.uri =  make_adapted_uri_fast(vj_ref.uri, pt_data.vehicle_journeys.size());
        adapted_vj.is_adapted = true;
        pt_data.headsign_handler.change_name_and_register_as_headsign(*vj, vj_ref.name);
        adapted_vj.name = vj_ref.name;
        adapted_vj.company = vj_ref.company;
        //TODO: we loose stay_in on impacted vj, we will need to work on this.
        adapted_vj.next_vj = nullptr;
        adapted_vj.prev_vj = nullptr;
        adapted_vj.meta_vj = vj_ref.meta_vj;
        adapted_vj.theoric_vehicle_journey = &vj_ref;
        adapted_vj.utc_to_local_offset = vj_ref.utc_to_local_offset;
        // The validity_pattern is only active on the period of the impact
        nt::ValidityPattern tmp_vp{};
        nt::ValidityPattern tmp_ref_vp{*vj_ref.adapted_validity_pattern};
        for (const auto& period : impact->application_periods) {
            //we can impact a vj with a departure the day before who past midnight
            auto time_it = bg::day_iterator{period.begin().date() - bg::days{1}};
            for(;time_it<=period.end().date(); ++time_it) {
                if (!meta.production_date.contains(*time_it)) {
                    continue;
                }
                const auto day = (*time_it - meta.production_date.begin()).days();
                //if the ref is active this day, we active the new one too
                if(tmp_ref_vp.check(day) && execution_period(*time_it, vj_ref).intersects(period)){
                    tmp_vp.add(day);
                    tmp_ref_vp.remove(day);
                }
            }
        }
        if(tmp_vp.days.none()){
            //this vehicle journey won't ever circulate
            return false;
        }
        auto meta_vj_it = pt_data.meta_vj.find(vj_ref.uri);
        if (meta_vj_it != pt_data.meta_vj.cend()) {
            pt_data.meta_vj[vj_ref.uri]->adapted_vj.push_back(&adapted_vj);
        }
        adapted_vj.adapted_validity_pattern = pt_data.get_or_create_validity_pattern(tmp_vp);
        vj_ref.adapted_validity_pattern = pt_data.get_or_create_validity_pattern(tmp_ref_vp);

        pt_data.vehicle_journeys.push_back(&adapted_vj);
        pt_data.vehicle_journeys_map[adapted_vj.uri] = &adapted_vj;

        // The vehicle_journey is never active on theorical validity_pattern
        tmp_vp.reset();
        adapted_vj.validity_pattern = pt_data.get_or_create_validity_pattern(tmp_vp);
        size_t order = 0;
        // We skip the stop_time linked to impacted stop_point
        for (const auto& st_ref : vj_ref.stop_time_list) {
            if (contains(impacted_stop_points, st_ref.journey_pattern_point->stop_point)) {
                continue;
            }
            adapted_vj.stop_time_list.push_back(st_ref);
            auto& st = adapted_vj.stop_time_list.back();
            // We link the journey_pattern_point to the stop_time
            st.journey_pattern_point = jp->journey_pattern_point_list[order];
            st.vehicle_journey = &adapted_vj;
            ++ order;
        }

        // we need to copy the vj's comments
        for (const auto& c: pt_data.comments.get(vj_ref)) {
            pt_data.comments.add(adapted_vj, c);
        }
        // We need to link the newly created vj with this impact
        adapted_vj.impacted_by.push_back(impact);
        return true;
    }

    bool operator()(nt::DiscreteVehicleJourney& vj_ref) const {
        auto vj = std::make_unique<nt::DiscreteVehicleJourney>();
        if(init_vj(*vj, vj_ref)){
            jp->discrete_vehicle_journey_list.push_back(std::move(vj));
        }
        return true;
    }

    bool operator()(nt::FrequencyVehicleJourney& vj_ref) const {
        auto vj = std::make_unique<nt::FrequencyVehicleJourney>();
        vj->start_time = vj_ref.start_time;
        vj->end_time = vj_ref.end_time;
        vj->headway_secs = vj_ref.headway_secs;
        if(init_vj(*vj, vj_ref)){
            jp->frequency_vehicle_journey_list.push_back(std::move(vj));
        }
        return true;
    }
};

struct vehicle_journey_impactor {
  vehicle_journey_impactor(nt::PT_Data& pt_data,
          const std::set<const nt::StopPoint*>& impacted_stop_points,
          nt::JourneyPattern* jp,
          const boost::shared_ptr<nt::new_disruption::Impact>& impact,
          const nt::MetaData& meta):
              pt_data{pt_data},
              impacted_stop_points{impacted_stop_points},
              jp{jp},
              impact{impact},
              meta{meta}{}

  void register_vj_for_update(nt::DiscreteVehicleJourney* vj, nt::JourneyPattern* jp) {
      vj_discrete_to_be_updated.push_back({vj, jp});
  }
  void register_vj_for_update(nt::FrequencyVehicleJourney* vj, nt::JourneyPattern* jp) {
      vj_frequency_to_be_updated.push_back({vj, jp});
  }

  // update the adapated vj with the new journey pattern
  // - update stop times
  // - detach from its old jp
  // - assign the new jp to the adapted vj
  // - remove old jp from impact
  // - add the adapted into new jp
  template<typename VJ_T>
  void complete_impl(const std::vector<std::pair<VJ_T*, nt::JourneyPattern*>>& vj_to_be_updated ) {

      for (auto& vj_jp: vj_to_be_updated) {
          auto& vj_ptr = vj_jp.first;
          auto* new_jp = vj_jp.second;
          auto* old_jp = vj_ptr->journey_pattern;

          // We reconstruct a new stop time list whose stop points are not contained in the impacted
          // stop points, then we move it to vj's stop time list
          std::vector<nt::StopTime> stop_times;
          for (auto& stop_time: vj_ptr->stop_time_list) {
              const auto stop_point = stop_time.journey_pattern_point->stop_point;
              auto it = boost::find_if(new_jp->journey_pattern_point_list,
                      [stop_point](const nt::JourneyPatternPoint* jpp){ return jpp->stop_point == stop_point; });
              // the stop_point of stop time does not belong to the jpp of the new_jp, we don't keep it.
              if (it == new_jp->journey_pattern_point_list.cend()) {
                  continue;
              }
              stop_time.journey_pattern_point = *it;
              stop_times.push_back(stop_time);
          }
          // calling the move constructor
          vj_ptr->stop_time_list = std::move(stop_times);

          auto& vj_list = get_vj_list_of_jp_helper<VJ_T>(old_jp);

          auto it_old_vj = std::find_if(std::begin(vj_list), std::end(vj_list),
                  [&vj_ptr](const std::unique_ptr<VJ_T>& old_vj ){
                  return old_vj->uri == vj_ptr->uri;
          });

          assert(it_old_vj!= std::end(vj_list));

          // Detach the vj from its jp and give it's ownership to a temporary unique_ptr
          std::unique_ptr<VJ_T> old_vj_unique_ptr{it_old_vj->release()};
          vj_list.erase(it_old_vj);

          // Assign the new_jp to the vj
          vj_ptr->journey_pattern = new_jp;

          // We should also remove the old jp form impact's impacted_journey_patterns
          boost::remove_erase_if(impact->impacted_journey_patterns, [&](const nt::JourneyPattern* jp){
              return jp->uri == old_jp->uri;
          });

          // Now we can attach the vj to the new jp
          get_vj_list_of_jp_helper<VJ_T>(new_jp).push_back(std::move(old_vj_unique_ptr));

      }
  }

  // update modified vj with its new jp to attach
  void complete() {
      complete_impl(vj_discrete_to_be_updated);
      complete_impl(vj_frequency_to_be_updated);
  };

  bool is_impacted(const nt::VehicleJourney& vj) const{
      for (auto period : impact->application_periods) {
          bg::day_iterator time_itr{period.begin().date() - bg::days{1}};
          for(;time_itr<=period.end().date(); ++time_itr) {
              if (!meta.production_date.contains(*time_itr)) {
                  continue;
              }
              auto day = (*time_itr - meta.production_date.begin()).days();

              if (!vj.is_adapted) {
                  if (vj.validity_pattern->check(day)) {
                      return true;
                  }
              }else{
                  // TODO: We should handle the intersected disruption periods
                  if (vj.adapted_validity_pattern->check(day)) {
                      return true;
                  }
              }

          }
      }
      return false;
  }

  nt::JourneyPattern* get_new_jp_without_impacted_stop_points(const nt::JourneyPattern& jp,
          const std::set<const nt::StopPoint*>& impacted_stop_points) {
      std::vector<nt::StopPoint*> stop_points_for_key{};
      for (const auto* jpp: jp.journey_pattern_point_list) {
          if (! contains(impacted_stop_points, jpp->stop_point)) {
              stop_points_for_key.push_back(jpp->stop_point);
          }
      }
      auto jp_key = nt::JourneyPatternKey{jp, std::move(stop_points_for_key)};
      return pt_data.get_or_create_journey_pattern(jp_key);
  }

  template<typename T>
  bool is_updated_with_impacted_stop_points(const T& vj){
      const auto& jp_points = vj->journey_pattern->journey_pattern_point_list;
      return  std::none_of(std::begin(jp_points), std::end(jp_points), [&](const nt::JourneyPatternPoint* jpp){
          return contains(impacted_stop_points, jpp->stop_point);});
  }

  template<typename VJ>
  bool operator()(std::unique_ptr<VJ>& vj){
      // If vj circulates in impact's period
      if (! is_impacted(*vj)) {
          return true;
      };
      if(vj->is_adapted) {

          // Here we test if there is any vj's journey pattern points that are in impacted_stop_points
          // If vj doesn't have any journey pattern points in impacted_stop_points, there is nothing to do
          if (is_updated_with_impacted_stop_points(vj)) {
              return true;
          }
          // Here, we know that this vj is actually impacted by the impact, because one or more of its jpp
          // are in the list of impacted stop points, we should re-compute a new journey
          // pattern for this vj, then attach this vj to the new jp later
          auto new_jp = get_new_jp_without_impacted_stop_points(*(vj->journey_pattern), impacted_stop_points);

          register_vj_for_update(vj.get(), new_jp);

          // The new journey_pattern is linked to the impact
          push_back_unique(impact->impacted_journey_patterns, new_jp);

      }else{

          for (auto* adapted_vj: vj->meta_vj->adapted_vj) {

              //check the date if adapted_vj is inside impact periode
              if (is_impacted(*adapted_vj)) {
                  // we check if there are any of journey pattern points of this adapted vj that belongs to
                  // impacted stop points list
                  if (is_updated_with_impacted_stop_points(adapted_vj)) {
                      // if this adpated_vj has no JPP in this stop point list, it means that its theoretical
                      // vj is also impacted by this impact.
                      push_back_unique(impact->impacted_journey_patterns, adapted_vj->journey_pattern);

                      const bool has_no_impact = std::none_of(std::begin(adapted_vj->impacted_by),
                              std::end(adapted_vj->impacted_by),
                              [&](boost::weak_ptr<navitia::type::new_disruption::Impact>& weak_ptr ){
                          if(auto ptr = weak_ptr.lock()) {
                              return ptr.get() == impact.get();
                          }
                          return false;
                      });
                      if (has_no_impact) {
                          adapted_vj->impacted_by.push_back(impact);
                      }
                  }
                  // we find an impacted vj for the vj, we don't need to bother with it, it will be updated later
                  return true;
              };
          }

          // If adapted vj doesn't exist, we get_or_create a new jp with a new list of stop points
          // after the application of impact
          auto new_jp = get_new_jp_without_impacted_stop_points(*jp, impacted_stop_points);

          // It's not possible that jp and new_jp are same, beacuse they cannot have the same stop points
          assert(new_jp != jp);

          functor_copy_vj_to_jp{impact, new_jp, impacted_stop_points, pt_data, meta}(*vj);

          // The new journey_pattern is linked to the impact
          push_back_unique(impact->impacted_journey_patterns, new_jp);
      }
      return true;
  }

private:

  nt::PT_Data& pt_data;
  const std::set<const nt::StopPoint*>& impacted_stop_points;
  nt::JourneyPattern* jp;
  const boost::shared_ptr<nt::new_disruption::Impact>& impact;
  const nt::MetaData& meta;

  std::vector<std::pair<nt::DiscreteVehicleJourney*, nt::JourneyPattern*>> vj_discrete_to_be_updated;
  std::vector<std::pair<nt::FrequencyVehicleJourney*, nt::JourneyPattern*>> vj_frequency_to_be_updated;

};


// this is a convenient functor to collect all impacts that are applied on a jp
// You can call jp->for_each_vehicle_journey(ImpactsCollector{}) to get all impacts
struct ImpactsCollector {
public:
    template<typename T>
    bool operator()(std::unique_ptr<T>& vj) {
        boost::for_each(vj->impacted_by, [&](const typename decltype(vj->impacted_by)::value_type& weak_ptr){
                if(auto shared_ptr = weak_ptr.lock()) {
                    jp_impacts.insert(shared_ptr.get());
                };
            });
        return true;
    }
    std::set<nt::new_disruption::Impact*> jp_impacts{};
};


struct add_impacts_visitor : public apply_impacts_visitor {
    add_impacts_visitor(const boost::shared_ptr<nt::new_disruption::Impact>& impact,
            nt::PT_Data& pt_data, const nt::MetaData& meta) :
                apply_impacts_visitor(impact, pt_data, meta, "add") {}

    ~add_impacts_visitor() {}
    add_impacts_visitor(const add_impacts_visitor&) = default;

    using apply_impacts_visitor::operator();

    bool func_on_vj(nt::VehicleJourney& vj) override{
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

    void remove_jp_if_no_vj(nt::JourneyPattern* jp, const std::set<nt::new_disruption::Impact*>& jp_impacts) {

        // test if there are any vj still attached to the jp
        if (! jp->discrete_vehicle_journey_list.empty() ||
                ! jp->frequency_vehicle_journey_list.empty()) {
            return;
        }
        // let's take advantage of RAII of unique_ptr
        auto jp_unique_ptr = std::unique_ptr<nt::JourneyPattern>{jp};

        // first, we remove all jpp in jp
        for(auto* jpp: jp->journey_pattern_point_list){
            // let's take advantage of RAII of unique_ptr
            auto jpp_unique_ptr = std::unique_ptr<nt::JourneyPatternPoint>{jpp};

            boost::range::remove_erase(pt_data.journey_pattern_points, jpp);

            pt_data.journey_pattern_points_map.erase(jpp->uri);

            boost::range::remove_erase(jpp->stop_point->journey_pattern_point_list, jpp);
        }

        boost::range::remove_erase_if(pt_data.journey_pattern_points, [&](nt::JourneyPatternPoint* jpp){
            return contains(jp->journey_pattern_point_list, jpp);
        });

        // Once journey points are deleted from its list, we should re-compute their idx
        pt_data.reindex_journey_pattern_points();

        // Then we remove jp from pt_data
        boost::range::remove_erase(pt_data.journey_patterns, jp);
        pt_data.journey_patterns_map.erase(jp->uri);

        // reindex journey patterns
        pt_data.reindex_journey_patterns();

        // remove jp from it's impacts
        boost::for_each(jp_impacts, [&](nt::new_disruption::Impact* impact){
            boost::range::remove_erase(impact->impacted_journey_patterns, jp);
        });
    }


    // add new journey pattern for impacted stop point
    void add_new_jp_for_impacted_sp(nt::JourneyPattern* jp,
            const std::set<const nt::StopPoint*>& impacted_stop_points) {

        // collect all impacts that are applied on the jp
        auto impacts_collector = ImpactsCollector{};
        jp->for_each_vehicle_journey_ptr(impacts_collector);


        vehicle_journey_impactor impactor{pt_data, impacted_stop_points, jp, impact, meta};
        jp->for_each_vehicle_journey_ptr(impactor);

        // the vehicle_journey_impactor has register the updated vj and its vj's new jp
        // now we should attach the vj to the new jp
        impactor.complete();

        // remove this jp if it doesn't contain any vj
        remove_jp_if_no_vj(jp, impacts_collector.jp_impacts);

    }

    void operator()(const nt::StopPoint* stop_point) {
        this->log_start_action(stop_point->uri);
        for (const auto* jp_point : stop_point->journey_pattern_point_list) {
            add_new_jp_for_impacted_sp(jp_point->journey_pattern, {stop_point});
        }
        this->log_end_action(stop_point->uri);
    }

    void operator()(const nt::StopArea* stop_area) {
        this->log_start_action(stop_area->uri);
        auto impacted_stop_points = std::set<const nt::StopPoint*>{std::begin(stop_area->stop_point_list),
            std::end(stop_area->stop_point_list)};
        auto jp_in_this_stop_area = std::set<nt::JourneyPattern*>{};
        // We first find out all journey patterns that pass by this stop area
        for (const auto* stop_point : impacted_stop_points) {
            for (const auto* jp_point:  stop_point->journey_pattern_point_list) {
                jp_in_this_stop_area.insert(jp_point->journey_pattern);
            }
        }
        for (auto* jp: jp_in_this_stop_area) {
            add_new_jp_for_impacted_sp(jp, impacted_stop_points);
        }
        this->log_end_action(stop_area->uri);
    }
};

void apply_impact(boost::shared_ptr<nt::new_disruption::Impact> impact,
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
    size_t nb_vj_reassigned = 0;

    std::set<boost::shared_ptr<nt::new_disruption::Impact>> other_impacts;

    delete_impacts_visitor(boost::shared_ptr<nt::new_disruption::Impact> impact,
            nt::PT_Data& pt_data, const nt::MetaData& meta) :
                apply_impacts_visitor(impact, pt_data, meta, "delete") {}

    ~delete_impacts_visitor() {}

    using apply_impacts_visitor::operator();

    // We set all the validity pattern to the theorical one, we will re-apply
    // other disruptions after
    bool func_on_vj(nt::VehicleJourney& vj) override{
        vj.adapted_validity_pattern = vj.validity_pattern;
        ++ nb_vj_reassigned;
        const auto& impact = this->impact;
        boost::range::remove_erase_if(vj.impacted_by,
                [&impact](const boost::weak_ptr<nt::new_disruption::Impact>& i) {
            auto spt = i.lock();
            return (spt) ? spt == impact : true;
        });
        return true;
    }

    void delete_impacts_impl() {

        // We need to erase vehicle journeys from the collections and the map
        // We do not remove the link journey_pattern->vj, because we will
        // remove the journey_pattern_after

        std::set<nt::idx_t> vehicle_journeys_to_erase;
        std::set<nt::idx_t> jpp_to_erase;

        LOG4CPLUS_TRACE(log4cplus::Logger::getInstance("log"),
                "Removing jpp from stop_points");
        const auto& jp_to_delete = impact->impacted_journey_patterns;
        for (auto* journey_pattern : jp_to_delete) {
            journey_pattern->for_each_vehicle_journey([&](const type::VehicleJourney& vj) {
                // We should save all the other impacts previously applied, so that
                // we cab re-apply them later.
                for (auto& impact_weak_ptr: vj.impacted_by) {
                    auto ptr = impact_weak_ptr.lock();
                    if (ptr && ptr.get() != impact.get()) {
                        other_impacts.insert(ptr);
                    }
                }
                vehicle_journeys_to_erase.insert(vj.idx);
                return true;
            });

            // We need to remove the journey_pattern_point from its stop_point
            // because we will remove the journey_pattern_point after.
            for (auto* journey_pattern_point : journey_pattern->journey_pattern_point_list) {

                boost::range::remove_erase_if(journey_pattern_point->stop_point->journey_pattern_point_list,
                        [&](const nt::JourneyPatternPoint* jpp) {return journey_pattern_point->uri == jpp->uri;});

                jpp_to_erase.insert(journey_pattern_point->idx);
            }
        }

        LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("log"),
                "Removing vehicle_journeys");
        for(auto it = vehicle_journeys_to_erase.rbegin(); it != vehicle_journeys_to_erase.rend(); ++it) {
            // We should also remove the adapted vj from meta_vj
            auto* vj = pt_data.vehicle_journeys[*it];

            auto* meta_vj = pt_data.meta_vj[vj->theoric_vehicle_journey->uri];
            boost::range::remove_erase(meta_vj->adapted_vj, vj);
            pt_data.remove_from_collections(*vj);
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

            LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("log"),
                    "Removing journey patterns from journey patterns pool");

            // Now we can erase journey_patterns from the dataset
            pt_data.journey_patterns_map.erase(journey_pattern->uri);
            boost::range::remove_erase_if(pt_data.journey_patterns, [&](nt::JourneyPattern* jp){
                return jp->uri == journey_pattern->uri;
            });
        }
        pt_data.reindex_journey_patterns();

        //we delete all impacted journey pattern, so there is no more impacted journey pattern,
        //if we don't do that we will try to delete them multiple times
        impact->impacted_journey_patterns.clear();

        // Now we should clear the JP pool, if the jp exists no longer in the journey_patterns_map,
        // it should be removed from jp
        std::vector<nt::JourneyPatternKey> jp_to_remove_from_pool;
        boost::for_each(pt_data.journey_patterns_pool,
                [&](const decltype(pt_data.journey_patterns_pool)::value_type& key_pair){
            if (! contains(pt_data.journey_patterns_map, key_pair.second->uri)) {
                jp_to_remove_from_pool.push_back(key_pair.first);
            }
        });
        boost::for_each(jp_to_remove_from_pool, [&](const nt::JourneyPatternKey& key){
            pt_data.journey_patterns_pool.erase(key);
        });
    }

    template<typename T>
    void reassign_validity_pattern(const std::vector<T*>& stop_points) {
        LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("log"),
                "Reassigning validity_pattern");
        for (const auto* stop_point : stop_points) {
            for (const auto* journey_pattern_point : stop_point->journey_pattern_point_list) {
                (*this)(journey_pattern_point->journey_pattern);
            }
        }
        LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("log"),
                nb_vj_reassigned << " validity pattern reassigned");
    }


    void operator()(const nt::StopPoint* stop_point)  {

        this->log_start_action(stop_point->uri);

        delete_impacts_impl();

        reassign_validity_pattern(std::vector<const nt::StopPoint*>{stop_point});

        this->log_end_action(stop_point->uri);
    }

    void operator()(const nt::StopArea* stop_area) {
        this->log_start_action(stop_area->uri);

        delete_impacts_impl();

        reassign_validity_pattern(stop_area->stop_point_list);

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

    // We should re-apply all other impacts
    boost::for_each(v.other_impacts, [&](decltype(v.other_impacts)::const_reference i){
        if (i) { apply_impact(i, pt_data, meta); }
    });

    LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("log"),
            impact.get()->uri << " deleted");
}

} // anonymous namespace

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
        for (const auto& impact : (*it)->get_impacts()) {
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

    //we delete the disruption before adding the new one
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
