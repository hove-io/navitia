/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.
  
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

#pragma once

#include "type/pt_data.h"
#include "type/message.h"
#include "type/chaos.pb.h"
#include "type/gtfs-realtime.pb.h"

namespace navitia {

boost::shared_ptr<navitia::type::new_disruption::Tag>
make_tag(const chaos::Tag& chaos_tag,
         navitia::type::new_disruption::DisruptionHolder& holder);

boost::shared_ptr<navitia::type::new_disruption::Cause>
make_cause(const chaos::Cause& chaos_cause,
           navitia::type::new_disruption::DisruptionHolder& holder);

boost::shared_ptr<navitia::type::new_disruption::Severity>
make_severity(const chaos::Severity& chaos_severity,
              navitia::type::new_disruption::DisruptionHolder& holder);

std::vector<navitia::type::new_disruption::PtObject>
make_pt_objects(const google::protobuf::RepeatedPtrField<chaos::PtObject>& chaos_pt_objects);

std::vector<navitia::type::new_disruption::PtObject>
make_pt_objects(const google::protobuf::RepeatedPtrField<transit_realtime::EntitySelector>& chaos_pt_objects);

boost::shared_ptr<navitia::type::new_disruption::Impact>
make_impact(const chaos::Impact& chaos_impact,
            navitia::type::new_disruption::DisruptionHolder& holder);

void add_disruption(navitia::type::PT_Data& pt_data,
                    const chaos::Disruption& chaos_disruption);

} // namespace navitia
