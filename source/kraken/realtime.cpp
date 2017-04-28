/* Copyright © 2001-2015, Canal TP and/or its affiliates. All rights reserved.

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
#include "realtime.h"
#include "utils/logger.h"
#include "type/data.h"
#include "type/pt_data.h"
#include "type/meta_data.h"
#include "type/datetime.h"
#include "kraken/apply_disruption.h"
#include "type/kirin.pb.h"

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/make_shared.hpp>
#include <boost/optional.hpp>
#include <chrono>

namespace navitia {

namespace nd = type::disruption;

static bool is_handleable(const transit_realtime::TripUpdate& trip_update){
    namespace bpt = boost::posix_time;

    auto log = log4cplus::Logger::getInstance("realtime");
    // Case 1: the trip is cancelled
    if (trip_update.trip().schedule_relationship() == transit_realtime::TripDescriptor_ScheduleRelationship_CANCELED) {
        return true;
    }
    // Case 2: the trip is not cancelled, but modified (ex: the train's arrival is delayed or the train's
    // departure is brought forward), we check the size of stop_time_update because we can't find a proper
    // enum in gtfs-rt proto to express this idea
    if (trip_update.trip().schedule_relationship() == transit_realtime::TripDescriptor_ScheduleRelationship_SCHEDULED
            && trip_update.stop_time_update_size()) {
        auto start_date = boost::gregorian::from_undelimited_string(trip_update.trip().start_date());
        auto start_first_day_of_impact = bpt::ptime(start_date, bpt::time_duration(0, 0, 0, 0));
        for (const auto& st: trip_update.stop_time_update()) {
            auto ptime_arrival = bpt::from_time_t(st.arrival().time());
            auto ptime_departure = bpt::from_time_t(st.departure().time());
            if (ptime_arrival < start_first_day_of_impact
                    || ptime_departure < start_first_day_of_impact) {
                LOG4CPLUS_WARN(log, "Trip Update " << trip_update.trip().trip_id() << ": Stop time "
                                    << st.stop_id() << " is before the day of impact");
                return false;
            }
        }
        return true;
    }
    return false;
}

static bool check_trip_update(const transit_realtime::TripUpdate& trip_update) {
    auto log = log4cplus::Logger::getInstance("realtime");
    if (trip_update.trip().schedule_relationship() ==
                                transit_realtime::TripDescriptor_ScheduleRelationship_SCHEDULED
            && trip_update.stop_time_update_size()) {
        uint32_t last_st_dep = std::numeric_limits<uint32_t>::max();
        for (const auto& st: trip_update.stop_time_update()) {
            uint32_t arrival_time = st.arrival().time();
            uint32_t departure_time = st.departure().time();
            if (last_st_dep != std::numeric_limits<uint32_t>::max()
                    && last_st_dep > arrival_time) {
                LOG4CPLUS_WARN(log, "Trip Update " << trip_update.trip().trip_id() << ": Stop time "
                                    << st.stop_id() << " is not correctly ordered");
                return false;
            }
            if (arrival_time > departure_time) {
                LOG4CPLUS_WARN(log, "Trip Update " << trip_update.trip().trip_id() << ": For the Stop Time "
                                    << st.stop_id() << " departure is before the arrival");
                return false;
            }
            last_st_dep = departure_time;
        }
    }
    return true;
}

static std::ostream& operator<<(std::ostream& s, const nt::StopTime& st) {
    // Note: do not use st.order, as the stoptime might not yet be part of the VJ
    return s << "ST " << (st.vehicle_journey ? st.vehicle_journey->uri : "Null")
             << " dep " << st.departure_time << " arr " << st.arrival_time;
}

/**
  * Check if a disruption is valid
  *
  * We check that all stop times are correctly ordered
  * And that the departure/arrival are correct too
 */
static bool check_disruption(const nt::disruption::Disruption& disruption) {
    auto log = log4cplus::Logger::getInstance("realtime");
    for (const auto& impact: disruption.get_impacts()) {
        boost::optional<const nt::StopTime&> last_st;
        for (const auto& stu: impact->aux_info.stop_times) {
            const auto& st = stu.stop_time;
            if (last_st) {
                if (last_st->departure_time > st.arrival_time) {
                    LOG4CPLUS_WARN(log, "stop time " << *last_st
                                   << " and " << st << " are not correctly ordered");
                    return false;
                }
            }
            if (st.departure_time < st.arrival_time) {
                LOG4CPLUS_WARN(log, "For the st " << st << " departure is before the arrival");
                return false;
            }
            last_st = st;
        }
    }
    return true;
}

