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

#include "utils/exception.h"
#include "type/type_interfaces.h"
#include "type/rt_level.h"
#include "type/data.h"

#include <boost/optional.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <utility>

namespace navitia {
namespace ptref {

struct ptref_error : public navitia::recoverable_exception {
    std::string more;

    ptref_error(std::string more) : more(std::move(more)) {}
    const char* what() const noexcept override;
};

struct parsing_error : public ptref_error {
    enum error_type { global_error, partial_error, unknown_object };

    error_type type;

    parsing_error(error_type type, const std::string& str) : ptref_error(str), type(type) {}
    parsing_error(const parsing_error&) = default;
    parsing_error& operator=(const parsing_error&) = default;
    ~parsing_error() noexcept override;
};

/// Runs the query on the data, and returns the corresponding indexes.
/// Throws in case of error (parsing, unknown function, no resultst...).
type::Indexes make_query(const type::Type_e requested_type,
                         const std::string& request,
                         const std::vector<std::string>& forbidden_uris,
                         const type::OdtLevel_e odt_level,
                         const boost::optional<boost::posix_time::ptime>& since,
                         const boost::optional<boost::posix_time::ptime>& until,
                         const type::RTLevel rt_level,
                         const type::Data& data,
                         const boost::optional<boost::posix_time::ptime>& current_datetime = boost::none);

type::Indexes make_query(const type::Type_e requested_type,
                         const std::string& request,
                         const std::vector<std::string>& forbidden_uris,
                         const type::OdtLevel_e odt_level,
                         const boost::optional<boost::posix_time::ptime>& since,
                         const boost::optional<boost::posix_time::ptime>& until,
                         const type::Data& data,
                         const boost::optional<boost::posix_time::ptime>& current_datetime = boost::none);

type::Indexes make_query(const type::Type_e requested_type,
                         const std::string& request,
                         const std::vector<std::string>& forbidden_uris,
                         const type::Data& data);

type::Indexes make_query(const type::Type_e requested_type, const std::string& request, const type::Data& data);

}  // namespace ptref
}  // namespace navitia
