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

#include "type_interfaces.h"

#include <boost/bimap.hpp>
#include <boost/bimap/multiset_of.hpp>

namespace navitia {
namespace type {

struct static_data {
private:
    static static_data* instance;

public:
    static static_data* get();
    static boost::posix_time::ptime parse_date_time(const std::string& s);
    static Type_e typeByCaption(const std::string& type_str);
    static std::string captionByType(Type_e type);
    boost::bimap<Type_e, std::string> types_string;
    static Mode_e modeByCaption(const std::string& mode_str);
    boost::bimap<boost::bimaps::multiset_of<Mode_e>, boost::bimaps::set_of<std::string>> modes_string;
};

}  // namespace type

}  // namespace navitia