static boost::shared_ptr<nt::disruption::Severity>
make_severity(const std::string& id,
              std::string wording,
              nt::disruption::Effect effect,
              const boost::posix_time::ptime& timestamp,
              nt::disruption::DisruptionHolder& holder) {
    auto& weak_severity = holder.severities[id];
    if (auto severity = weak_severity.lock()) { return severity; }

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

static boost::posix_time::time_period
base_execution_period(const boost::gregorian::date& date, const nt::MetaVehicleJourney& mvj) {
    auto running_vj = mvj.get_base_vj_circulating_at_date(date);
    if (running_vj) {
        return running_vj->execution_period(date);
    }
    // If there is no running vj at this date, we return a null period (begin == end)
    return {boost::posix_time::ptime{date}, boost::posix_time::ptime{date}};
}

static type::disruption::Message create_message(const std::string& str_msg) {
    type::disruption::Message msg;
    msg.text = str_msg;
    msg.channel_id = "rt";
    msg.channel_name = "rt";
    msg.channel_types = {nd::ChannelType::web, nd::ChannelType::mobile};

    return msg;
}

static nt::disruption::StopTimeUpdate::Status get_status(const transit_realtime::TripUpdate_StopTimeEvent& event,
        const transit_realtime::TripUpdate_StopTimeUpdate& st) {
    auto get_relationship = [](const transit_realtime::TripUpdate_StopTimeEvent& event,
            const transit_realtime::TripUpdate_StopTimeUpdate& st) {
        // we get either the stop_time_event if we have it or we use the GTFS-RT standard (on st)
        if (event.HasExtension(kirin::stop_time_event_relationship)) {
            return event.GetExtension(kirin::stop_time_event_relationship);
        }
        return st.schedule_relationship();
    };

    if (get_relationship(event, st) ==
            transit_realtime::TripUpdate_StopTimeUpdate_ScheduleRelationship_SKIPPED) {
        return nt::disruption::StopTimeUpdate::Status::DELETED;
    } else if (! event.has_delay() // for retrocompatibility since the old kirin version
                                   // was not giving delays
               || event.delay() != 0) {
        return nt::disruption::StopTimeUpdate::Status::DELAYED;
    } else {
        return nt::disruption::StopTimeUpdate::Status::UNCHANGED;
    }
}

static const type::disruption::Disruption*
create_disruption(const std::string& id,
                  const boost::posix_time::ptime& timestamp,
                  const transit_realtime::TripUpdate& trip_update,
                  const type::Data& data) {
    namespace bpt = boost::posix_time;

    auto log = log4cplus::Logger::getInstance("realtime");
    LOG4CPLUS_DEBUG(log, "Creating disruption");

    nt::disruption::DisruptionHolder& holder = data.pt_data->disruption_holder;
    auto circulation_date = boost::gregorian::from_undelimited_string(trip_update.trip().start_date());
    auto start_first_day_of_impact = bpt::ptime(circulation_date, bpt::time_duration(0, 0, 0, 0));
    const auto& mvj = *data.pt_data->meta_vjs.get_mut(trip_update.trip().trip_id());

    delete_disruption(id, *data.pt_data, *data.meta);
    auto& disruption = holder.make_disruption(id, type::RTLevel::RealTime);
    disruption.reference = disruption.uri;
    disruption.publication_period = data.meta->production_period();
    disruption.created_at = timestamp;
    disruption.updated_at = timestamp;
    // cause
    {// impact
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
        if (trip_update.HasExtension(kirin::trip_message)) {
            impact->messages.push_back(create_message(trip_update.GetExtension(kirin::trip_message)));
        }
        std::string wording;
        nt::disruption::Effect trip_effect = nt::disruption::Effect::UNKNOWN_EFFECT;
        if (trip_update.trip().schedule_relationship() == transit_realtime::TripDescriptor_ScheduleRelationship_CANCELED) {
            LOG4CPLUS_TRACE(log, "Disruption has NO_SERVICE effect");
            // Yeah, that's quite hardcodded...
            wording = "trip canceled";
            trip_effect = nt::disruption::Effect::NO_SERVICE;
        }
        else if (trip_update.trip().schedule_relationship() == transit_realtime::TripDescriptor_ScheduleRelationship_SCHEDULED
                && trip_update.stop_time_update_size()) {
            LOG4CPLUS_TRACE(log, "a trip has been changed");
            using nt::disruption::StopTimeUpdate;
            auto most_important_stoptime_status = StopTimeUpdate::Status::UNCHANGED;
            LOG4CPLUS_TRACE(log, "Adding stop time into impact");
            for (const auto& st: trip_update.stop_time_update()) {
                auto it = data.pt_data->stop_points_map.find(st.stop_id());
                if (it == data.pt_data->stop_points_map.cend()) {
                    LOG4CPLUS_WARN(log, "Disruption: " << disruption.uri << " cannot be handled, because "
                            "stop point: " << st.stop_id() << " cannot be found");
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
                    if ((! st.arrival().has_time() || st.arrival().time() == 0) && st.departure().has_time()) {
                        arrival_time = departure_time = st.departure().time();
                    }
                    if ((! st.departure().has_time() || st.departure().time() == 0) && st.arrival().has_time()) {
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
                                         uint32_t(ptime_departure.total_seconds()),
                                         stop_point_ptr};
                // Don't take into account boarding and alighting duration, we'll get the one from the base stop_time later
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
                if (departure_status == StopTimeUpdate::Status::DELETED) {
                    stop_time.set_drop_off_allowed(false);
                } else {
                    stop_time.set_drop_off_allowed(st.arrival().has_time());
                }

                if (arrival_status == StopTimeUpdate::Status::DELETED) {
                    stop_time.set_pick_up_allowed(false);
                } else {
                    stop_time.set_pick_up_allowed(st.departure().has_time());
                }
                // we update the trip status if the stoptime status is the most important status
                // the most important status is DELAYED then DELETED
                most_important_stoptime_status = std::max(most_important_stoptime_status, departure_status);
                most_important_stoptime_status = std::max(most_important_stoptime_status, arrival_status);

                StopTimeUpdate st_update{std::move(stop_time), message, departure_status, arrival_status};
                impact->aux_info.stop_times.emplace_back(std::move(st_update));
            }

            switch (most_important_stoptime_status) {
            case StopTimeUpdate::Status::DELAYED:
            case StopTimeUpdate::Status::UNCHANGED: // it can be a back to normal case
                trip_effect = nt::disruption::Effect::SIGNIFICANT_DELAYS; break;
            case StopTimeUpdate::Status::DELETED:
                trip_effect = nt::disruption::Effect::DETOUR; break;
            default:
                trip_effect = nt::disruption::Effect::UNKNOWN_EFFECT; break;
            }
        } else {
            LOG4CPLUS_ERROR(log, "unhandled real time message");
        }
        if (wording.empty()) {
            if ( trip_effect == nt::disruption::Effect::SIGNIFICANT_DELAYS) {
                wording = "trip delayed";
            } else if (trip_effect == nt::disruption::Effect::DETOUR) {
                wording = "trip modified";
            }
        }
        impact->application_periods.push_back({begin_app_period, end_app_period});
        impact->severity = make_severity(id, std::move(wording), trip_effect, timestamp, holder);
        nd::Impact::link_informed_entity(
                    nd::make_pt_obj(nt::Type_e::MetaVehicleJourney, trip_update.trip().trip_id(), *data.pt_data),
                    impact, data.meta->production_date, nt::RTLevel::RealTime);
        // messages
        disruption.add_impact(std::move(impact), holder);
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
                     const type::Data& data) {
    auto log = log4cplus::Logger::getInstance("realtime");
    LOG4CPLUS_TRACE(log, "realtime trip update received");

    if (! is_handleable(trip_update)
            || ! check_trip_update(trip_update)) {
        LOG4CPLUS_DEBUG(log, "unhandled real time message");
        return;
    }

    auto meta_vj = data.pt_data->meta_vjs.get_mut(trip_update.trip().trip_id());
    if (! meta_vj) {
        LOG4CPLUS_ERROR(log, "unknown vehicle journey " << trip_update.trip().trip_id());
        // TODO for trip().ADDED, we'll need to create a new VJ
        return;
    }

    const auto* disruption = create_disruption(id, timestamp, trip_update, data);

    if (! disruption || ! check_disruption(*disruption)) {
        LOG4CPLUS_INFO(log, "disruption " << id << " on " << meta_vj->uri << " not valid, we do not handle it");
        delete_disruption(id, *data.pt_data, *data.meta);
        return;
    }

    apply_disruption(*disruption, *data.pt_data, *data.meta);
}

}
