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

#pragma once

#include "type/data.h"
#include "type/rt_level.h"
#include "type/dataset.h"
#include "type/contributor.h"
#include "type/calendar.h"
#include "type/base_pt_objects.h"
#include "type/network.h"
#include "type/company.h"
#include "type/commercial_mode.h"
#include "type/physical_mode.h"
#include "type/meta_vehicle_journey.h"

namespace navitia {
namespace ptref {

type::Indexes get_difference(const type::Indexes& idxs1, const type::Indexes& idxs2);
type::Indexes get_intersection(const type::Indexes& idxs1, const type::Indexes& idxs2);
type::Indexes get_corresponding(type::Indexes indexes,
                                type::Type_e from,
                                const type::Type_e to,
                                const type::Data& data);
type::Type_e type_by_caption(const std::string& type);
type::Indexes get_indexes_by_impacts(const type::Type_e& type_e, const type::Data& d, bool only_no_service = true);
type::Indexes get_impacts_by_tags(const std::vector<std::string>& tag_name, const type::Data& d);
type::Indexes filter_vj_active_at(const type::Indexes& indexes,
                                  const boost::posix_time::ptime current_datetime,
                                  const type::RTLevel rt_level,
                                  const type::Data& data);
type::Indexes filter_on_period(const type::Indexes& indexes,
                               const type::Type_e requested_type,
                               const boost::optional<boost::posix_time::ptime>& since,
                               const boost::optional<boost::posix_time::ptime>& until,
                               const type::RTLevel rt_level,
                               const type::Data& data);
type::Indexes get_within(const type::Type_e type,
                         const type::GeographicalCoord& coord,
                         const double distance,
                         const type::Data& data);
type::Indexes get_indexes_from_code(const type::Type_e type,
                                    const std::string& key,
                                    const std::string& value,
                                    const type::Data& data);
type::Indexes get_indexes_from_id(const type::Type_e type, const std::string& id, const type::Data& data);
type::Indexes get_indexes_from_name(const type::Type_e type, const std::string& name, const type::Data& data);
type::Indexes get_indexes_from_code_type(const type::Type_e type,
                                         const std::vector<std::string>& keys,
                                         const type::Data& data);
type::Indexes get_indexes_from_route_direction_type(const std::vector<std::string>& keys, const type::Data& data);

}  // namespace ptref
}  // namespace navitia
