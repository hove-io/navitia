#include "types.h"
#include "utils/csv.h"
#include "../first_letter/first_letter.h"

#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/iostreams/filtering_streambuf.hpp>

#include <fstream>
#include "fastlz_filter/filter.h"
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include "eos_portable_archive/portable_iarchive.hpp"
#include "eos_portable_archive/portable_oarchive.hpp"


#include <iostream>
#include <unordered_map>

namespace pt = boost::posix_time;
using navitia::type::City;
namespace navitia{ namespace streetnetwork{


void StreetNetwork::load_bd(const std::string& path){
    load_bdcity(path + "/commune.txt");
    load_bdtopo(path + "/route_adresse.txt");
}

void StreetNetwork::load_bdtopo(std::string filename) {
    CsvReader reader(filename);
    std::map<std::string, int> cols;

    std::vector<std::string> row = reader.next();
    for(size_t i=0; i < row.size(); i++){
        cols[row[i]] = i;
    }

    size_t nom = cols["Etiquette"];
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

            source = vertex_map[row[x1] + row[y1]] = boost::add_vertex(v, graph);
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
            target = vertex_map[row[x2] + row[y2]] = boost::add_vertex(v, graph);
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
            std::cout << row[n_deb_d] << ", " << row[n_fin_d] << ", " << row[n_deb_g] << ", " << row[n_fin_g] << std::endl;
        }

        boost::add_edge(source, target, e1, graph);
        boost::add_edge(target, source, e2, graph);

        std::string way_key = row[nom] + row[insee];
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
    FirstLetter<unsigned int> fl;
    BOOST_FOREACH(auto way, way_map){
        ways.push_back(way.second);
        ways.back().idx = idx;
        BOOST_FOREACH(auto node_pair, ways.back().edges){
            edge_t e = boost::edge(node_pair.first, node_pair.second, graph).first;
            graph[e].way_idx = idx;
        }
        fl.add_string(ways.back().name, idx);
        idx++;
    }
    fl.build();
}

void StreetNetwork::load_bdcity(std::string filename){
    
    CsvReader reader(filename);
    std::map<std::string, int> cols;

    std::vector<std::string> row = reader.next();
    for(size_t i=0; i < row.size(); i++){
        cols[row[i]] = i;
    }

    size_t name = cols["NOM"];
    size_t insee = cols["CODE_INSEE"];
    
    idx_t counter = 0;
    for(reader.next(); !reader.eof() ;row = reader.next()){
        if(row.size() < 2)
            continue;
        City city;
        city.idx = counter;
        city.id = row[insee];
        city.name = row[name];
        cities.push_back(city);
        city_map[city.id] = city.idx;
        counter++;
        std::cout << row[insee] << " " << row[name] << std::endl;
    }

}



void StreetNetwork::save(const std::string & filename) {
    std::ofstream ofs(filename.c_str());
    boost::archive::text_oarchive oa(ofs);
    oa << *this;
}

void StreetNetwork::load(const std::string & filename) {
    std::ifstream ifs(filename.c_str());
    boost::archive::text_iarchive ia(ifs);
    ia >> *this;
}

void StreetNetwork::save_bin(const std::string & filename) {
    std::ofstream ofs(filename.c_str(),  std::ios::out | std::ios::binary);
    boost::archive::binary_oarchive oa(ofs);
    oa << *this;
}

void StreetNetwork::load_bin(const std::string & filename) {
    std::ifstream ifs(filename.c_str(),  std::ios::in | std::ios::binary);
    boost::archive::binary_iarchive ia(ifs);
    ia >> *this;
}

void StreetNetwork::load_flz(const std::string & filename) {
    std::ifstream ifs(filename.c_str(),  std::ios::in | std::ios::binary);
    boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
    in.push(FastLZDecompressor(2048*500),8192*500, 8192*500);
    in.push(ifs);
    eos::portable_iarchive ia(in);
    ia >> *this;
}

void StreetNetwork::save_flz(const std::string & filename) {
    std::ofstream ofs(filename.c_str(),std::ios::out|std::ios::binary|std::ios::trunc);
    boost::iostreams::filtering_streambuf<boost::iostreams::output> out;
    out.push(FastLZCompressor(2048*500), 1024*500, 1024*500);
    out.push(ofs);
    eos::portable_oarchive oa(out);
    oa << *this;
}

}}
