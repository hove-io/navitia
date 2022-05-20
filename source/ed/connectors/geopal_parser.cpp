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

#include "geopal_parser.h"

#include <utility>

#include "data_cleaner.h"
#include "projection_system_reader.h"

namespace ed {
namespace connectors {

GeopalParserException::~GeopalParserException() noexcept = default;

GeopalParser::GeopalParser(std::string path) : path(std::move(path)) {
    logger = log4cplus::Logger::getInstance("log");
    try {
        boost::filesystem::path directory(this->path);
        boost::filesystem::directory_iterator iter(directory), end;
        for (; iter != end; ++iter) {
            if (iter->path().extension() == ".txt") {
                std::string file_name = iter->path().filename().string();
                this->files.push_back(file_name);
            }
        }
    } catch (const boost::filesystem::filesystem_error& e) {
        throw GeopalParserException("GeopalParser : Error " + e.code().message());
    }
}

bool GeopalParser::starts_with(std::string filename, const std::string& prefex) {
    return boost::algorithm::starts_with(filename, prefex);
}

void GeopalParser::fill() {
    // default input coord system is lambert 2
    conv_coord =
        ProjectionSystemReader(path + "/projection.txt", ConvCoord(Projection("Lambert 2 étendu", "27572", false)))
            .read_conv_coord();

    LOG4CPLUS_INFO(logger, "projection system: " << this->conv_coord.origin.name << "("
                                                 << this->conv_coord.origin.definition << ")");

    this->fill_admins();
    LOG4CPLUS_INFO(logger, "Admin count: " << this->data.admins.size());
    this->fill_postal_codes();
    this->fill_ways_edges();
    LOG4CPLUS_INFO(logger, "Node count: " << this->data.nodes.size());
    LOG4CPLUS_INFO(logger, "Way count: " << this->data.ways.size());
    LOG4CPLUS_INFO(logger, "Edge count: " << this->data.edges.size());

    LOG4CPLUS_INFO(logger, "begin: data cleaning");
    data_cleaner cleaner(data);
    cleaner.clean();
    LOG4CPLUS_INFO(logger, "End: data cleaning");

    LOG4CPLUS_INFO(logger, "House number count: " << this->data.house_numbers.size());
}

ed::types::Node* GeopalParser::add_node(const navitia::type::GeographicalCoord& coord, const std::string& uri) {
    auto* node = new ed::types::Node;
    node->id = this->data.nodes.size() + 1;
    node->coord = this->conv_coord.convert_to(coord);
    this->data.nodes[uri] = node;
    return node;
}

void GeopalParser::fill_postal_codes() {
    for (const std::string& file_name : this->files) {
        if (!this->starts_with(file_name, "adresse")) {
            continue;
        }
        CsvReader reader(this->path + "/" + file_name, ';', true, true);
        if (!reader.is_open()) {
            throw GeopalParserException("Error on open file " + reader.filename);
        }
        std::vector<std::string> mandatory_headers = {"code_insee", "code_post"};
        if (!reader.validate(mandatory_headers)) {
            throw GeopalParserException("Impossible to parse file " + reader.filename
                                        + " . Not find column : " + reader.missing_headers(mandatory_headers));
        }
        int insee_c = reader.get_pos_col("code_insee");
        int code_post_c = reader.get_pos_col("code_post");
        while (!reader.eof()) {
            std::vector<std::string> row = reader.next();
            if (reader.is_valid(code_post_c, row) && reader.is_valid(insee_c, row)) {
                auto adm = this->data.admins.find(row[insee_c]);
                if (adm != this->data.admins.end()) {
                    if (reader.is_valid(code_post_c, row)) {
                        adm->second->postal_codes.insert(row[code_post_c]);
                    }
                }
            }
        }
    }
}

void GeopalParser::fill_admins() {
    for (const std::string& file_name : this->files) {
        if (!this->starts_with(file_name, "commune")) {
            continue;
        }
        CsvReader reader(this->path + "/" + file_name, ';', true, true);
        if (!reader.is_open()) {
            throw GeopalParserException("Error on open file " + reader.filename);
        }
        std::vector<std::string> mandatory_headers = {"nom", "code_insee", "x_commune", "y_commune"};
        if (!reader.validate(mandatory_headers)) {
            throw GeopalParserException("Impossible to parse file " + reader.filename
                                        + " . Not find column : " + reader.missing_headers(mandatory_headers));
        }
        int name_c = reader.get_pos_col("nom");
        int insee_c = reader.get_pos_col("code_insee");
        int x_c = reader.get_pos_col("x_commune");
        int y_c = reader.get_pos_col("y_commune");
        while (!reader.eof()) {
            std::vector<std::string> row = reader.next();
            if (reader.is_valid(insee_c, row) && reader.is_valid(x_c, row) && reader.is_valid(y_c, row)) {
                auto itm = this->data.admins.find(row[insee_c]);
                if (itm == this->data.admins.end()) {
                    auto* admin = new ed::types::Admin;
                    admin->insee = row[insee_c];
                    admin->id = this->data.admins.size() + 1;
                    if (reader.is_valid(name_c, row))
                        admin->name = row[name_c];
                    admin->coord = this->conv_coord.convert_to(
                        navitia::type::GeographicalCoord(str_to_double(row[x_c]), str_to_double(row[y_c])));
                    this->data.admins[admin->insee] = admin;
                }
            }
        }
    }
}

namespace {
// We have in input something like that:
//
// 1                       9
// +----------------------->
// 2                       8
//
// 1 and 9 are at the left of the way, 2 and 8 at the right.  We
// generate something like that:
//
//    1     3     6     9
//    +     +     +     +
// +----------------------->
//     +       +       +
//     2       4       8
struct HouseNumberFromEdgesFiller {
    using Coord = navitia::type::GeographicalCoord;
    using Way = ed::types::Way;

