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
#include "realtime.h"

#include "kraken/apply_disruption.h"
#include "type/data.h"
#include "type/datetime.h"
#include "type/kirin.pb.h"
#include "type/meta_data.h"
#include "type/pt_data.h"
#include "type/meta_vehicle_journey.h"
#include "utils/functions.h"
#include "utils/logger.h"

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/make_shared.hpp>
#include <boost/optional.hpp>

#include <chrono>

namespace navitia {

namespace nd = type::disruption;
namespace nt = navitia::type;

static bool base_vj_exists_the_same_day(const type::Data& data, const transit_realtime::TripUpdate& trip_update) {
    const auto& mvj = *data.pt_data->get_or_create_meta_vehicle_journey(trip_update.trip().trip_id(),
                                                                        data.pt_data->get_main_timezone());
    return mvj.get_base_vj_circulating_at_date(
        boost::gregorian::from_undelimited_string(trip_update.trip().start_date()));
}

static bool is_cancelled_trip(const transit_realtime::TripUpdate& trip_update) {
    if (trip_update.HasExtension(kirin::effect)) {
        return trip_update.GetExtension(kirin::effect) == transit_realtime::Alert_Effect::Alert_Effect_NO_SERVICE;
    }  // TODO: remove this deprecated code (for retrocompatibility with Kirin < 0.8.0 only)
    return trip_update.trip().schedule_relationship() == transit_realtime::TripDescriptor_ScheduleRelationship_CANCELED;
}

static bool is_circulating_trip(const transit_realtime::TripUpdate& trip_update) {
    if (trip_update.HasExtension(kirin::effect)) {
        return (trip_update.GetExtension(kirin::effect) == transit_realtime::Alert_Effect::Alert_Effect_REDUCED_SERVICE
                || trip_update.GetExtension(kirin::effect)
                       == transit_realtime::Alert_Effect::Alert_Effect_UNKNOWN_EFFECT
                || trip_update.GetExtension(kirin::effect)
                       == transit_realtime::Alert_Effect::Alert_Effect_SIGNIFICANT_DELAYS
                || trip_update.GetExtension(kirin::effect) == transit_realtime::Alert_Effect::Alert_Effect_DETOUR
                || trip_update.GetExtension(kirin::effect)
                       == transit_realtime::Alert_Effect::Alert_Effect_MODIFIED_SERVICE
                || trip_update.GetExtension(kirin::effect)
                       == transit_realtime::Alert_Effect::Alert_Effect_ADDITIONAL_SERVICE)
               && trip_update.stop_time_update_size();
    }  // TODO: remove this deprecated code (for retrocompatibility with Kirin < 0.8.0 only)
    return (trip_update.trip().schedule_relationship()
                == transit_realtime::TripDescriptor_ScheduleRelationship_SCHEDULED
            || trip_update.trip().schedule_relationship()
                   == transit_realtime::TripDescriptor_ScheduleRelationship_ADDED)
           && trip_update.stop_time_update_size();
}

static bool is_deleted(const transit_realtime::TripUpdate_StopTimeEvent& event) {
    if (event.HasExtension(kirin::stop_time_event_status)) {
        return contains({kirin::StopTimeEventStatus::DELETED, kirin::StopTimeEventStatus::DELETED_FOR_DETOUR},
                        event.GetExtension(kirin::stop_time_event_status));
    }
    return false;
}

static bool check_trip_update(const transit_realtime::TripUpdate& trip_update) {
    auto log = log4cplus::Logger::getInstance("realtime");
    if (is_circulating_trip(trip_update)) {
        uint32_t last_stop_event_time = std::numeric_limits<uint32_t>::min();
        for (const auto& st : trip_update.stop_time_update()) {
            uint32_t arrival_time = st.arrival().time();
            if (!is_deleted(st.arrival())) {
                if (last_stop_event_time > arrival_time) {
                    LOG4CPLUS_WARN(log, "Trip Update " << trip_update.trip().trip_id() << ": Stop time " << st.stop_id()
                                                       << " is not correctly ordered regarding arrival");
                    return false;
                }
                last_stop_event_time = arrival_time;
            }

            uint32_t departure_time = st.departure().time();
            if (!is_deleted(st.departure())) {
                if (last_stop_event_time > departure_time) {
                    LOG4CPLUS_WARN(log, "Trip Update " << trip_update.trip().trip_id() << ": Stop time " << st.stop_id()
                                                       << " is not correctly ordered regarding departure");
                    return false;
                }
                last_stop_event_time = departure_time;
            }
        }
    }
    return true;
}

static std::ostream& operator<<(std::ostream& s, const nt::StopTime& st) {
    // Note: do not use st.order, as the stoptime might not yet be part of the VJ
    return s << "ST " << (st.vehicle_journey ? st.vehicle_journey->uri : "Null") << " dep " << st.departure_time
             << " arr " << st.arrival_time;
}

/**
 * Check if a disruption is valid
 *
 * We check that all stop times are correctly ordered
 * And that the departure/arrival are correct too
 */
static bool check_disruption(const nt::disruption::Disruption& disruption) {
    auto log = log4cplus::Logger::getInstance("realtime");
    using nt::disruption::StopTimeUpdate;
    for (const auto& impact : disruption.get_impacts()) {
        uint32_t last_stop_event_time = std::numeric_limits<uint32_t>::min();
        for (const auto& stu : impact->aux_info.stop_times) {
            const auto& st = stu.stop_time;

            bool arr_deleted = contains({StopTimeUpdate::Status::DELETED, StopTimeUpdate::Status::DELETED_FOR_DETOUR},
                                        stu.arrival_status);
            if (!arr_deleted) {
                if (last_stop_event_time > st.arrival_time) {
                    LOG4CPLUS_WARN(log, "stop time " << st << " is not correctly ordered regarding arrival");
                    return false;
                }
                last_stop_event_time = st.arrival_time;
            }
            bool dep_deleted = contains({StopTimeUpdate::Status::DELETED, StopTimeUpdate::Status::DELETED_FOR_DETOUR},
                                        stu.departure_status);
            if (!dep_deleted) {
                if (last_stop_event_time > st.departure_time) {
                    LOG4CPLUS_WARN(log, "stop time " << st << " is not correctly ordered regarding departure");
                    return false;
                }
                last_stop_event_time = st.departure_time;
            }
        }
    }
    return true;
}

static boost::shared_ptr<nt::disruption::Severity> make_severity(const std::string& id,
                                                                 std::string wording,
                                                                 nt::disruption::Effect effect,
                                                                 const boost::posix_time::ptime& timestamp,
                                                                 nt::disruption::DisruptionHolder& holder) {
    auto& weak_severity = holder.severities[id];
    if (auto severity = weak_severity.lock()) {
        return severity;
    }

    auto severity = boost::make_shared<nt::disruption::Severity>();
    severity->uri = id;
    severity->wording = std::move(wording);
    severity->created_at = timestamp;
    severity->updated_at = timestamp;
    severity->color = "#000000";
    severity->priority = 42;
    severity->effect = effect;

    weak_severity = severity;
    return severity;
}

static boost::posix_time::time_period base_execution_period(const boost::gregorian::date& date,
                                                            const nt::MetaVehicleJourney& mvj) {
    auto running_vj = mvj.get_base_vj_circulating_at_date(date);
    if (running_vj) {
        return running_vj->execution_period(date);
    }
    // If there is no running vj at this date, we return a "null", reverted period [infinity; -infinity].
    // This helps compute the period using min and max.
    return {boost::gregorian::max_date_time, boost::posix_time::ptime(boost::gregorian::min_date_time)};
}

static type::disruption::Message create_message(const std::string& str_msg) {
    type::disruption::Message msg;
    msg.text = str_msg;
    msg.channel_id = "rt";
    msg.channel_name = "rt";
    msg.channel_types = {nd::ChannelType::web, nd::ChannelType::mobile};

    return msg;
}

static transit_realtime::TripUpdate_StopTimeUpdate_ScheduleRelationship get_relationship(
    const transit_realtime::TripUpdate_StopTimeEvent& event,
    const transit_realtime::TripUpdate_StopTimeUpdate& st) {
    // we get either the stop_time_event if we have it or we use the GTFS-RT standard (on st)
    if (event.HasExtension(kirin::stop_time_event_relationship)) {
        return event.GetExtension(kirin::stop_time_event_relationship);
    }
    return st.schedule_relationship();
}

static nt::disruption::StopTimeUpdate::Status get_status(const transit_realtime::TripUpdate_StopTimeEvent& event,
                                                         const transit_realtime::TripUpdate_StopTimeUpdate& st) {
    if (event.HasExtension(kirin::stop_time_event_status)) {
        const auto event_status = event.GetExtension(kirin::stop_time_event_status);
        switch (event_status) {
            case kirin::StopTimeEventStatus::ADDED:
                return nt::disruption::StopTimeUpdate::Status::ADDED;
            case kirin::StopTimeEventStatus::DELETED:
                return nt::disruption::StopTimeUpdate::Status::DELETED;
            case kirin::StopTimeEventStatus::DELETED_FOR_DETOUR:
                return nt::disruption::StopTimeUpdate::Status::DELETED_FOR_DETOUR;
            case kirin::StopTimeEventStatus::ADDED_FOR_DETOUR:
                return nt::disruption::StopTimeUpdate::Status::ADDED_FOR_DETOUR;
            default:
                break;
        }
    } else {
        // TODO: to be deleted once this version is deployed in prod.
        auto transit_realtime_status = get_relationship(event, st);
        switch (transit_realtime_status) {
            case transit_realtime::TripUpdate_StopTimeUpdate_ScheduleRelationship_SKIPPED:
                return nt::disruption::StopTimeUpdate::Status::DELETED;
            case transit_realtime::TripUpdate_StopTimeUpdate_ScheduleRelationship_ADDED:
                return nt::disruption::StopTimeUpdate::Status::ADDED;
            default:
                break;
        }
    }
    if (!event.has_delay() || event.delay() != 0) {
        return nt::disruption::StopTimeUpdate::Status::DELAYED;
    }
    return nt::disruption::StopTimeUpdate::Status::UNCHANGED;
}

static bool is_added_trip(const transit_realtime::TripUpdate& trip_update) {
    auto log = log4cplus::Logger::getInstance("realtime");

    if (trip_update.HasExtension(kirin::effect)) {
        if (trip_update.GetExtension(kirin::effect)
            == transit_realtime::Alert_Effect::Alert_Effect_ADDITIONAL_SERVICE) {
            LOG4CPLUS_TRACE(log, "Disruption has ADDITIONAL_SERVICE effect");
            return true;
        }
        return false;

    }  // TODO: remove this deprecated code (for retrocompatibility with Kirin < 0.8.0 only)
    if (trip_update.trip().schedule_relationship() == transit_realtime::TripDescriptor_ScheduleRelationship_ADDED) {
        LOG4CPLUS_TRACE(log, "Disruption has ADDITIONAL_SERVICE effect");
        return true;
    }
    return false;
}

static bool is_added_stop_time(const transit_realtime::TripUpdate& trip_update) {
    auto log = log4cplus::Logger::getInstance("realtime");
    using nt::disruption::StopTimeUpdate;

    if (trip_update.stop_time_update_size()) {
        for (const auto& st : trip_update.stop_time_update()) {
            // adding a stop_time event (adding departure or/and arrival) is adding service
            if (contains({StopTimeUpdate::Status::ADDED, StopTimeUpdate::Status::ADDED_FOR_DETOUR},
                         get_status(st.departure(), st))
                || contains({StopTimeUpdate::Status::ADDED, StopTimeUpdate::Status::ADDED_FOR_DETOUR},
                            get_status(st.arrival(), st))) {
                LOG4CPLUS_TRACE(log, "Disruption has ADDED stop_time event");
                return true;
            }
        }
    }
    return false;
}

static bool is_handleable(const transit_realtime::TripUpdate& trip_update,
                          const nt::PT_Data& pt_data,
                          const bool is_realtime_add_enabled,
                          const bool is_realtime_add_trip_enabled) {
    namespace bpt = boost::posix_time;

    auto log = log4cplus::Logger::getInstance("realtime");

    // if is_realtime_add_enabled = false, this disables the adding trip option too
    if ((!is_realtime_add_enabled && is_added_stop_time(trip_update))
        || (!is_realtime_add_enabled && is_added_trip(trip_update))) {
        LOG4CPLUS_DEBUG(log, "Add stop time is disabled: ignoring trip update id "
                                 << trip_update.trip().trip_id()
                                 << " with ADDED/ADDED_FOR_DETOUR stop times or ADDITIONAL_SERVICE effect");
        return false;
    }
    if (!is_realtime_add_trip_enabled && is_added_trip(trip_update)) {
        LOG4CPLUS_DEBUG(log, "Add trip is disabled: ignoring trip update id " << trip_update.trip().trip_id()
                                                                              << " with ADDITIONAL_SERVICE effect");
        return false;
    }
    // Case 1: the trip is cancelled
    if (is_cancelled_trip(trip_update)) {
        return true;
    }
    // Case 2: the trip is not cancelled, but modified (ex: the train's arrival is delayed or the train's
    // departure is brought forward), we check the size of stop_time_update because we can't find a proper
    // enum in gtfs-rt proto to express this idea
    if (is_circulating_trip(trip_update)) {
        if (is_added_trip(trip_update)) {
            // check if company id exists
            if (trip_update.trip().HasExtension(kirin::company_id)
                && !navitia::contains(pt_data.companies_map, trip_update.trip().GetExtension(kirin::company_id))) {
                LOG4CPLUS_DEBUG(log, "Trip company id " << trip_update.trip().GetExtension(kirin::company_id)
                                                        << " doesn't exist: ignoring trip update id "
                                                        << trip_update.trip().trip_id());
                return false;
            }
            // check if physical mode id exists
            if (trip_update.vehicle().HasExtension(kirin::physical_mode_id)
                && !navitia::contains(pt_data.physical_modes_map,
                                      trip_update.vehicle().GetExtension(kirin::physical_mode_id))) {
                LOG4CPLUS_DEBUG(log, "Trip physical mode id "
                                         << trip_update.vehicle().GetExtension(kirin::physical_mode_id)
                                         << " doesn't exist: ignoring trip update id " << trip_update.trip().trip_id());
                return false;
            }
            // check if line id exists (except if empty)
            if (trip_update.trip().HasExtension(kirin::line_id)
                && !trip_update.trip().GetExtension(kirin::line_id).empty()
                && !navitia::contains(pt_data.lines_map, trip_update.trip().GetExtension(kirin::line_id))) {
                LOG4CPLUS_DEBUG(log, "Trip line id " << trip_update.trip().GetExtension(kirin::line_id)
                                                     << " doesn't exist: ignoring trip update id "
                                                     << trip_update.trip().trip_id());
                return false;
            }
            // check if network id exists (except if empty)
            if (trip_update.trip().HasExtension(kirin::network_id)
                && !trip_update.trip().GetExtension(kirin::network_id).empty()
                && !navitia::contains(pt_data.networks_map, trip_update.trip().GetExtension(kirin::network_id))) {
                LOG4CPLUS_DEBUG(log, "Trip network id " << trip_update.trip().GetExtension(kirin::network_id)
                                                        << " doesn't exist: ignoring trip update id "
                                                        << trip_update.trip().trip_id());
                return false;
            }
            // check if dataset id exists (except if empty)
            if (trip_update.trip().HasExtension(kirin::dataset_id)
                && !trip_update.trip().GetExtension(kirin::dataset_id).empty()
                && !navitia::contains(pt_data.datasets_map, trip_update.trip().GetExtension(kirin::dataset_id))) {
                LOG4CPLUS_DEBUG(log, "Trip dataset id " << trip_update.trip().GetExtension(kirin::dataset_id)
                                                        << " doesn't exist: ignoring trip update id "
                                                        << trip_update.trip().trip_id());
                return false;
            }
            // check if commercial_mode id exists (except if empty)
            if (trip_update.trip().HasExtension(kirin::commercial_mode_id)
                && !trip_update.trip().GetExtension(kirin::commercial_mode_id).empty()
                && !navitia::contains(pt_data.commercial_modes_map,
                                      trip_update.trip().GetExtension(kirin::commercial_mode_id))) {
                LOG4CPLUS_DEBUG(log, "Trip commercial_mode id "
                                         << trip_update.trip().GetExtension(kirin::commercial_mode_id)
                                         << " doesn't exist: ignoring trip update id " << trip_update.trip().trip_id());
                return false;
            }
        }
        // WARNING: here trip.start_date is considered UTC, not local
        //(this date differs if vj starts during the period between midnight UTC and local midnight)
        auto start_date = boost::gregorian::from_undelimited_string(trip_update.trip().start_date());

        auto start_first_day_of_impact = bpt::ptime(start_date, bpt::time_duration(0, 0, 0, 0));
        for (const auto& st : trip_update.stop_time_update()) {
            auto ptime_arrival = bpt::from_time_t(st.arrival().time());
            auto ptime_departure = bpt::from_time_t(st.departure().time());
            if (ptime_arrival < start_first_day_of_impact || ptime_departure < start_first_day_of_impact) {
                LOG4CPLUS_WARN(log, "Trip Update " << trip_update.trip().trip_id() << ": Stop time " << st.stop_id()
                                                   << " is before the day of impact");
                return false;
            }
        }
        return true;
    }
    return false;
}

// TODO: remove this deprecated code (for retrocompatibility with Kirin < 0.8.0 only)
static nt::disruption::Effect get_calculated_trip_effect(nt::disruption::StopTimeUpdate::Status status) {
    using nt::disruption::Effect;
    using nt::disruption::StopTimeUpdate;

    switch (status) {
        case StopTimeUpdate::Status::UNCHANGED:  // it can be a back to normal case
            return Effect::UNKNOWN_EFFECT;
        case StopTimeUpdate::Status::DELAYED:
            return Effect::SIGNIFICANT_DELAYS;
        case StopTimeUpdate::Status::ADDED:
            return Effect::MODIFIED_SERVICE;
        case StopTimeUpdate::Status::DELETED:
            return Effect::REDUCED_SERVICE;
        case StopTimeUpdate::Status::ADDED_FOR_DETOUR:
        case StopTimeUpdate::Status::DELETED_FOR_DETOUR:
            return Effect::DETOUR;
        default:
            return Effect::UNKNOWN_EFFECT;
    }
}

static nt::disruption::Effect get_trip_effect(transit_realtime::Alert_Effect effect) {
    using nt::disruption::Effect;
    using transit_realtime::Alert_Effect;
    switch (effect) {
        case Alert_Effect::Alert_Effect_NO_SERVICE:
            return Effect::NO_SERVICE;
        case Alert_Effect::Alert_Effect_REDUCED_SERVICE:
            return Effect::REDUCED_SERVICE;
        case Alert_Effect::Alert_Effect_SIGNIFICANT_DELAYS:
            return Effect::SIGNIFICANT_DELAYS;
        case Alert_Effect::Alert_Effect_DETOUR:
            return Effect::DETOUR;
        case Alert_Effect::Alert_Effect_ADDITIONAL_SERVICE:
            return Effect::ADDITIONAL_SERVICE;
        case Alert_Effect::Alert_Effect_MODIFIED_SERVICE:
            return Effect::MODIFIED_SERVICE;
        case Alert_Effect::Alert_Effect_OTHER_EFFECT:
            return Effect::OTHER_EFFECT;
        case Alert_Effect::Alert_Effect_STOP_MOVED:
            return Effect::STOP_MOVED;
        case Alert_Effect::Alert_Effect_UNKNOWN_EFFECT:
        default:
            return Effect::UNKNOWN_EFFECT;
    }
}

static const std::string get_wordings(nt::disruption::Effect effect) {
    using nt::disruption::Effect;
    switch (effect) {
        case Effect::NO_SERVICE:
            return "trip canceled";
        case Effect::SIGNIFICANT_DELAYS:
            return "trip delayed";
        case Effect::DETOUR:
            return "detour";
        case Effect::MODIFIED_SERVICE:
            return "trip modified";
        case Effect::REDUCED_SERVICE:
            return "reduced service";
        case Effect::ADDITIONAL_SERVICE:
            return "additional service";
        case Effect::OTHER_EFFECT:
            return "other effect";
        case Effect::STOP_MOVED:
            return "stop moved";
        case Effect::UNKNOWN_EFFECT:
            return "unknown effect";
        default:
            return "";
    }
}

static const type::disruption::Disruption* create_disruption(const std::string& id,
                                                             const boost::posix_time::ptime& timestamp,
                                                             const transit_realtime::TripUpdate& trip_update,
                                                             const type::Data& data) {
    namespace bpt = boost::posix_time;
    auto log = log4cplus::Logger::getInstance("realtime");

    LOG4CPLUS_DEBUG(log, "Creating disruption");

    nt::disruption::DisruptionHolder& holder = data.pt_data->disruption_holder;

    // WARNING: here trip.start_date is considered UTC, not local
    //(this date differs if vj starts during the period between midnight UTC and local midnight)
    auto circulation_date = boost::gregorian::from_undelimited_string(trip_update.trip().start_date());

    auto start_first_day_of_impact = bpt::ptime(circulation_date, bpt::time_duration(0, 0, 0, 0));
    const auto& mvj = *data.pt_data->get_or_create_meta_vehicle_journey(trip_update.trip().trip_id(),
                                                                        data.pt_data->get_main_timezone());

    delete_disruption(id, *data.pt_data, *data.meta);
    auto& disruption = holder.make_disruption(id, type::RTLevel::RealTime);
    disruption.reference = disruption.uri;
    if (trip_update.trip().HasExtension(kirin::contributor)) {
        disruption.contributor = trip_update.trip().GetExtension(kirin::contributor);
    }

    disruption.publication_period = data.meta->production_period();
    disruption.created_at = timestamp;
    disruption.updated_at = timestamp;
    // cause
    {  // impact
        auto impact = boost::make_shared<nt::disruption::Impact>();

        // We want the application period to be the span of the
        // execution period of the base and realtime vj:
        //
        // [----------)             base vj
        //               [-------)  rt vj
        // [---------------------)  application period
        const auto base_application_period = base_execution_period(circulation_date, mvj);
        auto begin_app_period = base_application_period.begin();
        auto end_app_period = base_application_period.end();

        impact->uri = disruption.uri;
        impact->created_at = timestamp;
        impact->updated_at = timestamp;
        if (trip_update.trip().HasExtension(kirin::company_id)) {
            impact->company_id = trip_update.trip().GetExtension(kirin::company_id);
        }
        if (trip_update.vehicle().HasExtension(kirin::physical_mode_id)) {
            impact->physical_mode_id = trip_update.vehicle().GetExtension(kirin::physical_mode_id);
        }
        if (trip_update.HasExtension(kirin::trip_message)) {
            impact->messages.push_back(create_message(trip_update.GetExtension(kirin::trip_message)));
        }
        if (trip_update.HasExtension(kirin::headsign)) {
            impact->headsign = trip_update.GetExtension(kirin::headsign);
        }
        if (trip_update.HasExtension(kirin::trip_short_name)) {
            impact->trip_short_name = trip_update.GetExtension(kirin::trip_short_name);
        }
        if (trip_update.trip().HasExtension(kirin::dataset_id)) {
            impact->dataset_id = trip_update.trip().GetExtension(kirin::dataset_id);
        }
        if (trip_update.trip().HasExtension(kirin::network_id)) {
            impact->network_id = trip_update.trip().GetExtension(kirin::network_id);
        }
        if (trip_update.trip().HasExtension(kirin::commercial_mode_id)) {
            impact->commercial_mode_id = trip_update.trip().GetExtension(kirin::commercial_mode_id);
        }
        if (trip_update.trip().HasExtension(kirin::line_id)) {
            impact->line_id = trip_update.trip().GetExtension(kirin::line_id);
        }
        if (trip_update.trip().HasExtension(kirin::route_id)) {
            impact->route_id = trip_update.trip().GetExtension(kirin::route_id);
        }

        // TODO: Effect calculated from stoptime_status -> to be removed later
        // when effect completely implemented in trip_update
        nt::disruption::Effect trip_effect = nt::disruption::Effect::UNKNOWN_EFFECT;

        if (is_cancelled_trip(trip_update)) {
            // TODO: remove this deprecated code (for retrocompatibility with Kirin < 0.8.0 only)
            // Yeah, that's quite hardcoded...
            trip_effect = nt::disruption::Effect::NO_SERVICE;
        } else if (is_circulating_trip(trip_update)) {
            LOG4CPLUS_TRACE(log, "a trip has been changed");
            using nt::disruption::StopTimeUpdate;
            auto most_important_stoptime_status = StopTimeUpdate::Status::UNCHANGED;
            LOG4CPLUS_TRACE(log, "Adding stop time into impact");
            for (const auto& st : trip_update.stop_time_update()) {
                auto it = data.pt_data->stop_points_map.find(st.stop_id());
                if (it == data.pt_data->stop_points_map.cend()) {
                    LOG4CPLUS_WARN(log, "Disruption: " << disruption.uri
                                                       << " cannot be handled, because "
                                                          "stop point: "
                                                       << st.stop_id() << " cannot be found");
                    return nullptr;
                }
                auto* stop_point_ptr = it->second;
                assert(stop_point_ptr);
                uint32_t arrival_time = st.arrival().time();
                uint32_t departure_time = st.departure().time();

                {
                    /*
                     * When st.arrival().has_time() is false, st.arrival().time() will return 0, which, for kraken, is
                     * considered as 00:00(midnight), This is not good. So we must set manually arrival_time to
                     * departure_time.
                     *
                     * Same reasoning for departure_time.
                     *
                     * TODO: And this check should be done on Kirin's side...
                     * */
                    if ((!st.arrival().has_time() || st.arrival().time() == 0) && st.departure().has_time()) {
                        arrival_time = departure_time = st.departure().time();
                    }
                    if ((!st.departure().has_time() || st.departure().time() == 0) && st.arrival().has_time()) {
                        departure_time = arrival_time = st.arrival().time();
                    }
                }
                const auto arrival_pt = bpt::from_time_t(arrival_time);
                const auto departure_pt = bpt::from_time_t(departure_time);
                begin_app_period = std::min(begin_app_period, departure_pt);
                end_app_period = std::max(end_app_period, arrival_pt);
                auto ptime_arrival = arrival_pt - start_first_day_of_impact;
                auto ptime_departure = departure_pt - start_first_day_of_impact;

                type::StopTime stop_time{uint32_t(ptime_arrival.total_seconds()),
                                         uint32_t(ptime_departure.total_seconds()), stop_point_ptr};
                // Don't take into account boarding and alighting duration, we'll get the one from the base stop_time
                // later
                stop_time.boarding_time = stop_time.departure_time;
                stop_time.alighting_time = stop_time.arrival_time;
                std::string message;
                if (st.HasExtension(kirin::stoptime_message)) {
                    message = st.GetExtension(kirin::stoptime_message);
                }

                const auto departure_status = get_status(st.departure(), st);
                const auto arrival_status = get_status(st.arrival(), st);

                // for deleted stoptime departure (resp. arrival), we disable pickup (resp. drop_off)
                // but we keep the departure/arrival to be able to match the stoptime to it's base stoptime
                if (contains({StopTimeUpdate::Status::DELETED, StopTimeUpdate::Status::DELETED_FOR_DETOUR},
                             arrival_status)) {
                    stop_time.set_drop_off_allowed(false);
                } else {
                    stop_time.set_drop_off_allowed(st.arrival().has_time());
                }

                if (contains({StopTimeUpdate::Status::DELETED, StopTimeUpdate::Status::DELETED_FOR_DETOUR},
                             departure_status)) {
                    stop_time.set_pick_up_allowed(false);
                } else {
                    stop_time.set_pick_up_allowed(st.departure().has_time());
                }
                stop_time.set_skipped_stop(false);
                // we update the trip status if the stoptime status is the most important status
                // the most important status is DELAYED then DELETED
                most_important_stoptime_status =
                    std::max({most_important_stoptime_status, departure_status, arrival_status});

                StopTimeUpdate st_update{stop_time, message, departure_status, arrival_status};
                impact->aux_info.stop_times.emplace_back(std::move(st_update));
            }

            // TODO: remove this deprecated code (for retrocompatibility with Kirin < 0.8.0 only)
            trip_effect = get_calculated_trip_effect(most_important_stoptime_status);
        } else {
            LOG4CPLUS_ERROR(log, "unhandled real time message");
        }
        LOG4CPLUS_DEBUG(log, "Trip effect : " << get_wordings(trip_effect));

        if (trip_update.HasExtension(kirin::effect)) {
            trip_effect = get_trip_effect(trip_update.GetExtension(kirin::effect));
        }
        std::string wording = get_wordings(trip_effect);

        end_app_period = std::max(end_app_period, begin_app_period);  // make sure that start <= end
        impact->application_periods.emplace_back(begin_app_period, end_app_period);
        impact->severity = make_severity(id, std::move(wording), trip_effect, timestamp, holder);
        nd::Impact::link_informed_entity(
            nd::make_pt_obj(nt::Type_e::MetaVehicleJourney, trip_update.trip().trip_id(), *data.pt_data), impact,
            data.meta->production_date, nt::RTLevel::RealTime, *data.pt_data);
        // messages
        disruption.add_impact(impact, holder);
    }
    // localization
    // tags
    // note

    LOG4CPLUS_DEBUG(log, "Disruption added");
    return &disruption;
}

void handle_realtime(const std::string& id,
                     const boost::posix_time::ptime& timestamp,
                     const transit_realtime::TripUpdate& trip_update,
                     const type::Data& data,
                     const bool is_realtime_add_enabled,
                     const bool is_realtime_add_trip_enabled) {
    auto log = log4cplus::Logger::getInstance("realtime");
    LOG4CPLUS_TRACE(log, "realtime trip update received");

    if (!is_handleable(trip_update, *data.pt_data, is_realtime_add_enabled, is_realtime_add_trip_enabled)
        || !check_trip_update(trip_update)) {
        LOG4CPLUS_DEBUG(log, "unhandled real time message");
        return;
    }

    bool meta_vj_exists = data.pt_data->meta_vjs.exists(trip_update.trip().trip_id());
    if (!meta_vj_exists && !is_added_trip(trip_update)) {
        LOG4CPLUS_WARN(log, "Cannot perform operation on an unknown Meta VJ (other than adding trip)"
                                << ", trip id: " << trip_update.trip().trip_id() << ", effect: "
                                << get_wordings(get_trip_effect(trip_update.GetExtension(kirin::effect))));
        if (trip_update.stop_time_update_size()) {
            LOG4CPLUS_WARN(log,
                           "Meta VJ 1st stop time departure: " << trip_update.stop_time_update(0).departure().time());
        }
        return;
    }
    if (meta_vj_exists && is_added_trip(trip_update) && base_vj_exists_the_same_day(data, trip_update)) {
        LOG4CPLUS_WARN(log, "cannot add new trip, because trip id corresponds to a base VJ the same day"
                                << ", trip id: " << trip_update.trip().trip_id() << ", effect: "
                                << get_wordings(get_trip_effect(trip_update.GetExtension(kirin::effect))));
        return;
    }

    const auto* disruption = create_disruption(id, timestamp, trip_update, data);

    if (!disruption || !check_disruption(*disruption)) {
        LOG4CPLUS_INFO(
            log, "disruption " << id << " on " << trip_update.trip().trip_id() << " not valid, we do not handle it");
        delete_disruption(id, *data.pt_data, *data.meta);
        return;
    }

    apply_disruption(*disruption, *data.pt_data, *data.meta);
}

}  // namespace navitia
