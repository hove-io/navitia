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

    size_t name = cols["NOM"];
    size_t insee = cols["CODE_INSEE"];
    
    for(row = reader.next(); !reader.eof() ;row = reader.next()){
        if(row.size() < 2)
            continue;
        navimake::types::City* city = new navimake::types::City();
        city->external_code = row[insee];
        city->name = row[name];
        data.cities.push_back(city);
    }

}


//void BDTopoParser::load_streetnetwork(ns::StreetNetwork & street_network){
void BDTopoParser::load_streetnetwork(ns::GeoRef & street_network){
    //using namespace navitia::streetnetwork;
    using namespace navitia::georef;

    CsvReader reader(path + "/route_adresse.txt");
    std::map<std::string, int> cols;


    std::vector<std::string> row = reader.next();
    for(size_t i=0; i < row.size(); i++){
        boost::to_lower(row[i]);
        cols[row[i]] = i;
    }


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

    //navitia::type::Projection ("Lambert 2 étendu", "+init=epsg:27572", false);
    navitia::type::Projection proj_lambert93("Lambert 93", "+init=epsg:2154", false);

    std::unordered_map<std::string, vertex_t> vertex_map;
    std::unordered_map<std::string, Way> way_map;

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
/*
                v.coord = navitia::type::GeographicalCoord(boost::lexical_cast<double>(row[x1]),
                                                           boost::lexical_cast<double>(row[y1]),
                                                           );
*/
                v.coord = navitia::type::GeographicalCoord(str_to_double(row[x1]),
                                                           str_to_double(row[y1]),
                                                           proj_lambert93);

            } catch(...){
                std::cout << "coord : " << row[x1] << ";" << row[y1] << std::endl;
            }

            source = vertex_map[row[x1] + row[y1]] = boost::add_vertex(v, street_network.graph);
        }
        else source = vertex_map[row[x1] + row[y1]];

        it = vertex_map.find(row[x2] + row[y2]);
        if(it == vertex_map.end()){
            Vertex v;
            try{
/*
                v.coord = navitia::type::GeographicalCoord(boost::lexical_cast<double>(row[x2]),
                                                           boost::lexical_cast<double>(row[y2]),
                                                           proj_lambert2e);
*/
                v.coord = navitia::type::GeographicalCoord(str_to_double(row[x2]),
                                                           str_to_double(row[y2]),
                                                           proj_lambert93);
            } catch(...){
                std::cout << "coord : " << row[x2] << ";" << row[y2] << std::endl;
            }
            target = vertex_map[row[x2] + row[y2]] = boost::add_vertex(v, street_network.graph);
        }
        else target = vertex_map[row[x2] + row[y2]];

        Edge e1, e2;
        try{
            //e1.length = e2.length = boost::lexical_cast<double>(row[l]);
            e1.length = e2.length = str_to_double(row[l]);
        }catch(...){
            std::cout << "longueur :( " << row[l] << std::endl;
        }
/*
        try{
            e1.start_number = boost::lexical_cast<int>(row[n_deb_d]);
            e1.end_number = boost::lexical_cast<int>(row[n_fin_d]);
            e2.start_number = boost::lexical_cast<int>(row[n_deb_g]);
            e2.end_number = boost::lexical_cast<int>(row[n_fin_g]);
        }
        catch(...){
            e1.start_number = -1;
            e1.end_number = -1;
            e2.start_number = -1;
            e2.end_number = -1;
            //std::cout << row[n_deb_d] << ", " << row[n_fin_d] << ", " << row[n_deb_g] << ", " << row[n_fin_g] << std::endl;
        }
*/
        boost::add_edge(source, target, e1, street_network.graph);
        boost::add_edge(target, source, e2, street_network.graph);

        hn_deb_d.number = str_to_int(row[n_deb_d]);
        hn_fin_d.number = str_to_int(row[n_fin_d]);
        hn_deb_g.number = str_to_int(row[n_deb_g]);
        hn_fin_g.number = str_to_int(row[n_fin_g]);

        hn_deb_d.coord = hn_deb_g.coord = navitia::type::GeographicalCoord(str_to_double(row[x1]),
                                                                   str_to_double(row[y1]),
                                                                   proj_lambert93);
        hn_fin_d.coord = hn_fin_g.coord = navitia::type::GeographicalCoord(str_to_double(row[x2]),
                                                                   str_to_double(row[y2]),
                                                                   proj_lambert93);

        std::string way_key;
        if(row[insee].substr(0,2) == "75")
            way_key = row[nom] + "75056";
        else
            way_key = row[nom] + row[insee];
        auto way_it = way_map.find(way_key);
        if(way_it == way_map.end()){
            way_map[way_key].name = row[nom];
            way_map[way_key].city = row[insee];
            way_map[way_key].way_type = row[type];
            /*  auto city_it = city_map.find(row[insee]);
                if(city_it != city_map.end()){
                    way_map[way_key].city_idx = city_map[row[insee]];
                }*/
        }



        way_map[way_key].edges.push_back(std::make_pair(source, target));
        way_map[way_key].edges.push_back(std::make_pair(target, source));
    }

    unsigned int idx=0;

    BOOST_FOREACH(auto way, way_map){
        street_network.ways.push_back(way.second);
        street_network.ways.back().idx = idx;
        BOOST_FOREACH(auto node_pair, street_network.ways.back().edges){
            edge_t e = boost::edge(node_pair.first, node_pair.second, street_network.graph).first;
            street_network.graph[e].way_idx = idx;
        }
        idx++;
    }
}

}} //fermeture des namespaces
