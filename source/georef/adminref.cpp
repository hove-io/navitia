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

#include "adminref.h"
#include <boost/algorithm/string/join.hpp>

namespace navitia { namespace georef {
std::string Admin::get_range_postal_codes(){
    std::string post_code;
    // the label is the range of the postcode
    // ex: Tours (37000;37100;37200) -> Tours (37000-37200)
    if (!this->postal_codes.empty()) {
        int min_value = std::numeric_limits<int>::max();
        int max_value = std::numeric_limits<int>::min();
        try {
            for (const std::string& str_post_code : this->postal_codes) {
                int int_post_code;
                int_post_code = boost::lexical_cast<int>(str_post_code);
                min_value = std::min(min_value, int_post_code);
                max_value = std::max(max_value, int_post_code);
            }
            if (min_value == max_value){
                post_code = boost::lexical_cast<std::string>(min_value);
            }else{
                post_code = boost::lexical_cast<std::string>(min_value)
                        + "-" + boost::lexical_cast<std::string>(max_value);
            }
        }catch (boost::bad_lexical_cast) {
            post_code = this->postal_codes_to_string();
        }
    }
    return post_code;
}

std::string Admin::postal_codes_to_string() const{
    return boost::algorithm::join(this->postal_codes, ";");
}
}}
