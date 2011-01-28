#include "gtfs_parser.h"
#include "indexes.h"

#include <fstream>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace pt = boost::posix_time;

template<class T1, class T2>
struct True{
    bool operator()(const T1 &, const T2 &) {
        return true;
    }
};

template<class T1, class T2> bool true2(){return true;}

int main(int, char **) {
    std::cout << "Benchmarks de parsage GTFS, Sérialisation et Indexes" << std::endl << std::endl;
    std::cout << "Chargement des données GTFS de l'IdF" << std::endl;
    pt::ptime start(pt::microsec_clock::local_time());
    GtfsParser p("/home/tristram/mumoro/idf_gtfs", "20101101");
    Data data = p.getData();
    pt::ptime end(pt::microsec_clock::local_time());
    std::cout << "Durée pour lire les données de l'IdF depuis le GTFS : " << (end-start).total_milliseconds() << " ms" << std::endl << std::endl;

    start = pt::microsec_clock::local_time();
    auto sa_name_idx = make_string_index(data.stop_areas, &StopArea::name);
    end = pt::microsec_clock::local_time();
    std::cout << "Durée pour créer un indexe sur StopArea::name : " << (end-start).total_milliseconds() << " ms" << std::endl;

    start = pt::microsec_clock::local_time();
    auto st_by_dep_idx = make_index(order(data.stop_times, &StopTime::departure_time));
    end = pt::microsec_clock::local_time();
    std::cout << "Durée pour créer un indexe les StopTime trié par departure_time : " << (end-start).total_milliseconds() << " ms" << std::endl;

    start = pt::microsec_clock::local_time();
    int count = 0;
    int total_count = 0;
    BOOST_FOREACH(StopTime tp, st_by_dep_idx){
        total_count++;
        if(tp.arrival_time > 9*3600 && tp.arrival_time < 19*3600)
            count++;
    }
    end = pt::microsec_clock::local_time();
    std::cout << "Durée pour parcourir tous les stop times via un indexe et faire un test bidon (nombre d'horaires entre 9h et 19h) : " << (end-start).total_milliseconds() << " ms (" << count << "/" << total_count<< ")"<< std::endl;


start = pt::microsec_clock::local_time();
    {
        std::ofstream ofs("data.nav");
        boost::archive::binary_oarchive oa(ofs);
        oa << data;
    }
    end = pt::microsec_clock::local_time();
    std::cout << "Durée pour sérialiser les données uniqumeent : " << (end-start).total_milliseconds() << " ms" << std::endl;

    start = pt::microsec_clock::local_time();
    {
        std::ifstream ifs("data.nav");
        boost::archive::binary_iarchive ia(ifs);
        Data data2;
        decltype(st_by_dep_idx) index;
        ia >> data2;
    }
    std::cout << "Durée pour dé-sérialiser les données uniqumeent : " << (end-start).total_milliseconds() << " ms" << std::endl;

    start = pt::microsec_clock::local_time();
    {
        std::ofstream ofs("data2.nav");
        boost::archive::binary_oarchive oa(ofs);
        oa << data << st_by_dep_idx;
    }
    end = pt::microsec_clock::local_time();
    std::cout << "Durée pour sérialiser les données et l'indexe sur stop_times : " << (end-start).total_milliseconds() << " ms" << std::endl;

    start = pt::microsec_clock::local_time();
    {
        std::ifstream ifs("data2.nav");
        boost::archive::binary_iarchive ia(ifs);
        Data data2;
        decltype(st_by_dep_idx) index;
        ia >> data2 >> index;
    }
    end = pt::microsec_clock::local_time();
    std::cout << "Durée pour dé-sérialiser les données et l'indexe' : " << (end-start).total_milliseconds() << " ms" << std::endl;

}
