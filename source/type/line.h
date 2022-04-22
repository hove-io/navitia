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
#include "type/type_interfaces.h"
#include "type/fwd_type.h"
#include "type/geographical_coord.h"
#include "type/odt_properties.h"

#include <boost/optional.hpp>

namespace navitia {
namespace type {

struct Line : public Header, Nameable, HasMessages {
    const static Type_e type = Type_e::Line;
    std::string code;
    std::string forward_name;
    std::string backward_name;

    std::string additional_data;
    std::string color;
    std::string text_color;
    int sort = std::numeric_limits<int>::max();

    CommercialMode* commercial_mode = nullptr;

    std::vector<Company*> company_list;
    Network* network = nullptr;

    std::vector<Route*> route_list;
    std::vector<PhysicalMode*> physical_mode_list;
    std::vector<Calendar*> calendar_list;
    MultiLineString shape;
    boost::optional<boost::posix_time::time_duration> opening_time, closing_time;

    std::map<std::string, std::string> properties;

    std::vector<LineGroup*> line_group_list;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int);
    Indexes get(Type_e type, const PT_Data& data) const;
    bool operator<(const Line& other) const;
    type::hasOdtProperties get_odt_properties() const;
    std::string get_label() const;
};

struct LineGroup : public Header, Nameable {
    const static Type_e type = Type_e::LineGroup;

    Line* main_line;
    std::vector<Line*> line_list;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int);
    Indexes get(Type_e type, const PT_Data& data) const;
    bool operator<(const LineGroup& other) const { return this < &other; }
};
}  // namespace type
}  // namespace navitia
