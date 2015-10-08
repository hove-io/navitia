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

#include <boost/date_time/gregorian/gregorian.hpp>

namespace navitia {

static void cancel_vj(type::MetaVehicleJourney* meta_vj,
               const boost::gregorian::date& date,
               const transit_realtime::TripUpdate& /*trip_update*/,
               const type::Data& data) {
    // we need to cancel all vj of the meta vj
    for (const auto& vect_vj: {meta_vj->base_vj, meta_vj->adapted_vj, meta_vj->real_time_vj}) {
        for (auto* vj: vect_vj) {
            // the train is canceled on one day, we just need to unset its realtime validitypattern

            if (! vj->rt_validity_pattern()->check(date)) { continue; }
            LOG4CPLUS_INFO(log4cplus::Logger::getInstance("realtime"),
                           "canceling the vj " << vj->uri << " on " << date);

            nt::ValidityPattern tmp_vp(*vj->rt_validity_pattern());
            tmp_vp.remove(date);

            vj->validity_patterns[type::RTLevel::RealTime] = data.pt_data->get_or_create_validity_pattern(tmp_vp);
        }
    }
}

void handle_realtime(const transit_realtime::TripUpdate& trip_update, const type::Data& data) {
    auto log = log4cplus::Logger::getInstance("realtime");
    LOG4CPLUS_TRACE(log, "realtime trip update received");
    const auto& trip = trip_update.trip();

    // for the moment we handle only trip cancelation
    if (trip.schedule_relationship() != transit_realtime::TripDescriptor_ScheduleRelationship_CANCELED) {
        LOG4CPLUS_DEBUG(log, "unhandled real time message");
        return;
    }

    auto meta_vj = find_or_default(trip.trip_id(), data.pt_data->meta_vj);
    if (! meta_vj) {
        LOG4CPLUS_INFO(log, "unknown vehicle journey " << trip.trip_id());
        // TODO for trip().ADDED, we'll need to create a new VJ
        return;
    }

    auto circulation_date = boost::gregorian::from_undelimited_string(trip.start_date());

    if (trip.schedule_relationship() == transit_realtime::TripDescriptor_ScheduleRelationship_CANCELED) {
        cancel_vj(meta_vj, circulation_date, trip_update, data);
        return;
    }

}

}
