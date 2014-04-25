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
#include <vector>
namespace navitia { namespace routing {
    class RAPTOR;
     ///Construit un chemin, utilisé lorsque l'algorithme a été fait en sens anti-horaire
    Path makePathreverse(unsigned int destination_idx, unsigned int countb,
            const type::AccessibiliteParams & accessibilite_params,
            const RAPTOR &raptor_, bool disruption_active);

    ///Construit un chemin
    Path makePath(type::idx_t destination_idx, unsigned int countb, bool clockwise,
            bool disruption_active, const type::AccessibiliteParams & accessibilite_params,
            const RAPTOR &raptor_);


    ///Construit tous chemins trouvés
    std::vector<Path> 
    makePathes(const std::vector<std::pair<type::idx_t, boost::posix_time::time_duration> > &departures,
            const std::vector<std::pair<type::idx_t, boost::posix_time::time_duration> > &destinations,
            const type::AccessibiliteParams & accessibilite_params,
            const RAPTOR &raptor_, bool clockwise, bool disruption_active);

    /// Ajuste les temps d’attente
    void patch_datetimes(Path &path);
}}
