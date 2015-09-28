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

void cancel_vj(type::VehicleJourney* theoric_vj,
               const boost::gregorian::date& date,
               const transit_realtime::TripUpdate& /*trip_update*/,
               const type::Data& data) {

    // the train is cancelled on one day, we just need to unset its realtime validitypattern
    LOG4CPLUS_INFO(log4cplus::Logger::getInstance("realtime"),
                   "canceling the vj " << theoric_vj->uri << " on " << date);

    nt::ValidityPattern tmp_vp(*theoric_vj->rt_validity_pattern());
    tmp_vp.remove(date);

    theoric_vj->validity_patterns[type::RTLevel::RealTime] = data.pt_data->get_or_create_validity_pattern(tmp_vp);
}

void handle_realtime(const transit_realtime::TripUpdate& trip_update, const type::Data& data) {
    auto log = log4cplus::Logger::getInstance("realtime");
    LOG4CPLUS_TRACE(log, "realtime trip update received");
    const auto& trip = trip_update.trip();

    // for the moment we handle only trip cancelation
    if (! trip.CANCELED) {
        LOG4CPLUS_DEBUG(log, "unhandled real time message");
        return;
    }

    auto vj = find_or_default(trip.trip_id(), data.pt_data->vehicle_journeys_map);
    if (! vj) {
        LOG4CPLUS_INFO(log, "unknown vehicle journey " << trip.trip_id());
        // TODO for trip().ADDED, we'll need to create a new VJ
        return;
    }

    auto circulation_date = boost::gregorian::from_undelimited_string(trip.start_date());

    if (trip.CANCELED) {
        cancel_vj(vj, circulation_date, trip_update, data);
        return;
    }

}

}
