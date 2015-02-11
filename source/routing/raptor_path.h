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
#include "type/type.h"
#include "routing/routing.h"
#include "routing/raptor_utils.h"
#include <vector>
namespace navitia { namespace routing {
    struct RAPTOR;

    ///Construit un chemin
    Path makePath(SpIdx destination_idx, size_t countb, bool clockwise,
            bool disruption_active, const type::AccessibiliteParams & accessibilite_params,
            const RAPTOR &raptor_);


    ///Construit tous chemins trouvés
    std::vector<Path> 
    makePathes(const std::vector<std::pair<SpIdx, navitia::time_duration> > &departures,
            const std::vector<std::pair<SpIdx, navitia::time_duration> > &destinations,
            const type::AccessibiliteParams & accessibilite_params,
            const RAPTOR &raptor_, bool clockwise, bool disruption_active);

    /// Ajuste les temps d’attente
    void patch_datetimes(Path &path);

    std::pair<const type::StopTime*, uint32_t>
    get_current_stidx_gap(size_t count,
                          SpIdx stop_point,
                          const std::vector<Labels> &labels,
                          const type::AccessibiliteParams & accessibilite_params,
                          bool clockwise,
                          const RAPTOR &raptor,
                          bool disruption_active);

    struct BasePathVisitor {
        void connection(const type::StopPoint* /*departure*/, const type::StopPoint* /*destination*/,
                    boost::posix_time::ptime /*dep_time*/, boost::posix_time::ptime /*arr_time*/,
                    const type::StopPointConnection* /*stop_point_connection*/) {
            return ;
        }

        void init_vj() {
            return ;
        }

        void loop_vj(const type::StopTime* /*st*/, boost::posix_time::ptime /*departure*/, boost::posix_time::ptime /*arrival*/) {
            return ;
        }

        void change_vj(const type::StopTime* /*prev_st*/, const type::StopTime* /*current_st*/,
                       boost::posix_time::ptime /*prev_dt*/, boost::posix_time::ptime /*current_dt*/,
                       bool /*clockwise*/) {
            return ;
        }

        void finish_vj(bool /*clockwise*/) {
            return ;
        }

        void final_step(SpIdx /*current_jpp*/, size_t /*count*/, const std::vector<Labels> &/*labels*/) {
            return ;
        }
    };
}}
