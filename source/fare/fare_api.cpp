/* Copyright © 2001-2023, Hove and/or its affiliates. All rights reserved.

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

#include "fare_api.h"
#include "fare.h"

#include "routing/routing.h"
#include "type/datetime.h"
#include "type/pb_converter.h"

namespace navitia {
namespace fare {

void fill_fares(PbCreator& pb_creator, const pbnavitia::PtFaresRequest& fares) {
    for (const pbnavitia::PtFaresRequest::PtJourney& pt_journey : fares.pt_journeys()) {
        auto tickets = pb_creator.data->fare->compute_fare(pt_journey, *pb_creator.data);
        if (tickets.not_found) {
            continue;
        }
        pbnavitia::PtJourneyFare* pb_journey_fare = pb_creator.add_pt_journey_fares();
        pb_journey_fare->set_journey_id(pt_journey.id());

        pb_creator.fill_fare(pb_journey_fare->mutable_fare(), nullptr, tickets);
    }
}
}  // namespace fare
}  // namespace navitia
