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

#include "poi_parser.h"

#include <utility>

#include "type/data.h"
#include "utils/csv.h"
#include "utils/functions.h"
#include "projection_system_reader.h"

namespace ed {
namespace connectors {

PoiParserException::~PoiParserException() noexcept = default;

PoiParser::PoiParser(std::string path) : path(std::move(path)) {
    logger = log4cplus::Logger::getInstance("log");
}

void PoiParser::fill_poi_type() {
    CsvReader reader(this->path + "/poi_type.txt", ';', true, true);
    if (!reader.is_open()) {
        throw PoiParserException("Cannot open file : " + reader.filename);
    }
    std::vector<std::string> mandatory_headers = {"poi_type_id", "poi_type_name"};
    if (!reader.validate(mandatory_headers)) {
        throw PoiParserException("Impossible to parse file " + reader.filename
                                 + " . Not find column : " + reader.missing_headers(mandatory_headers));
    }
    int id_c = reader.get_pos_col("poi_type_id");
    int name_c = reader.get_pos_col("poi_type_name");
    while (!reader.eof()) {
        std::vector<std::string> row = reader.next();
        if (reader.is_valid(id_c, row) && reader.is_valid(name_c, row)) {
            const auto& itm = this->data.poi_types.find(row[id_c]);
            if (itm == this->data.poi_types.end()) {
                auto* poi_type = new ed::types::PoiType;
                poi_type->id = this->data.poi_types.size() + 1;
                poi_type->name = row[name_c];
                this->data.poi_types[row[id_c]] = poi_type;
            }
        }
    }
}

void PoiParser::fill_poi() {
    CsvReader reader(this->path + "/poi.txt", ';', true, true);
    if (!reader.is_open()) {
        throw PoiParserException("Cannot open file : " + reader.filename);
    }
    std::vector<std::string> mandatory_headers = {"poi_id",  "poi_name", "poi_weight", "poi_visible",
                                                  "poi_lat", "poi_lon",  "poi_type_id"};
    if (!reader.validate(mandatory_headers)) {
        throw PoiParserException("Impossible to parse file " + reader.filename
                                 + " . Not find column : " + reader.missing_headers(mandatory_headers));
    }
    int id_c = reader.get_pos_col("poi_id");
    int name_c = reader.get_pos_col("poi_name");
    int weight_c = reader.get_pos_col("poi_weight");
    int visible_c = reader.get_pos_col("poi_visible");
    int lat_c = reader.get_pos_col("poi_lat");
    int lon_c = reader.get_pos_col("poi_lon");
    int type_id_c = reader.get_pos_col("poi_type_id");
    int address_number_c = reader.get_pos_col("poi_address_number");
    int address_name_c = reader.get_pos_col("poi_address_name");

    while (!reader.eof()) {
        std::vector<std::string> row = reader.next();
        if (reader.is_valid(id_c, row) && reader.is_valid(name_c, row) && reader.is_valid(weight_c, row)
            && reader.is_valid(visible_c, row) && reader.is_valid(lat_c, row) && reader.is_valid(lon_c, row)
            && reader.is_valid(type_id_c, row)) {
            const auto& itm = this->data.pois.find(row[id_c]);
            if (itm == this->data.pois.end()) {
                const auto& poi_type = this->data.poi_types.find(row[type_id_c]);
                if (poi_type != this->data.poi_types.end()) {
                    ed::types::Poi poi(row[id_c]);
                    poi.id = this->data.pois.size() + 1;
                    poi.name = row[name_c];
                    try {
                        poi.visible = boost::lexical_cast<bool>(row[visible_c]);
                    } catch (const boost::bad_lexical_cast&) {
                        LOG4CPLUS_WARN(logger, "Impossible to parse the visible for " + row[id_c] + " " + row[name_c]);
                        poi.visible = true;
                    }
                    try {
                        poi.weight = boost::lexical_cast<int>(row[weight_c]);
                    } catch (const boost::bad_lexical_cast&) {
                        LOG4CPLUS_WARN(logger, "Impossible to parse the weight for " + row[id_c] + " " + row[name_c]);
                        poi.weight = 0;
                    }
                    poi.poi_type = poi_type->second;
                    poi.coord = this->conv_coord.convert_to(
                        navitia::type::GeographicalCoord(str_to_double(row[lon_c]), str_to_double(row[lat_c])));
                    if (reader.is_valid(address_number_c, row)) {
                        poi.address_number = row[address_number_c];
                    }
                    if (reader.is_valid(address_name_c, row)) {
                        poi.address_name = row[address_name_c];
                    }
                    this->data.pois[row[id_c]] = poi;
                }
            }
        }
    }
}

void PoiParser::fill_poi_properties() {
    CsvReader reader(this->path + "/poi_properties.txt", ';', true, true);
    if (!reader.is_open()) {
        LOG4CPLUS_WARN(logger, "Cannot found file : " + reader.filename);
        return;
    }
    std::vector<std::string> mandatory_headers = {"poi_id", "key", "value"};
    if (!reader.validate(mandatory_headers)) {
        throw PoiParserException("Impossible to parse file " + reader.filename
                                 + " . Not find column : " + reader.missing_headers(mandatory_headers));
    }
    int id_c = reader.get_pos_col("poi_id");
    int key_c = reader.get_pos_col("key");
    int value_c = reader.get_pos_col("value");

    while (!reader.eof()) {
        std::vector<std::string> row = reader.next();
        if (reader.is_valid(id_c, row) && reader.is_valid(key_c, row) && reader.is_valid(value_c, row)) {
            const auto itm = this->data.pois.find(row[id_c]);
            if (itm != this->data.pois.end()) {
                itm->second.properties[row[key_c]] = row[value_c];
            } else {
                LOG4CPLUS_WARN(logger, "Poi " << row[id_c] << " not found");
            }
        }
    }
}

void PoiParser::fill() {
    // default input coord system is lambert 2
    conv_coord = ProjectionSystemReader(path + "/projection.txt").read_conv_coord();

    LOG4CPLUS_INFO(logger, "projection system: " << this->conv_coord.origin.name << "("
                                                 << this->conv_coord.origin.definition << ")");

    fill_poi_type();
    fill_poi();
    fill_poi_properties();
}

}  // namespace connectors
}  // namespace ed