    HouseNumberFromEdgesFiller(const CsvReader& r, GeopalParser& p)
        : reader(r),
          parser(p),
          x1(r.get_pos_col("x_debut")),
          y1(r.get_pos_col("y_debut")),
          x2(r.get_pos_col("x_fin")),
          y2(r.get_pos_col("y_fin")),
          left1(r.get_pos_col("bornedeb_g")),
          left2(r.get_pos_col("bornefin_g")),
          right1(r.get_pos_col("bornedeb_d")),
          right2(r.get_pos_col("bornefin_d")) {}
    void fill(const std::vector<std::string>& row, Way* way) {
        const auto from = coord(row[x1], row[y1]);
        const auto to = coord(row[x2], row[y2]);
        /*
        std::cout << std::setprecision(10)
                  << "{\"type\": \"Feature\", \"properties\": {}, \"geometry\": {"
                  << "\"type\": \"LineString\", \"coordinates\": ["
                  << "[" << from.lon() << ", " << from.lat() << "], "
                  << "[" << to.lon() << ", " << to.lat() << "] ] } }," << std::endl;
        */
        if (const auto numbers = get_numbers(left1, left2, row)) {
            if (numbers->first <= numbers->second) {
                add_left_house_number(from, to, numbers->first, numbers->second, way);
            } else {
                add_right_house_number(to, from, numbers->second, numbers->first, way);
            }
        }
        if (const auto numbers = get_numbers(right1, right2, row)) {
            if (numbers->first <= numbers->second) {
                add_right_house_number(from, to, numbers->first, numbers->second, way);
            } else {
                add_left_house_number(to, from, numbers->second, numbers->first, way);
            }
        }
    }

private:
    Coord coord(const std::string& x, const std::string& y) const {
        return parser.conv_coord.convert_to(Coord(str_to_double(x), str_to_double(y)));
    }
    boost::optional<std::pair<int, int> > get_numbers(const int idx1,
                                                      const int idx2,
                                                      const std::vector<std::string>& row) const {
        if (!reader.is_valid(left1, row) || !reader.is_valid(left2, row)) {
            return boost::none;
        }
        try {
            const int nb1 = boost::lexical_cast<int>(row.at(idx1));
            const int nb2 = boost::lexical_cast<int>(row.at(idx2));
            if (nb1 <= 0 || nb2 <= 0 || nb1 % 2 != nb2 % 2) {
                return boost::none;
            }
            return std::make_pair(nb1, nb2);
        } catch (boost::bad_lexical_cast&) {
            return boost::none;
        }
    }
    // return an orthogonal vector in trigonomic sense of approx 3m, null vector if from == to
    static Coord get_orthogonal_vec(const Coord& from, const Coord& to) {
        if (from == to) {
            return Coord(0, 0);
        }
        const Coord vec(to.lon() - from.lon(), to.lat() - from.lat());
        const double norm = sqrt(vec.lon() * vec.lon() + vec.lat() * vec.lat());
        return Coord(-vec.lat() / norm * 0.00003, vec.lon() / norm * 0.00003);
    }
    // make a geometric translation (add the components)
    static Coord translate(const Coord& u, const Coord& v) { return Coord(u.lon() + v.lon(), u.lat() + v.lat()); }
    void add_left_house_number(const Coord& from, const Coord& to, const int begin_num, const int last_num, Way* way) {
        // we translate from and to to the left, and then we just have
        // to add the house number on the segment [from, to]
        const auto vec = get_orthogonal_vec(from, to);
        add_middle_house_number(translate(from, vec), translate(to, vec), begin_num, last_num, way);
    }
    void add_right_house_number(const Coord& from, const Coord& to, const int begin_num, const int last_num, Way* way) {
        // we translate from and to to the right, and then we just have
        // to add the house number on the segment [from, to]
        const auto vec = get_orthogonal_vec(to, from);
        add_middle_house_number(translate(from, vec), translate(to, vec), begin_num, last_num, way);
    }
    void add_middle_house_number(const Coord& from,
                                 const Coord& to,
                                 const int begin_num,
                                 const int last_num,
                                 Way* way) {
        // We have that:
        //
        // 1                       9
        // +----------------------->
        // from                    to
        //
        // and we want to generate the house numbers like that:
        //
        //    1     3     6     9
        // +--+-----+-----+-----+-->
        // from                    to
        //    +----->
        //      vec
        //
        // We translate our current position using the vector vec to
        // go from point to point.
        const auto nb = double((last_num - begin_num) / 2 + 1);
        const Coord vec((to.lon() - from.lon()) / nb, (to.lat() - from.lat()) / nb);
        Coord coord = translate(from, Coord(vec.lon() / 2, vec.lat() / 2));
        for (int house_nb = begin_num; house_nb <= last_num; house_nb += 2) {
            add_house_number(coord, house_nb, way);
            coord = translate(coord, vec);
        }
    }
    void add_house_number(const Coord& coord, const int house_number, Way* way) {
        const std::string hn_uri = way->uri + ":" + std::to_string(house_number);
        if (parser.data.house_numbers.count(hn_uri) == 0) {
            ed::types::HouseNumber& current_hn = parser.data.house_numbers[hn_uri];
            current_hn.coord = coord;
            current_hn.number = std::to_string(house_number);
            current_hn.way = way;
        }
        /*
        std::cout << std::setprecision(10)
                  << "{\"type\": \"Feature\", \"properties\": { \"house_number\": "
                  << house_number << "}, \"geometry\": { \"type\": \"Point\", \"coordinates\": "
                  << "[" << coord.lon() << ", " << coord.lat() << "] } }," << std::endl;
        */
    }

