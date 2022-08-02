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

#include "type/pt_data.h"
#include "type/message.h"
#include "type/chaos.pb.h"
#include "type/meta_data.h"

#include <memory>

namespace navitia {

/*
 * Create a disruption from the chaos protobuf
 *
 * The disruption is registered in pt_data
 */
const type::disruption::Disruption& make_disruption(const chaos::Disruption& chaos_disruption, type::PT_Data& pt_data);

/*
 * Create a disruption from the chaos protobuf and apply it on the pt_data
 */
void make_and_apply_disruption(const chaos::Disruption& chaos_disruption,
                               type::PT_Data& pt_data,
                               const type::MetaData& meta);

boost::optional<type::disruption::LineSection> make_line_section(const chaos::PtObject& chaos_section,
                                                                 type::PT_Data& pt_data);
boost::optional<type::disruption::RailSection> make_rail_section(const chaos::PtObject& chaos_section,
                                                                 const type::PT_Data& pt_data);

bool is_publishable(const transit_realtime::TimeRange& publication_period,
                    boost::posix_time::time_period production_period);

}  // namespace navitia
