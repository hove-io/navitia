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

#include "geopal_parser.h"
#include "data_cleaner.h"

namespace ed { namespace connectors {

GeopalParser::GeopalParser(const std::string& path, const ed::connectors::ConvCoord& conv_coord): path(path),
                            conv_coord(conv_coord) {
    logger = log4cplus::Logger::getInstance("log");
    try{
            boost::filesystem::path directory(this->path);
            boost::filesystem::directory_iterator iter(directory), end;
             for(;iter != end; ++iter){
                 if (iter->path().extension() == ".txt"){
                    std::string file_name = iter->path().filename().string();
                    this->files.push_back(file_name);
                 }
             }
        }catch( const boost::filesystem::filesystem_error& e){
            throw GeopalParserException("GeopalParser : Error " + e.code().message());
        }
}

bool GeopalParser::starts_with(std::string filename, const std::string& prefex){
    return boost::algorithm::starts_with(filename, prefex);
}

void GeopalParser::fill(){

    this->fill_admins();
    LOG4CPLUS_INFO(logger, "Admin count: " << this->data.admins.size());
    this->fill_nodes();
    LOG4CPLUS_INFO(logger, "Noeud count: " << this->data.nodes.size());
    this->fill_ways_edges();
    LOG4CPLUS_INFO(logger, "Way count: " << this->data.ways.size());
    LOG4CPLUS_INFO(logger, "Edge count: " << this->data.edges.size());

    LOG4CPLUS_INFO(logger, "begin: data cleaning");
    data_cleaner cleaner(data);
    cleaner.clean();
    LOG4CPLUS_INFO(logger, "End: data cleaning");

    this->fill_house_numbers();
    LOG4CPLUS_INFO(logger, "House number count: " << this->data.house_numbers.size());
}

ed::types::Node* GeopalParser::add_node(const navitia::type::GeographicalCoord& coord, const std::string& uri){
    ed::types::Node* node = new ed::types::Node;
    node->id = this->data.nodes.size() + 1;
    node->coord = this->conv_coord.convert_to(coord);
    this->data.nodes[uri] = node;
    return node;
}

void GeopalParser::fill_nodes(){
    for(const std::string file_name : this->files){
        if(!this->starts_with(file_name, "adresse")){
            continue;
        }
        CsvReader reader(this->path + "/" + file_name, ';', true, true);
        if(!reader.is_open()) {
            throw GeopalParserException("Error on open file " + reader.filename);
        }
        std::vector<std::string> mandatory_headers = {"code_insee", "code_post", "x_adresse", "y_adresse"};
        if(!reader.validate(mandatory_headers)) {
            throw GeopalParserException("Impossible to parse file "
                                        + reader.filename + " . Not find column : "
                                        + reader.missing_headers(mandatory_headers));
        }
        int insee_c = reader.get_pos_col("code_insee");
        int code_post_c = reader.get_pos_col("code_post");
        int x_c = reader.get_pos_col("x_adresse");
        int y_c = reader.get_pos_col("y_adresse");
        while(!reader.eof()){
            std::vector<std::string> row = reader.next();
            std::string uri;
            if (reader.is_valid(x_c, row) && reader.is_valid(y_c, row) && reader.is_valid(insee_c, row)){
                uri = row[x_c] + row[y_c];
                auto adm = this->data.admins.find(row[insee_c]);
                if (adm != this->data.admins.end()){
                    if(reader.is_valid(code_post_c, row)){
                        adm->second->postcode = row[code_post_c];
                    }
                    auto nd = this->data.nodes.find(uri);
                    if(nd == this->data.nodes.end())
                        this->add_node(navitia::type::GeographicalCoord(str_to_double(row[x_c]), str_to_double(row[y_c])), uri);
                }
            }
        }
    }
}

void GeopalParser::fill_admins(){
    for(const std::string file_name : this->files){
        if (! this->starts_with(file_name, "commune")){
            continue;
        }
        CsvReader reader(this->path + "/" + file_name, ';', true, true);
        if(!reader.is_open()) {
            throw GeopalParserException("Error on open file " + reader.filename);
        }
        std::vector<std::string> mandatory_headers = {"nom" , "code_insee", "x_commune", "y_commune"};
        if(!reader.validate(mandatory_headers)) {
            throw GeopalParserException("Impossible to parse file " + reader.filename +" . Not find column : " + reader.missing_headers(mandatory_headers));
        }
        int name_c = reader.get_pos_col("nom");
        int insee_c = reader.get_pos_col("code_insee");
        int x_c = reader.get_pos_col("x_commune");
        int y_c = reader.get_pos_col("y_commune");
        while(!reader.eof()){
            std::vector<std::string> row = reader.next();
            if (reader.is_valid(insee_c, row) && reader.is_valid(x_c, row) && reader.is_valid(y_c, row)){
                auto itm = this->data.admins.find(row[insee_c]);
                if(itm == this->data.admins.end()){
                    ed::types::Admin* admin = new ed::types::Admin;
                    admin->insee = row[insee_c];
                    admin->id = this->data.admins.size() + 1;
                    if (reader.is_valid(name_c, row))
                        admin->name = row[name_c];
                    admin->coord = this->conv_coord.convert_to(navitia::type::GeographicalCoord(str_to_double(row[x_c]), str_to_double(row[y_c])));
                    this->data.admins[admin->insee] = admin;
                }
            }
        }
    }
}

void GeopalParser::fill_ways_edges(){
    for(const std::string file_name : this->files){
        if (! this->starts_with(file_name, "route_a")){
            continue;
        }
        CsvReader reader(this->path + "/" + file_name, ';', true, true);
        if(!reader.is_open()) {
            throw GeopalParserException("Error on open file " + reader.filename);
        }
        std::vector<std::string> mandatory_headers = {"id", "x_debut" , "y_debut", "x_fin", "y_fin", "longueur", "inseecom_g",
        "inseecom_d"};
        if(!reader.validate(mandatory_headers)) {
            throw GeopalParserException("Impossible to parse file " + reader.filename +" . Not find column : " + reader.missing_headers(mandatory_headers));
        }
        int nom_voie_d = reader.get_pos_col("nom_voie_d");
        int x1 = reader.get_pos_col("x_debut");
        int y1 = reader.get_pos_col("y_debut");
        int x2 = reader.get_pos_col("x_fin");
        int y2 = reader.get_pos_col("y_fin");
        int l = reader.get_pos_col("longueur");
        int inseecom_d = reader.get_pos_col("inseecom_d");
        int id = reader.get_pos_col("id");
        while(!reader.eof()){
            std::vector<std::string> row = reader.next();
            if (reader.is_valid(x1, row) && reader.is_valid(y1, row)
                && reader.is_valid(x2, row) && reader.is_valid(y2, row)
                && reader.is_valid(inseecom_d, row)
                && reader.is_valid(id, row)){
                auto admin = this->data.admins.find(row[inseecom_d]);
                if(admin != this->data.admins.end()){
                    std::string source  = row[x1] + row[y1];
                    std::string target  = row[x2] + row[y2];
                    ed::types::Node* source_node;
                    ed::types::Node* target_node;
                    auto source_it = this->data.nodes.find(source);
                    auto target_it = this->data.nodes.find(target);

                    if(source_it == this->data.nodes.end()){
                        source_node = this->add_node(navitia::type::GeographicalCoord(str_to_double(row[x1]), str_to_double(row[y1])), source);
                    }else{
                        source_node = source_it->second;
                    }

                    if(target_it == this->data.nodes.end()){
                        target_node = this->add_node(navitia::type::GeographicalCoord(str_to_double(row[x2]), str_to_double(row[y2])), target);
                    }else{
                        target_node = target_it->second;
                    }
                    ed::types::Way* current_way = nullptr;
                    std::string wayd_uri = row[id];
                    auto way = this->data.ways.find(wayd_uri);
                    if(way == this->data.ways.end()){
                        ed::types::Way* wy = new ed::types::Way;
                        wy->admin = admin->second;
                        admin->second->is_used = true;
                        wy->id = this->data.ways.size() + 1;
                        if(reader.is_valid(nom_voie_d, row)){
                            wy->name = row[nom_voie_d];
                        }
                        wy->type ="";
                        wy->uri = wayd_uri;
                        this->data.ways[wayd_uri] = wy;
                        current_way = wy;
                    }else{
                        current_way = way->second;
                    }
                    auto edge = this->data.edges.find(wayd_uri);
                    if(edge == this->data.edges.end()){
                        ed::types::Edge* edg = new ed::types::Edge;
                        edg->source = source_node;
                        edg->source->is_used = true;
                        edg->target = target_node;
                        edg->target->is_used = true;
                        if(reader.is_valid(l, row)){
                            edg->length = str_to_int(row[l]);
                        }
                        edg->way = current_way;
                        this->data.edges[wayd_uri]= edg;
                        current_way->edges.push_back(edg);
                    }
                }
            }
        }
    }
}

void GeopalParser::fill_house_numbers(){
    for(const std::string file_name : this->files){
        if (! this->starts_with(file_name, "adresse")){
            continue;
        }
        CsvReader reader(this->path + "/" + file_name, ';', true, true);
        if(!reader.is_open()) {
            throw GeopalParserException("Error on open file " + reader.filename);
        }
        std::vector<std::string> mandatory_headers = {"id_tr", "numero", "nom_voie", "code_insee", "x_adresse", "y_adresse"};
        if(!reader.validate(mandatory_headers)) {
            throw GeopalParserException("Impossible to parse file " + reader.filename +" . Not find column : " + reader.missing_headers(mandatory_headers));
        }
        int insee_c = reader.get_pos_col("code_insee");
        int nom_voie_c = reader.get_pos_col("nom_voie");
        int numero_c = reader.get_pos_col("numero");
        int x_c = reader.get_pos_col("x_adresse");
        int y_c = reader.get_pos_col("y_adresse");
        int id_tr = reader.get_pos_col("id_tr");
        while(!reader.eof()){
            std::vector<std::string> row = reader.next();
            if (reader.is_valid(x_c, row) && reader.is_valid(y_c, row)
                && reader.is_valid(insee_c, row) && reader.is_valid(numero_c, row)
                && reader.is_valid(nom_voie_c, row)
                && reader.is_valid(id_tr, row)){
                std::string way_uri = row[id_tr];
                auto it = this->data.fusion_ways.find(way_uri);
                if(it != this->data.fusion_ways.end()){
                    std::string hn_uri = row[x_c] + row[y_c] + row[numero_c];
                    auto hn = this->data.house_numbers.find(hn_uri);
                    if (hn ==this-> data.house_numbers.end()){
                        ed::types::HouseNumber* current_hn = new ed::types::HouseNumber;
                        current_hn->coord = this->conv_coord.convert_to(navitia::type::GeographicalCoord(str_to_double(row[x_c]), str_to_double(row[y_c])));
                        current_hn->number = row[numero_c];
                        current_hn->way = it->second;
                        this->data.house_numbers[hn_uri] = current_hn;
                    }
                }
            }
        }
    }
}
}
}//namespace
