#include "bdtopo_parser.h"
#include "utils/csv.h"
#include "utils/functions.h"

#include <boost/lexical_cast.hpp>

#include <unordered_map>

namespace navimake{ namespace connectors{

namespace nt = navitia::type;
//namespace ns = navitia::streetnetwork;
namespace ns = navitia::georef;

BDTopoParser::BDTopoParser(const std::string& path): path(path){}

void BDTopoParser::load_city(navimake::Data& data){
    
    CsvReader reader(path + "/commune.txt");
    std::map<std::string, int> cols;

    std::vector<std::string> row = reader.next();
    for(size_t i=0; i < row.size(); i++){
        cols[row[i]] = i;
    }

    size_t name = cols["nom"];
    size_t insee = cols["code_insee"];
    
    for(row = reader.next(); !reader.eof() ;row = reader.next()){
        if(row.size() < 2)
            continue;
        navimake::types::City* city = new navimake::types::City();
        city->external_code = row[insee];
        city->name = row[name];
        data.cities.push_back(city);
    }

}


void BDTopoParser::load_georef(ns::GeoRef & geo_ref){
    using namespace navitia::georef;

    CsvReader reader(path + "/route_adresse.txt");
    std::map<std::string, int> cols;

    std::vector<std::string> row = reader.next();
    int col_count = boost::lexical_cast<int>(row.size());
    for(int i=0; i < col_count; i++){
        boost::to_lower(row[i]);
        cols[row[i]] = i;
    }


    int type = reader.get_pos_col("typ_adres", cols);
    int nom = reader.get_pos_col("nom_rue_d", cols);
    int x1 = reader.get_pos_col("x_debut", cols);
    int y1 = reader.get_pos_col("y_debut", cols);
    int x2 = reader.get_pos_col("x_fin", cols);
    int y2 = reader.get_pos_col("y_fin", cols);
    int l = reader.get_pos_col("longueur", cols);
    int insee = reader.get_pos_col("inseecom_g", cols);
    int n_deb_d = reader.get_pos_col("bornedeb_d", cols);
    int n_deb_g = reader.get_pos_col("bornedeb_g", cols);
    int n_fin_d = reader.get_pos_col("bornefin_d", cols);
    int n_fin_g = reader.get_pos_col("bornefin_g", cols);
    /*
    size_t type = cols["typ_adres"];
    size_t nom = cols["nom_rue_d"];
    size_t x1 = cols["x_debut"];
    size_t y1 = cols["y_debut"];
    size_t x2 = cols["x_fin"];
    size_t y2 = cols["y_fin"];
    size_t l = cols["longueur"];
    size_t insee = cols["inseecom_g"];
    size_t n_deb_d = cols["bornedeb_d"];
    size_t n_deb_g = cols["bornedeb_g"];
    size_t n_fin_d = cols["bornefin_d"];
    size_t n_fin_g = cols["bornefin_g"];
    */

    std::unordered_map<std::string, vertex_t> vertex_map;
    std::unordered_map<std::string, Way> way_map;
    std::unordered_map<int, HouseNumber> house_number_left_map;
    std::unordered_map<int, HouseNumber> house_number_right_map;

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
            way_map[way_key].city = row[insee];
            if (type > -1){
                way_map[way_key].way_type = row[type];
            }
        }

         //hn_deb_d
        //std::string hn_key = row[x1] + row[y1] + row[n_deb_d];
        auto hn = house_number_right_map.find(hn_deb_d.number);
        if ((hn == house_number_right_map.end()) && (hn_deb_d.number > 0) && (hn_deb_d.number % 2 == 0)){
            way_map[way_key].house_number_right.push_back(hn_deb_d);
            house_number_right_map[hn_deb_d.number]= hn_deb_d;
        }

        //hn_deb_g
        //hn_key = row[x1] + row[y1] + row[n_deb_g];
        hn = house_number_left_map.find(hn_deb_g.number);
        if ((hn == house_number_left_map.end()) && (hn_deb_g.number > 0)  && (hn_deb_d.number % 2 != 0)){
            way_map[way_key].house_number_left.push_back(hn_deb_g);
            house_number_left_map[hn_deb_g.number]= hn_deb_g;
        }

        //hn_fin_d
        //hn_key = row[x2] + row[y2] + row[n_fin_d];
        hn = house_number_right_map.find(hn_fin_d.number);
        if ((hn == house_number_right_map.end()) && (hn_fin_d.number > 0)  && (hn_deb_d.number % 2 == 0)){
            way_map[way_key].house_number_right.push_back(hn_fin_d);
            house_number_right_map[hn_fin_d.number]= hn_fin_d;
        }

        //hn_fin_g
        //hn_key = row[x2] + row[y2] + row[n_fin_g];
        hn = house_number_left_map.find(hn_fin_g.number);
        if ((hn == house_number_left_map.end()) && (hn_fin_g.number > 0)  && (hn_deb_d.number % 2 != 0)){
            way_map[way_key].house_number_left.push_back(hn_fin_g);
            house_number_left_map[hn_fin_g.number]= hn_fin_g;
        }

        way_map[way_key].edges.push_back(std::make_pair(source, target));
        way_map[way_key].edges.push_back(std::make_pair(target, source));
    }

    unsigned int idx=0;

    for(auto way : way_map){
        way.second.sort_house_number();
        geo_ref.ways.push_back(way.second);
        geo_ref.ways.back().idx = idx;        
        BOOST_FOREACH(auto node_pair, geo_ref.ways.back().edges){
            edge_t e = boost::edge(node_pair.first, node_pair.second, geo_ref.graph).first;
            geo_ref.graph[e].way_idx = idx;            
        }
        idx++;
    }
}

}} //fermeture des namespaces
