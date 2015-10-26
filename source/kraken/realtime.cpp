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

static boost::shared_ptr<nt::disruption::Severity>
make_no_service_severity(const boost::posix_time::ptime& timestamp,
                         nt::disruption::DisruptionHolder& holder) {
    // Yeah, that's quite hardcodded...
    const std::string id = "kraken:rt:no_service";
    auto& weak_severity = holder.severities[id];
    if (auto severity = weak_severity.lock()) { return severity; }

    auto severity = boost::make_shared<nt::disruption::Severity>();
    severity->uri = id;
    severity->wording = "trip canceled!";
    severity->created_at = timestamp;
    severity->updated_at = timestamp;
    severity->color = "#000000";
    severity->priority = 42;
    severity->effect = nt::disruption::Effect::NO_SERVICE;

    weak_severity = severity;
    return severity;
}

static boost::posix_time::time_period
execution_period(const boost::gregorian::date& date, const nt::MetaVehicleJourney& mvj) {
    auto running_vj = mvj.get_vj_at_date(type::RTLevel::Base, date);
    if (running_vj) {
        return execution_period(date, *running_vj);
    }
    // should be dead code
    return execution_period(date, *mvj.get_first_vj_at(type::RTLevel::Base));
}


static void
create_disruption(const std::string& id,
                  const boost::posix_time::ptime& timestamp,
                  const transit_realtime::TripUpdate& trip_update,
                  const type::Data& data) {
    auto log = log4cplus::Logger::getInstance("realtime");
    nt::disruption::DisruptionHolder &holder = data.pt_data->disruption_holder;
    auto circulation_date = boost::gregorian::from_undelimited_string(trip_update.trip().start_date());
    const auto& mvj = *data.pt_data->meta_vjs.get_mut(trip_update.trip().trip_id());

    delete_disruption(id, *data.pt_data, *data.meta);
    auto disruption = std::make_unique<nt::disruption::Disruption>();
    disruption->uri = id;
    disruption->reference = disruption->uri;
    disruption->publication_period = data.meta->production_period();
    disruption->created_at = timestamp;
    disruption->updated_at = timestamp;
    // cause
    {// impact
        auto impact = boost::make_shared<nt::disruption::Impact>();
        impact->uri = disruption->uri;
        impact->created_at = timestamp;
        impact->updated_at = timestamp;
        impact->application_periods.push_back(execution_period(circulation_date, mvj));
        impact->severity = make_no_service_severity(timestamp, holder);
        impact->informed_entities.push_back(
            make_pt_obj(nt::Type_e::MetaVehicleJourney, trip_update.trip().trip_id(), *data.pt_data, impact));
        // messages
        disruption->add_impact(std::move(impact));
    }
    // localization
    // tags
    // note

    holder.disruptions.push_back(std::move(disruption));
    LOG4CPLUS_DEBUG(log, "Disruption added");
}

void handle_realtime(const std::string& id,
                     const boost::posix_time::ptime& timestamp,
                     const transit_realtime::TripUpdate& trip_update,
                     const type::Data& data) {
    auto log = log4cplus::Logger::getInstance("realtime");
    LOG4CPLUS_TRACE(log, "realtime trip update received");
    const auto& trip = trip_update.trip();

    // for the moment we handle only trip cancelation
    if (trip.schedule_relationship() != transit_realtime::TripDescriptor_ScheduleRelationship_CANCELED) {
        LOG4CPLUS_DEBUG(log, "unhandled real time message");
        return;
    }

    auto meta_vj = data.pt_data->meta_vjs.get_mut(trip.trip_id());
    if (! meta_vj) {
        LOG4CPLUS_INFO(log, "unknown vehicle journey " << trip.trip_id());
        // TODO for trip().ADDED, we'll need to create a new VJ
        return;
    }

    create_disruption(id, timestamp, trip_update, data);

    auto circulation_date = boost::gregorian::from_undelimited_string(trip.start_date());

    if (trip.schedule_relationship() == transit_realtime::TripDescriptor_ScheduleRelationship_CANCELED) {
        meta_vj->cancel_vj(type::RTLevel::RealTime, {circulation_date}, data);
        return;
    }

}

}
