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

#include "projection_system_reader.h"
#include <boost/filesystem.hpp>
#include <utility>

#include "utils/exception.h"
#include "utils/csv.h"

namespace ed {
namespace connectors {

ProjectionSystemReader::ProjectionSystemReader(std::string p, ConvCoord d)
    : file(std::move(p)), default_conv_coord(d) {}

ConvCoord ProjectionSystemReader::read_conv_coord() const {
    CsvReader reader(file, ';', true, true);

    if (!reader.is_open()) {
        LOG4CPLUS_INFO(logger, "No projection file given, we use the default projection system: "
                                   << default_conv_coord.origin.name);
        return default_conv_coord;
    }

    std::vector<std::string> mandatory_headers = {"name", "definition"};

    if (!reader.validate(mandatory_headers)) {
        throw navitia::exception("Impossible to parse file " + reader.filename
                                 + " . Cannot find column : " + reader.missing_headers(mandatory_headers));
    }

    // we only have one line
    if (reader.eof()) {
        LOG4CPLUS_INFO(
            logger, "Projection file empty, we use the default projection system: " << default_conv_coord.origin.name);
        return default_conv_coord;
    }

    std::vector<std::string> row = reader.next();
    if (reader.eof()) {
        LOG4CPLUS_INFO(
            logger, "Projection file empty, we use the default projection system: " << default_conv_coord.origin.name);
        return default_conv_coord;
    }

    auto name = row.at(reader.get_pos_col("name"));
    auto definition = row.at(reader.get_pos_col("definition"));

    int is_degree_col = reader.get_pos_col("is_degree");

    // is degree is false by default
    auto is_degree = false;
    if (reader.has_col(is_degree_col, row)) {
        try {
            is_degree = boost::lexical_cast<bool>(row.at(is_degree_col));
        } catch (const boost::bad_lexical_cast&) {
            LOG4CPLUS_INFO(logger, "Unable to cast '" << row[is_degree_col] << "' to bool, ignoring parameter");
        }
    }

    return ConvCoord(Projection(name, definition, is_degree));
}

}  // namespace connectors
}  // namespace ed
