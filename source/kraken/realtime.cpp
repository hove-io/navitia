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
#include "kraken/apply_disruption.h"

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/make_shared.hpp>
#include <boost/optional.hpp>

namespace navitia {

static bool is_handleable(const transit_realtime::TripUpdate& trip_update){
    // Case 1: the trip is cancelled
    if (trip_update.trip().schedule_relationship() == transit_realtime::TripDescriptor_ScheduleRelationship_CANCELED) {
        return true;
    }
    // Case 2: the trip is not cancelled, but modified (ex: the train's arrival is delayed or the train's departure is
    // brought foward), we check the size of stop_time_update because we can't find a proper enum in gtfs-rt proto to
    // express this idea
    if (trip_update.trip().schedule_relationship() == transit_realtime::TripDescriptor_ScheduleRelationship_SCHEDULED &&
            trip_update.stop_time_update_size()) {
        return true;
    }
    return false;
}


static boost::shared_ptr<nt::disruption::Severity>
make_severity(const std::string& id,
              std::string wording,
              nt::disruption::Effect effect,
              const boost::posix_time::ptime& timestamp,
              nt::disruption::DisruptionHolder& holder) {
    // Yeah, that's quite hardcodded...
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


static const type::disruption::Disruption&
create_disruption(const std::string& id,
                  const boost::posix_time::ptime& timestamp,
                  const transit_realtime::TripUpdate& trip_update,
                  const type::Data& data) {
    auto log = log4cplus::Logger::getInstance("realtime");
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
        for (const auto& st: trip_update.stop_time_update()) {
            auto stop_point_ptr = data.pt_data->stop_points_map[st.stop_id()];
            impact->aux_info.stop_times.emplace_back(st.arrival().time(), st.departure().time(), stop_point_ptr);
       }
        std::string wording;
        nt::disruption::Effect effect;
        if (trip_update.trip().schedule_relationship() == transit_realtime::TripDescriptor_ScheduleRelationship_CANCELED) {
            wording = "trip canceled!";
            effect = nt::disruption::Effect::NO_SERVICE;
        }
        else if (trip_update.trip().schedule_relationship() == transit_realtime::TripDescriptor_ScheduleRelationship_SCHEDULED
                && trip_update.stop_time_update_size()) {
            wording = "trip modified!";
            effect = nt::disruption::Effect::MODIFIED_SERVICE;
        }else {
            LOG4CPLUS_ERROR(log, "unhandled real time message");
            throw navitia::exception("Effect of disruption is unknown");
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
        LOG4CPLUS_INFO(log, "unknown vehicle journey " << trip_update.trip().trip_id());
        // TODO for trip().ADDED, we'll need to create a new VJ
        return;
    }

    const auto& disruption = create_disruption(id, timestamp, trip_update, data);

    apply_disruption(disruption, *data.pt_data, *data.meta);
}

}
