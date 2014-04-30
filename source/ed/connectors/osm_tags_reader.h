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

#include <bitset>
#include <map>

namespace ed { namespace connectors {

constexpr uint8_t CYCLE_FWD = 0;
constexpr uint8_t CYCLE_BWD = 1;
constexpr uint8_t CAR_FWD = 2;
constexpr uint8_t CAR_BWD = 3;
constexpr uint8_t FOOT_FWD = 4;
constexpr uint8_t FOOT_BWD = 5;

/// Retourne les propriétés quant aux modes et sens qui peuvent utiliser le way
std::bitset<8> parse_way_tags(const std::map<std::string, std::string> & tags);

}}
