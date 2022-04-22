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
#include <utils/logger.h>
#include <boost/filesystem.hpp>
#include "utils/csv.h"
#include "utils/functions.h"
#include "ed/data.h"
#include "ed/connectors/conv_coord.h"

namespace ed {
namespace connectors {
struct GeopalParserException : public navitia::exception {
    GeopalParserException(const std::string& message) : navitia::exception(message) {}
    GeopalParserException(const GeopalParserException&) = default;
    GeopalParserException& operator=(const GeopalParserException&) = default;
    ~GeopalParserException() noexcept override;
};

class GeopalParser {
private:
    std::string path;  ///< Path files
    log4cplus::Logger logger;
    std::vector<std::string> files;

    ed::types::Node* add_node(const navitia::type::GeographicalCoord& coord, const std::string& uri);
    void fill_admins();
    void fill_postal_codes();
    void fill_ways_edges();
    bool starts_with(std::string filename, const std::string& prefex);

public:
    ConvCoord conv_coord = ConvCoord(Projection("Lambert 2 étendu", "27572", false));

    ed::Georef data;

    GeopalParser(std::string path);

    void fill();
};
}  // namespace connectors
}  // namespace ed
