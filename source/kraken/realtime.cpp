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

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/make_shared.hpp>
#include <boost/optional.hpp>
#include <chrono>

namespace navitia {

static bool is_handleable(const transit_realtime::TripUpdate& trip_update){
    // Case 1: the trip is cancelled
    if (trip_update.trip().schedule_relationship() == transit_realtime::TripDescriptor_ScheduleRelationship_CANCELED) {
        return true;
    }
    // Case 2: the trip is not cancelled, but modified (ex: the train's arrival is delayed or the train's
    // departure is brought foward), we check the size of stop_time_update because we can't find a proper
    // enum in gtfs-rt proto to express this idea
    if (trip_update.trip().schedule_relationship() == transit_realtime::TripDescriptor_ScheduleRelationship_SCHEDULED
            && trip_update.stop_time_update_size()) {
        return true;
    }
    return false;
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
        for (const auto& st: impact->aux_info.stop_times) {
            if (last_st) {
                if (last_st->departure_time > st.arrival_time) {
                    LOG4CPLUS_WARN(log, "stop time " << *last_st
                                   << " and " << st << " are not correctly ordered");
                    return false;
                }
            } else {
                //we want the first stoptime to be in [0, 24h[
                if (st.departure_time > DateTimeUtils::SECONDS_PER_DAY) {
                    LOG4CPLUS_WARN(log, "stop time " << st << " departure is too late");
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
execution_period(const boost::gregorian::date& date, const nt::MetaVehicleJourney& mvj) {
    auto running_vj = mvj.get_vj_at_date(type::RTLevel::Base, date);
    if (running_vj) {
        return running_vj->execution_period(date);
    }
    // If there is no running vj at this date, we return a null period (begin == end)
    return {boost::posix_time::ptime{date}, boost::posix_time::ptime{date}};
}


static boost::gregorian::date
get_first_day_of_imapct(const nt::disruption::Impact& impact){
    auto first_period = impact.application_periods.at(0);
    return first_period.begin().date();
}

static const type::disruption::Disruption&
create_disruption(const std::string& id,
                  const boost::posix_time::ptime& timestamp,
                  const transit_realtime::TripUpdate& trip_update,
                  const type::Data& data) {
    auto log = log4cplus::Logger::getInstance("realtime");
    LOG4CPLUS_DEBUG(log, "Creating disruption");

    nt::disruption::DisruptionHolder& holder = data.pt_data->disruption_holder;
    auto circulation_date = boost::gregorian::from_undelimited_string(trip_update.trip().start_date());
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
        impact->uri = disruption.uri;
        impact->created_at = timestamp;
        impact->updated_at = timestamp;
        impact->application_periods.push_back(execution_period(circulation_date, mvj));
        std::string wording;
        nt::disruption::Effect effect = nt::disruption::Effect::UNKNOWN_EFFECT;
        if (trip_update.trip().schedule_relationship() == transit_realtime::TripDescriptor_ScheduleRelationship_CANCELED) {
            LOG4CPLUS_TRACE(log, "Disruption has NO_SERVICE effect");
            // Yeah, that's quite hardcodded...
            wording = "trip canceled!";
            effect = nt::disruption::Effect::NO_SERVICE;
        }
        else if (trip_update.trip().schedule_relationship() == transit_realtime::TripDescriptor_ScheduleRelationship_SCHEDULED
                && trip_update.stop_time_update_size()) {
            LOG4CPLUS_TRACE(log, "Disruption has MODIFIED_SERVICE effect");
            // Yeah, that's quite hardcodded...
            wording = "trip modified!";
            effect = nt::disruption::Effect::MODIFIED_SERVICE;
            LOG4CPLUS_TRACE(log, "Adding stop time into impact");
            for (const auto& st: trip_update.stop_time_update()) {
                auto* stop_point_ptr = data.pt_data->stop_points_map[st.stop_id()];
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
                    if ((! st.arrival().has_time() || st.arrival().time() == 0) && st.departure().has_time()){
                        arrival_time = departure_time = st.departure().time();
                    }
                    if ((! st.departure().has_time() || st.departure().time() == 0) && st.arrival().has_time()){
                        departure_time = arrival_time =st.arrival().time();
                    }
                }
                auto first_day_of_impact = get_first_day_of_imapct(*impact);
                auto arrival_seconds = boost::posix_time::from_time_t(arrival_time).time_of_day().total_seconds();
                auto departure_seconds = boost::posix_time::from_time_t(departure_time).time_of_day().total_seconds();
                
                // If the arrival/depature time's date is later than the first day of impact, this means
                // the VJ passes the midnight, it's pickup/dropoff time should add 86400 seconds(one day)
                // Ex:
                // the first day of impact is 1st, Jan, the stoptime's base arrival_time is 23:50 on 1st Jan, 
                // and the new arrival_time is tommorow monrning at 00:10. The new arrival is computed as
                // 10*60 + 86400 seconds.
                auto get_diff_days = [&](uint32_t time){
                    return (boost::posix_time::from_time_t(time).date() - first_day_of_impact).days();
                };

                arrival_seconds +=  get_diff_days(arrival_time) * navitia::DateTimeUtils::SECONDS_PER_DAY;
                departure_seconds += get_diff_days(departure_time) * navitia::DateTimeUtils::SECONDS_PER_DAY;

                type::StopTime stop_time{static_cast<uint32_t>(arrival_seconds),
                    static_cast<uint32_t>(departure_seconds), stop_point_ptr};
                stop_time.set_pick_up_allowed(st.departure().has_time());
                stop_time.set_drop_off_allowed(st.arrival().has_time());
                impact->aux_info.stop_times.emplace_back(std::move(stop_time));
           }
        } else {
            LOG4CPLUS_ERROR(log, "unhandled real time message");
        }
        impact->severity = make_severity(id, std::move(wording), effect, timestamp, holder);
        impact->informed_entities.push_back(
            make_pt_obj(nt::Type_e::MetaVehicleJourney, trip_update.trip().trip_id(), *data.pt_data, impact));
        // messages
        disruption.add_impact(std::move(impact));
    }
    // localization
    // tags
    // note

    LOG4CPLUS_DEBUG(log, "Disruption added");
    return disruption;
}

void handle_realtime(const std::string& id,
                     const boost::posix_time::ptime& timestamp,
                     const transit_realtime::TripUpdate& trip_update,
                     const type::Data& data) {
    auto log = log4cplus::Logger::getInstance("realtime");
    LOG4CPLUS_TRACE(log, "realtime trip update received");

    if (! is_handleable(trip_update)) {
        LOG4CPLUS_DEBUG(log, "unhandled real time message");
        return;
    }

    auto meta_vj = data.pt_data->meta_vjs.get_mut(trip_update.trip().trip_id());
    if (! meta_vj) {
        LOG4CPLUS_ERROR(log, "unknown vehicle journey " << trip_update.trip().trip_id());
        // TODO for trip().ADDED, we'll need to create a new VJ
        return;
    }

    const auto& disruption = create_disruption(id, timestamp, trip_update, data);

    if (! check_disruption(disruption)) {
        LOG4CPLUS_INFO(log, "disruption " << id << " on " << meta_vj->uri << " not valid, we do not handle it");
        delete_disruption(id, *data.pt_data, *data.meta);
        return;
    }

    apply_disruption(disruption, *data.pt_data, *data.meta);
}

}