    const CsvReader& reader;
    GeopalParser& parser;
    const int x1;
    const int y1;
    const int x2;
    const int y2;
    const int left1;
    const int left2;
    const int right1;
    const int right2;
};
}  // namespace

void GeopalParser::fill_ways_edges() {
    for (const std::string& file_name : this->files) {
        if (!this->starts_with(file_name, "route_a")) {
            continue;
        }
        CsvReader reader(this->path + "/" + file_name, ';', true, true);
        if (!reader.is_open()) {
            throw GeopalParserException("Error on open file " + reader.filename);
        }
        std::vector<std::string> mandatory_headers = {"id",    "x_debut",  "y_debut",    "x_fin",
                                                      "y_fin", "longueur", "inseecom_g", "inseecom_d"};
        if (!reader.validate(mandatory_headers)) {
            throw GeopalParserException("Impossible to parse file " + reader.filename
                                        + " . Column(s) not found: " + reader.missing_headers(mandatory_headers));
        }
        auto house_number_filler = HouseNumberFromEdgesFiller(reader, *this);
        int nom_voie_d = reader.get_pos_col("nom_voie_d");
        int x1 = reader.get_pos_col("x_debut");
        int y1 = reader.get_pos_col("y_debut");
        int x2 = reader.get_pos_col("x_fin");
        int y2 = reader.get_pos_col("y_fin");
        int l = reader.get_pos_col("longueur");
        int visible = reader.get_pos_col("visible");
        int inseecom_d = reader.get_pos_col("inseecom_d");
        int id = reader.get_pos_col("id");
        while (!reader.eof()) {
            std::vector<std::string> row = reader.next();
            if (reader.is_valid(x1, row) && reader.is_valid(y1, row) && reader.is_valid(x2, row)
                && reader.is_valid(y2, row) && reader.is_valid(id, row)) {
                auto admin_it = this->data.admins.find(row[inseecom_d]);
                ed::types::Admin* admin = nullptr;
                if (admin_it != this->data.admins.end()) {
                    admin = admin_it->second;
                }
                std::string source = row[x1] + row[y1];
                std::string target = row[x2] + row[y2];
                ed::types::Node* source_node;
                ed::types::Node* target_node;
                auto source_it = this->data.nodes.find(source);

                if (source_it == this->data.nodes.end()) {
                    source_node = this->add_node(
                        navitia::type::GeographicalCoord(str_to_double(row[x1]), str_to_double(row[y1])), source);
                } else {
                    source_node = source_it->second;
                }

                auto target_it = this->data.nodes.find(target);
                if (target_it == this->data.nodes.end()) {
                    target_node = this->add_node(
                        navitia::type::GeographicalCoord(str_to_double(row[x2]), str_to_double(row[y2])), target);
                } else {
                    target_node = target_it->second;
                }
                ed::types::Way* current_way = nullptr;
                std::string wayd_uri = row[id];
                auto way = this->data.ways.find(wayd_uri);
                if (way == this->data.ways.end()) {
                    auto* wy = new ed::types::Way;
                    if (admin) {
                        wy->admin = admin;
                        admin->is_used = true;
                    }
                    wy->id = this->data.ways.size() + 1;
                    if (reader.is_valid(nom_voie_d, row)) {
                        wy->name = row[nom_voie_d];
                    }
                    wy->type = "";
                    wy->uri = wayd_uri;
                    if (reader.has_col(visible, row) && row[visible] == "0") {
                        wy->visible = false;
                    } else {
                        wy->visible = true;
                    }
                    this->data.ways[wayd_uri] = wy;
                    current_way = wy;
                } else {
                    current_way = way->second;
                }
                auto edge = this->data.edges.find(wayd_uri);
                if (edge == this->data.edges.end()) {
                    auto* edg = new ed::types::Edge;
                    edg->source = source_node;
                    edg->source->is_used = true;
                    edg->target = target_node;
                    edg->target->is_used = true;
                    if (reader.is_valid(l, row)) {
                        edg->length = str_to_int(row[l]);
                    }
                    edg->way = current_way;
                    this->data.edges[wayd_uri] = edg;
                    current_way->edges.push_back(edg);
                }
                house_number_filler.fill(row, current_way);
            }
        }
    }
}

}  // namespace connectors
}  // namespace ed
