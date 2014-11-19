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

#include <boost/make_shared.hpp>

namespace navitia {

namespace nt = navitia::type;

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

static std::vector<nt::new_disruption::PtObj>
make_pt_objects(const google::protobuf::RepeatedPtrField<chaos::PtObject>& chaos_pt_objects,
                nt::PT_Data& pt_data,
                const boost::shared_ptr<nt::new_disruption::Impact> &impact = {}) {
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
            LOG4CPLUS_WARN(log4cplus::Logger::getInstance("log"), "fill_disruption_from_chaos: LineSection not yet supported");
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

// TODO: remove this when we remove EntitySelector in Chaos
static std::vector<nt::new_disruption::PtObj>
make_pt_objects(const google::protobuf::RepeatedPtrField<transit_realtime::EntitySelector>& chaos_pt_objects,
                nt::PT_Data& pt_data) {
    std::vector<nt::new_disruption::PtObj> res;
    for (const auto& chaos_selector: chaos_pt_objects) {
        if (chaos_selector.has_stop_id()) {
            res.push_back(nt::new_disruption::make_pt_obj(nt::Type_e::StopArea, chaos_selector.stop_id(), pt_data));
            continue;
        }
        LOG4CPLUS_WARN(log4cplus::Logger::getInstance("log"), "EntitySelector contains not supported data");
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
        impact->messages.push_back({
            chaos_message.text(),
            from_posix(chaos_message.created_at()),
            from_posix(chaos_message.updated_at()),
        });
    }

    return std::move(impact);
}

void add_disruption(nt::PT_Data& pt_data,
                    const chaos::Disruption& chaos_disruption) {
    auto from_posix = navitia::from_posix_timestamp;
    nt::new_disruption::DisruptionHolder &holder = pt_data.disruption_holder;

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
        disruption->add_impact(make_impact(chaos_impact, pt_data));
    }
    disruption->localization = make_pt_objects(chaos_disruption.localization(), pt_data);
    for (const auto& chaos_tag: chaos_disruption.tags()) {
        disruption->tags.push_back(make_tag(chaos_tag, holder));
    }
    disruption->note = chaos_disruption.note();

    holder.disruptions.push_back(std::move(disruption));
}

} // namespace navitia
