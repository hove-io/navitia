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

#include "ptreferential.h"
#include "ptreferential_ng.h"

namespace navitia {
namespace ptref {

const char* ptref_error::what() const noexcept {
    return this->more.c_str();
}
parsing_error::~parsing_error() noexcept = default;

type::Indexes make_query(const type::Type_e requested_type,
                         const std::string& request,
                         const std::vector<std::string>& forbidden_uris,
                         const type::OdtLevel_e odt_level,
                         const boost::optional<boost::posix_time::ptime>& since,
                         const boost::optional<boost::posix_time::ptime>& until,
                         const type::RTLevel rt_level,
                         const type::Data& data,
                         const boost::optional<boost::posix_time::ptime>& current_datetime) {
    const auto indexes = make_query_ng(requested_type, request, forbidden_uris, odt_level, since, until, rt_level, data,
                                       current_datetime);
    if (indexes.empty()) {
        throw ptref_error("Filters: Unable to find object");
    }
    return indexes;
}

type::Indexes make_query(const type::Type_e requested_type,
                         const std::string& request,
                         const std::vector<std::string>& forbidden_uris,
                         const type::OdtLevel_e odt_level,
                         const boost::optional<boost::posix_time::ptime>& since,
                         const boost::optional<boost::posix_time::ptime>& until,
                         const type::Data& data,
                         const boost::optional<boost::posix_time::ptime>& current_datetime) {
    return make_query(requested_type, request, forbidden_uris, odt_level, since, until, type::RTLevel::Base, data,
                      current_datetime);
}

type::Indexes make_query(const type::Type_e requested_type,
                         const std::string& request,
                         const std::vector<std::string>& forbidden_uris,
                         const type::Data& data) {
    return make_query(requested_type, request, forbidden_uris, type::OdtLevel_e::all, boost::none, boost::none, data,
                      boost::none);
}

type::Indexes make_query(const type::Type_e requested_type, const std::string& request, const type::Data& data) {
    const std::vector<std::string> forbidden_uris;
    return make_query(requested_type, request, forbidden_uris, data);
}

}  // namespace ptref
}  // namespace navitia
