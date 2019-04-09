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

#include "synonym_parser.h"
#include <boost/lexical_cast.hpp>
#include "utils/csv.h"
#include "utils/configuration.h"
#include <boost/algorithm/string.hpp>

namespace ed {
namespace connectors {

SynonymParserException::~SynonymParserException() noexcept {}

SynonymParser::SynonymParser(const std::string& filename) : filename(filename) {
    logger = log4cplus::Logger::getInstance("log");
}

void SynonymParser::fill_synonyms() {
    std::string key, value;
    CsvReader reader(this->filename, ',', true, true);
    if (!reader.is_open()) {
        throw SynonymParserException("Cannot open file : " + reader.filename);
    }
    std::vector<std::string> mandatory_headers = {"key", "value"};
    if (!reader.validate(mandatory_headers)) {
        throw SynonymParserException("Impossible to parse file " + reader.filename
                                     + " . Not find column : " + reader.missing_headers(mandatory_headers));
    }

    int key_c = reader.get_pos_col("key"), value_c = reader.get_pos_col("value");
    while (!reader.eof()) {
        auto row = reader.next();
        if (reader.is_valid(key_c, row) && reader.has_col(value_c, row)) {
            key = row[key_c];
            value = row[value_c];
            boost::to_lower(key);
            boost::to_lower(value);
            this->synonym_map[key] = value;
        }
    }
    LOG4CPLUS_TRACE(logger, "synonyms : " << this->synonym_map.size());
}

void SynonymParser::fill() {
    this->fill_synonyms();
}
}  // namespace connectors
}  // namespace ed
