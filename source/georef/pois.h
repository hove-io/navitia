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

namespace nt = navitia::type;

namespace navitia {
    namespace georef {
        // la liste des pois à charger
        struct Pois{
            std::string poi_key;
            std::string vls;
                    // Key,          Value
            std::map<std::string, std::string> PoiList;
            Pois():poi_key("amenity"), vls("bicycle_rental"){
                PoiList["college"] = "école";
                PoiList["university"] = "université";
                PoiList["theatre"] = "théâtre";
                PoiList["hospital"] = "hôpital";
                PoiList["post_office"] = "bureau de poste";
                PoiList["bicycle_rental"] = "VLS";
            }
        };
}
}

