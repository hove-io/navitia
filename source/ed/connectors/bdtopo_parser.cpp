#include "bdtopo_parser.h"
#include "utils/csv.h"
#include "utils/functions.h"

#include <unordered_map>

namespace ed{ namespace connectors{

namespace nt = navitia::type;
namespace ns = navitia::georef;

BDTopoParser::BDTopoParser(const std::string& path): path(path){}

void BDTopoParser::load_city(ed::Data& data){

    CsvReader reader(path + "/commune.txt", true);

    std::vector<std::string> row = reader.next();

    int name = reader.get_pos_col("nom");
    int insee = reader.get_pos_col("code_insee");
    int x = reader.get_pos_col("x");
    int y = reader.get_pos_col("y");

    for(row = reader.next(); !reader.eof() ;row = reader.next()){
        if(row.size() < 2)
            continue;
        ed::types::City* city = new ed::types::City();
        if(insee != -1)
            city->uri = row[insee];
        if(name != -1)
            city->name = row[name];
        if(x!=-1 && y!=-1)
            city->coord = navitia::type::GeographicalCoord(str_to_double(row[x]),
                                                                        str_to_double(row[y]));
        data.cities.push_back(city);
    }

}
void add_way(std::unordered_map<std::string, navitia::georef::Way>& way_map,
            std::unordered_map<std::string, navitia::georef::HouseNumber>& house_number_left_map,
             std::unordered_map<std::string, navitia::georef::HouseNumber>& house_number_right_map,
             navitia::georef::HouseNumber& house_number, std::string& way_key){

    if (house_number.number > 0){
        std::string key;
        key = boost::lexical_cast<std::string>(house_number.number)+":"+way_key;

        if (house_number.number % 2 == 0){
            auto hn = house_number_right_map.find(key);
            if (hn == house_number_right_map.end()){
                way_map[way_key].add_house_number(house_number);
                house_number_right_map[key]= house_number;
            }
        } else{
            auto hn = house_number_left_map.find(key);
            if (hn == house_number_left_map.end()){
                way_map[way_key].add_house_number(house_number);
                house_number_left_map[key]= house_number;
            }
        }
    }
}

void BDTopoParser::load_georef(ns::GeoRef & geo_ref){
    using namespace navitia::georef;

    CsvReader reader(path + "/route_adresse.txt", true);

    std::vector<std::string> row = reader.next();

    int type = reader.get_pos_col("typ_adres");
    int nom = reader.get_pos_col("nom_rue_d");
    int x1 = reader.get_pos_col("x_debut");
    int y1 = reader.get_pos_col("y_debut");
    int x2 = reader.get_pos_col("x_fin");
    int y2 = reader.get_pos_col("y_fin");
    int l = reader.get_pos_col("longueur");
    int insee = reader.get_pos_col("inseecom_g");
    int n_deb_d = reader.get_pos_col("bornedeb_d");
    int n_deb_g = reader.get_pos_col("bornedeb_g");
    int n_fin_d = reader.get_pos_col("bornefin_d");
    int n_fin_g = reader.get_pos_col("bornefin_g");

    std::unordered_map<std::string, vertex_t> vertex_map;
    std::unordered_map<std::string, Way> way_map;
    std::unordered_map<std::string, HouseNumber> house_number_left_map;
    std::unordered_map<std::string, HouseNumber> house_number_right_map;

    for(row = reader.next(); !reader.eof() ;row = reader.next()){
        vertex_t source, target;
        HouseNumber hn_deb_d;
        HouseNumber hn_fin_d;
        HouseNumber hn_deb_g;
        HouseNumber hn_fin_g;

        auto it = vertex_map.find(row[x1] + row[y1]);
        if(it == vertex_map.end()){
            Vertex v;
            try{
                v.coord = navitia::type::GeographicalCoord(str_to_double(row[x1]),
                                                           str_to_double(row[y1]));

            } catch(...){
                std::cout << "coord : " << row[x1] << ";" << row[y1] << std::endl;
            }

            source = vertex_map[row[x1] + row[y1]] = boost::add_vertex(v, geo_ref.graph);
        }
        else source = vertex_map[row[x1] + row[y1]];

        it = vertex_map.find(row[x2] + row[y2]);
        if(it == vertex_map.end()){
            Vertex v;
            try{
                v.coord = navitia::type::GeographicalCoord(boost::lexical_cast<double>(row[x2]),
                                                           boost::lexical_cast<double>(row[y2]));

            } catch(...){
                std::cout << "coord : " << row[x2] << ";" << row[y2] << std::endl;
            }
            target = vertex_map[row[x2] + row[y2]] = boost::add_vertex(v, geo_ref.graph);
        }
        else target = vertex_map[row[x2] + row[y2]];

        Edge e1, e2;
        try{
            e1.length = e2.length = str_to_double(row[l]);
        }catch(...){
            std::cout << "longueur :( " << row[l] << ")" << std::endl;
        }

        boost::add_edge(source, target, e1, geo_ref.graph);
        boost::add_edge(target, source, e2, geo_ref.graph);

        hn_deb_d.number = str_to_int(row[n_deb_d]);

        hn_fin_d.number = str_to_int(row[n_fin_d]);

        hn_deb_g.number = str_to_int(row[n_deb_g]);

        hn_fin_g.number = str_to_int(row[n_fin_g]);

        hn_deb_d.coord = hn_deb_g.coord = navitia::type::GeographicalCoord(str_to_double(row[x1]),
                                                                   str_to_double(row[y1]));
        hn_fin_d.coord = hn_fin_g.coord = navitia::type::GeographicalCoord(str_to_double(row[x2]),
                                                                   str_to_double(row[y2]));

        std::string way_key;
        if(row[insee].substr(0,2) == "75")
            way_key = row[nom] + "75056";
        else
            way_key = row[nom] + row[insee];

        auto way_it = way_map.find(way_key);
        if(way_it == way_map.end()){
            way_map[way_key].name = row[nom];
            way_map[way_key].uri = way_key;
            if (type > -1){
                way_map[way_key].way_type = row[type];
            }
        }

        add_way(way_map, house_number_left_map, house_number_right_map, hn_deb_d, way_key);
        add_way(way_map, house_number_left_map, house_number_right_map, hn_fin_d, way_key);

        add_way(way_map, house_number_left_map, house_number_right_map, hn_deb_g, way_key);
        add_way(way_map, house_number_left_map, house_number_right_map, hn_fin_g, way_key);

        way_map[way_key].edges.push_back(std::make_pair(source, target));
        way_map[way_key].edges.push_back(std::make_pair(target, source));
    }

    unsigned int idx=0;

     for(auto way : way_map){
        geo_ref.ways.push_back(way.second);
        geo_ref.ways.back().idx = idx;
        for(auto node_pair : geo_ref.ways.back().edges){
            edge_t e = boost::edge(node_pair.first, node_pair.second, geo_ref.graph).first;
            geo_ref.graph[e].way_idx = idx;
        }
        idx++;
    }
}

}} //fermeture des namespaces
