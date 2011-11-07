#include "bdtopo_parser.h"

#include <boost/lexical_cast.hpp>
#include "csv.h"

namespace navimake{ namespace connectors{

namespace nt = navitia::type;
namespace ns = navitia::streetnetwork;

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
    
    for(reader.next(); !reader.eof() ;row = reader.next()){
        if(row.size() < 2)
            continue;
        navimake::types::City* city = new navimake::types::City();
        city->external_code = row[insee];
        city->name = row[name];
        data.cities.push_back(city);
    }

}


    void BDTopoParser::load_streetnetwork(nt::Data& data){
        using namespace navitia::streetnetwork;
        std::unordered_map<std::string, idx_t> city_map = create_cities_indexes(data.pt_data.cities);

        CsvReader reader(path + "/route_adresse.txt");
        std::map<std::string, int> cols;


        std::vector<std::string> row = reader.next();
        for(size_t i=0; i < row.size(); i++){
            cols[row[i]] = i;
        }

        size_t nom = cols["Nom_rue_d"];
        size_t x1 = cols["X_debut"];
        size_t y1 = cols["Y_debut"];
        size_t x2 = cols["X_fin"];
        size_t y2 = cols["Y_fin"];
        size_t l = cols["Longueur"];
        size_t insee = cols["Inseecom_g"];
        size_t n_deb_d = cols["Bornedeb_d"];
        size_t n_deb_g = cols["Bornedeb_g"];
        size_t n_fin_d = cols["Bornefin_d"];
        size_t n_fin_g = cols["Bornefin_g"];

        std::unordered_map<std::string, vertex_t> vertex_map;
        std::unordered_map<std::string, Way> way_map;
        for(reader.next(); !reader.eof() ;row = reader.next()){
            vertex_t source, target;

            auto it = vertex_map.find(row[x1] + row[y1]);
            if(it == vertex_map.end()){
                Vertex v;
                try{
                    v.lon = boost::lexical_cast<double>(row[x1]);
                    v.lat = boost::lexical_cast<double>(row[y1]);
                } catch(...){
                    std::cout << "coord : " << row[x1] << ";" << row[y1] << std::endl;
                }

                source = vertex_map[row[x1] + row[y1]] = boost::add_vertex(v, data.street_network.graph);
            }
            else source = vertex_map[row[x1] + row[y1]];

            it = vertex_map.find(row[x2] + row[y2]);
            if(it == vertex_map.end()){
                Vertex v;
                try{
                    v.lon = boost::lexical_cast<double>(row[x2]);
                    v.lat = boost::lexical_cast<double>(row[y2]);
                } catch(...){
                    std::cout << "coord : " << row[x1] << ";" << row[y1] << std::endl;
                }
                target = vertex_map[row[x2] + row[y2]] = boost::add_vertex(v, data.street_network.graph);
            }
            else target = vertex_map[row[x2] + row[y2]];

            Edge e1, e2;
            try{
                e1.length = e2.length = boost::lexical_cast<double>(row[l]);
            }catch(...){
                std::cout << "longueur :( " << row[l] << std::endl;
            }

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

            boost::add_edge(source, target, e1, data.street_network.graph);
            boost::add_edge(target, source, e2, data.street_network.graph);

            std::string way_key;
            if(row[insee].substr(0,2) == "75")
                way_key = row[nom] + "75056";
            else
                way_key = row[nom] + row[insee];
            auto way_it = way_map.find(way_key);
            if(way_it == way_map.end()){
                way_map[way_key].name = row[nom];
                way_map[way_key].city = row[insee];
                auto city_it = city_map.find(row[insee]);
                if(city_it != city_map.end()){
                    way_map[way_key].city_idx = city_map[row[insee]];
                }
            }

            way_map[way_key].edges.push_back(std::make_pair(source, target));
            way_map[way_key].edges.push_back(std::make_pair(target, source));
        }

        unsigned int idx=0;

        BOOST_FOREACH(auto way, way_map){
            data.street_network.ways.push_back(way.second);
            data.street_network.ways.back().idx = idx;
            BOOST_FOREACH(auto node_pair, data.street_network.ways.back().edges){
                edge_t e = boost::edge(node_pair.first, node_pair.second, data.street_network.graph).first;
                data.street_network.graph[e].way_idx = idx;
            }
            idx++;
        }
    }



    std::unordered_map<std::string, idx_t> BDTopoParser::create_cities_indexes(const std::vector<nt::City>& cities) const{
        std::unordered_map<std::string, idx_t> city_map;
        std::for_each(cities.begin(), cities.end(), [&city_map](const nt::City& city){
            city_map[city.external_code] = city.idx;
        });
        return city_map;
    }


}} //fermeture des namespaces
