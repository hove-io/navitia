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
#include "make_disruption_from_chaos.h"
#include "apply_disruption.h"
#include "utils/logger.h"
#include <boost/make_shared.hpp>

namespace bt = boost::posix_time;

namespace navitia {

static void update_tag(nt::disruption::Tag& tag, const chaos::Tag& chaos_tag) {
    auto from_posix = navitia::from_posix_timestamp;

    tag.uri = chaos_tag.id();
    tag.name = chaos_tag.name();
    tag.created_at = from_posix(chaos_tag.created_at());
    tag.updated_at = from_posix(chaos_tag.updated_at());
}

static boost::shared_ptr<nt::disruption::Tag>
make_tag(const chaos::Tag& chaos_tag, nt::disruption::DisruptionHolder& holder) {
    auto& weak_tag = holder.tags[chaos_tag.id()];
    auto tag = weak_tag.lock();
    if (! tag) {
        tag = boost::make_shared<nt::disruption::Tag>();
        weak_tag = tag;
    }

    update_tag(*tag, chaos_tag);

    return tag;
}

static void update_cause(nt::disruption::Cause& cause, const chaos::Cause& chaos_cause) {
    auto from_posix = navitia::from_posix_timestamp;
    cause.uri = chaos_cause.id();
    cause.wording = chaos_cause.wording();
    cause.created_at = from_posix(chaos_cause.created_at());
    cause.updated_at = from_posix(chaos_cause.updated_at());
    if (chaos_cause.has_category()) {
        cause.category = chaos_cause.category().name();
    }
}

static boost::shared_ptr<nt::disruption::Cause>
make_cause(const chaos::Cause& chaos_cause, nt::disruption::DisruptionHolder& holder) {
    auto& weak_cause = holder.causes[chaos_cause.id()];
    auto cause = weak_cause.lock();
    if (! cause) {
        cause = boost::make_shared<nt::disruption::Cause>();
        weak_cause = cause;
    }

    update_cause(*cause, chaos_cause);
    return cause;

}

static void update_severity(nt::disruption::Severity& severity, const chaos::Severity& chaos_severity) {
    namespace tr = transit_realtime;
    namespace new_disr = nt::disruption;
    auto from_posix = navitia::from_posix_timestamp;

    severity.uri = chaos_severity.id();
    severity.wording = chaos_severity.wording();
    severity.created_at = from_posix(chaos_severity.created_at());
    severity.updated_at = from_posix(chaos_severity.updated_at());
    severity.color = chaos_severity.color();
    severity.priority = chaos_severity.priority();
    switch (chaos_severity.effect()) {
#define EFFECT_ENUM_CONVERSION(e) \
        case tr::Alert_Effect_##e: severity.effect = new_disr::Effect::e; break

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
}

static boost::shared_ptr<nt::disruption::Severity>
make_severity(const chaos::Severity& chaos_severity, nt::disruption::DisruptionHolder& holder) {
    auto& weak_severity = holder.severities[chaos_severity.id()];
    auto severity = weak_severity.lock();
    if (! severity) {
        severity = boost::make_shared<nt::disruption::Severity>();
        weak_severity = severity;
    }

    update_severity(*severity, chaos_severity);

    return severity;
}

static boost::optional<nt::disruption::LineSection>
make_line_section(const chaos::PtObject& chaos_section,
                  nt::PT_Data& pt_data) {
    if (!chaos_section.has_pt_line_section()) {
        LOG4CPLUS_WARN(log4cplus::Logger::getInstance("log"),
                       "fill_disruption_from_chaos: LineSection invalid!");
        return boost::none;
    }
    const auto& pb_section = chaos_section.pt_line_section();
    nt::disruption::LineSection line_section;
    auto* line = find_or_default(pb_section.line().uri(), pt_data.lines_map);
    if (line) {
        line_section.line = line;
    } else {
        LOG4CPLUS_WARN(log4cplus::Logger::getInstance("log"),
                       "fill_disruption_from_chaos: line id "
                       << pb_section.line().uri() << " in LineSection invalid!");
        return boost::none;
    }
    if (auto* start = find_or_default(pb_section.start_point().uri(), pt_data.stop_areas_map)) {
        line_section.start_point = start;
    } else {
        LOG4CPLUS_WARN(log4cplus::Logger::getInstance("log"),
                       "fill_disruption_from_chaos: start_point id "
                       << pb_section.start_point().uri() << " in LineSection invalid!");
        return boost::none;
    }
    if (auto* end = find_or_default(pb_section.end_point().uri(), pt_data.stop_areas_map)) {
        line_section.end_point = end;
    } else {
        LOG4CPLUS_WARN(log4cplus::Logger::getInstance("log"),
                       "fill_disruption_from_chaos: end_point id "
                       << pb_section.end_point().uri() << " in LineSection invalid!");
        return boost::none;
    }
    if(!pb_section.routes().empty()) {
        for(const auto& pb_route: pb_section.routes()) {
            if (auto* route = find_or_default(pb_route.uri(), pt_data.routes_map)) {
                line_section.routes.push_back(route);
            } else {
                LOG4CPLUS_WARN(log4cplus::Logger::getInstance("log"),
                               "fill_disruption_from_chaos: route id "
                               << pb_route.uri() << " in LineSection invalid!");
                return boost::none;
            }
        }
    } else {
        LOG4CPLUS_TRACE(log4cplus::Logger::getInstance("trace"),
                        "Empty routes for line_section on line" << line_section.line->uri <<
                        ". Applying disruptions on all routes");
        line_section.routes = line_section.line->route_list;
    }

    return line_section;
}

static std::vector<nt::disruption::PtObj>
make_pt_objects(const google::protobuf::RepeatedPtrField<chaos::PtObject>& chaos_pt_objects,
                nt::PT_Data& pt_data) {
    using namespace nt::disruption;

    std::vector<PtObj> res;
    for (const auto& chaos_pt_object: chaos_pt_objects) {
        switch (chaos_pt_object.pt_object_type()) {
        case chaos::PtObject_Type_network:
            res.push_back(make_pt_obj(nt::Type_e::Network, chaos_pt_object.uri(), pt_data));
            break;
        case chaos::PtObject_Type_stop_area:
            res.push_back(make_pt_obj(nt::Type_e::StopArea, chaos_pt_object.uri(), pt_data));
            break;
        case chaos::PtObject_Type_stop_point:
            res.push_back(make_pt_obj(nt::Type_e::StopPoint, chaos_pt_object.uri(), pt_data));
            break;
        case chaos::PtObject_Type_line_section:
            if (auto line_section = make_line_section(chaos_pt_object, pt_data)) {
                res.push_back(*line_section);
            }
            break;
        case chaos::PtObject_Type_line:
            res.push_back(make_pt_obj(nt::Type_e::Line, chaos_pt_object.uri(), pt_data));
            break;
        case chaos::PtObject_Type_route:
            res.push_back(make_pt_obj(nt::Type_e::Route, chaos_pt_object.uri(), pt_data));
            break;
        case chaos::PtObject_Type_trip:
            res.push_back(make_pt_obj(nt::Type_e::MetaVehicleJourney, chaos_pt_object.uri(), pt_data));
            break;
        case chaos::PtObject_Type_unkown_type:
            res.push_back(UnknownPtObj());
            break;
        }
        // no created_at and updated_at?
    }
    return res;
}

static std::set<nt::disruption::ChannelType> create_channel_types(const chaos::Channel& chaos_channel) {
    std::set<navitia::type::disruption::ChannelType> res;
    for (const auto channel_type: chaos_channel.types()){
        switch(channel_type){
        case chaos::Channel_Type_web:
            res.insert(nt::disruption::ChannelType::web);
            break;
        case chaos::Channel_Type_sms:
            res.insert(nt::disruption::ChannelType::sms);
            break;
        case chaos::Channel_Type_email:
            res.insert(nt::disruption::ChannelType::email);
            break;
        case chaos::Channel_Type_mobile:
            res.insert(nt::disruption::ChannelType::mobile);
            break;
        case chaos::Channel_Type_notification:
            res.insert(nt::disruption::ChannelType::notification);
            break;
        case chaos::Channel_Type_twitter:
            res.insert(nt::disruption::ChannelType::twitter);
            break;
        case chaos::Channel_Type_facebook:
            res.insert(nt::disruption::ChannelType::facebook);
            break;
        case chaos::Channel_Type_unkown_type:
            res.insert(nt::disruption::ChannelType::unknown_type);
            break;
        default:
            throw navitia::exception("Unhandled ChannelType value in Chaos.Proto");
        }
    }
    return res;
}

static boost::shared_ptr<nt::disruption::Impact>
make_impact(const chaos::Impact& chaos_impact, nt::PT_Data& pt_data,
            const navitia::type::MetaData& meta) {
    auto from_posix = navitia::from_posix_timestamp;
    nt::disruption::DisruptionHolder& holder = pt_data.disruption_holder;

    auto impact = boost::make_shared<nt::disruption::Impact>();
    impact->uri = chaos_impact.id();
    impact->created_at = from_posix(chaos_impact.created_at());
    auto updated_at = chaos_impact.updated_at();
    impact->updated_at = updated_at ? from_posix(updated_at) : impact->created_at;
    for (const auto& chaos_ap: chaos_impact.application_periods()) {
        impact->application_periods.emplace_back(from_posix(chaos_ap.start()), from_posix(chaos_ap.end()));
    }
    impact->severity = make_severity(chaos_impact.severity(), holder);

    for (auto& ptobj: make_pt_objects(chaos_impact.informed_entities(), pt_data)) {
        nt::disruption::Impact::link_informed_entity(std::move(ptobj),
                                                     impact, meta.production_date, nt::RTLevel::Adapted);
    }
    for (const auto& chaos_message: chaos_impact.messages()) {
        const auto& channel = chaos_message.channel();
        auto channel_types = create_channel_types(channel);
        impact->messages.push_back({
            chaos_message.text(),
            channel.id(),
            channel.name(),
            channel.content_type(),
            from_posix(chaos_message.created_at()),
            from_posix(chaos_message.updated_at()),
            channel_types
        });
    }

    return impact;
}

static const type::disruption::Disruption&
make_disruption(const chaos::Disruption& chaos_disruption, nt::PT_Data& pt_data,
                const navitia::type::MetaData& meta) {
    auto log = log4cplus::Logger::getInstance("log");
    LOG4CPLUS_DEBUG(log, "Adding disruption: " << chaos_disruption.id());
    auto from_posix = navitia::from_posix_timestamp;
    nt::disruption::DisruptionHolder& holder = pt_data.disruption_holder;

    auto& disruption = holder.make_disruption(chaos_disruption.id(), nt::RTLevel::Adapted);

    if (chaos_disruption.has_contributor()) {
        disruption.contributor = chaos_disruption.contributor();
    }

    disruption.reference = chaos_disruption.reference();
    disruption.publication_period = {
        from_posix(chaos_disruption.publication_period().start()),
        from_posix(chaos_disruption.publication_period().end())
    };
    disruption.created_at = from_posix(chaos_disruption.created_at());
    disruption.updated_at = from_posix(chaos_disruption.updated_at());
    disruption.cause = make_cause(chaos_disruption.cause(), holder);
    for (const auto& chaos_impact: chaos_disruption.impacts()) {
        auto impact = make_impact(chaos_impact, pt_data, meta);
        disruption.add_impact(impact, holder);
    }
    disruption.localization = make_pt_objects(chaos_disruption.localization(), pt_data);
    for (const auto& chaos_tag: chaos_disruption.tags()) {
        disruption.tags.push_back(make_tag(chaos_tag, holder));
    }
    disruption.note = chaos_disruption.note();

    LOG4CPLUS_DEBUG(log, chaos_disruption.id() << " disruption added");

    return disruption;
}

void make_and_apply_disruption(const chaos::Disruption& chaos_disruption,
                    navitia::type::PT_Data& pt_data,
                    const navitia::type::MetaData& meta) {
    //we delete the disruption before adding the new one
    delete_disruption(chaos_disruption.id(), pt_data, meta);

    const auto& disruption = make_disruption(chaos_disruption, pt_data, meta);

    apply_disruption(disruption, pt_data, meta);
}
}
