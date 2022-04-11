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

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

#include <string>

namespace navitia {
namespace type {
class Data;
}  // namespace type
}  // namespace navitia

namespace ed {

template <class T = navitia::type::Data>
bool try_save_file(const std::string& filename, const T& data) {
    auto logger = log4cplus::Logger::getInstance("ed2nav::try_save_file");
    LOG4CPLUS_INFO(logger, "Trying to save " << filename);
    try {
        data.save(filename);
    } catch (const navitia::exception& e) {
        LOG4CPLUS_ERROR(logger, "Unable to save " << filename);
        LOG4CPLUS_ERROR(logger, e.what());
        return false;
    }
    LOG4CPLUS_INFO(logger, "File " << filename << " has been saved");
    return true;
}

template <class T = navitia::type::Data>
bool write_data_to_file(const std::string& output_filename, const T& data);
int ed2nav(int argc, const char** argv);

}  // namespace ed
