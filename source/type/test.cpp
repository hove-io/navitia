#include "gtfs_parser.h"
#include "indexes.h"

#include <fstream>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
//#include <boost/iostreams/filter/gzip.hpp>
//#include <boost/iostreams/filter/bzip2.hpp>
//#include <boost/iostreams/filter/zlib.hpp>

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
    pt::ptime end;

    GtfsParser p("/home/tristram/idf_gtfs", "20101101");
    Data data = p.getData();
    end = pt::microsec_clock::local_time();
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
        if(tp.departure_time >= 11*3600 && tp.departure_time <= 12*3600)
            count++;
    }
    end = pt::microsec_clock::local_time();
    std::cout << "Durée pour parcourir tous les stop times via un indexe et faire un test bidon (nombre d'horaires entre 11h et 12h) : " << (end-start).total_milliseconds() << " ms (" << count << "/" << total_count<< ")"<< std::endl;

    start = pt::microsec_clock::local_time();
    auto low = std::lower_bound(st_by_dep_idx.begin(), st_by_dep_idx.end(), 11*3600, attribute_cmp(&StopTime::departure_time));
    auto up = std::upper_bound(st_by_dep_idx.begin(), st_by_dep_idx.end(), 12*3600, attribute_cmp(&StopTime::departure_time));
    count = 0;
    total_count = 0;
    BOOST_FOREACH(StopTime tp, std::make_pair(low, up)){
        total_count++;
        if(tp.departure_time >= 11*3600 && tp.departure_time <= 12*3600)
            count++;
    }
    end = pt::microsec_clock::local_time();
    std::cout << "Durée pour parcourir tous les stop times via un indexe + dichotomie et faire un test bidon (nombre d'horaires entre 11h et 12h) : " << (end-start).total_milliseconds() << " ms (" << count << "/" << total_count<< ")"<< std::endl;


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
        ia >> data2;
        BOOST_ASSERT(data2.stop_times.size() == data.stop_times.size());
        BOOST_ASSERT(data2.stop_areas.size() == data.stop_areas.size());
        BOOST_ASSERT(data2.stop_points.size() == data.stop_points.size());
    }
    end = pt::microsec_clock::local_time();
    std::cout << "Durée pour dé-sérialiser les données uniqumeent : " << (end-start).total_milliseconds() << " ms" << std::endl;
/*
    start = pt::microsec_clock::local_time();
    {
        std::ofstream ofs("data.nav.gz",std::ios::out|std::ios::binary|std::ios::trunc);
        boost::iostreams::filtering_streambuf<boost::iostreams::output> out;
        out.push(boost::iostreams::gzip_compressor());
        out.push(ofs);
        boost::archive::binary_oarchive oa(out);
        oa << data;
    }
    end = pt::microsec_clock::local_time();
    std::cout << "Durée pour sérialiser les données uniquement avec compression gz : " << (end-start).total_milliseconds() << " ms" << std::endl;

    start = pt::microsec_clock::local_time();
    {
        std::ifstream ifs("data.nav.gz", std::ios::in|std::ios::binary);
        boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
        in.push(boost::iostreams::gzip_decompressor());
        in.push(ifs);
        Data data2;
        boost::archive::binary_iarchive ia(in);
        ia >> data2;
        BOOST_ASSERT(data2.stop_times.size() == data.stop_times.size());
        BOOST_ASSERT(data2.stop_areas.size() == data.stop_areas.size());
        BOOST_ASSERT(data2.stop_points.size() == data.stop_points.size());
    }
    end = pt::microsec_clock::local_time();
    std::cout << "Durée pour dé-sérialiser les données avec compression gz : " << (end-start).total_milliseconds() << " ms" << std::endl;

    start = pt::microsec_clock::local_time();
    {
        std::ofstream ofs("data.nav.Z",std::ios::out|std::ios::binary|std::ios::trunc);
        boost::iostreams::filtering_streambuf<boost::iostreams::output> out;
        out.push(boost::iostreams::zlib_compressor());
        out.push(ofs);
        boost::archive::binary_oarchive oa(out);
        oa << data;
    }
    end = pt::microsec_clock::local_time();
    std::cout << "Durée pour sérialiser les données uniquement avec compression Z : " << (end-start).total_milliseconds() << " ms" << std::endl;

    start = pt::microsec_clock::local_time();
    {
        std::ifstream ifs("data.nav.Z", std::ios::in|std::ios::binary);
        boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
        in.push(boost::iostreams::zlib_decompressor());
        in.push(ifs);
        Data data2;
        boost::archive::binary_iarchive ia(in);
        ia >> data2;
        BOOST_ASSERT(data2.stop_times.size() == data.stop_times.size());
        BOOST_ASSERT(data2.stop_areas.size() == data.stop_areas.size());
        BOOST_ASSERT(data2.stop_points.size() == data.stop_points.size());
    }
    end = pt::microsec_clock::local_time();
    std::cout << "Durée pour dé-sérialiser les données avec compression Z : " << (end-start).total_milliseconds() << " ms" << std::endl;
    start = pt::microsec_clock::local_time();
    {
        std::ofstream ofs("data.nav.bz2",std::ios::out|std::ios::binary|std::ios::trunc);
        boost::iostreams::filtering_streambuf<boost::iostreams::output> out;
        out.push(boost::iostreams::bzip2_compressor());
        out.push(ofs);
        boost::archive::binary_oarchive oa(out);
        oa << data;
    }
    end = pt::microsec_clock::local_time();
    std::cout << "Durée pour sérialiser les données uniquement avec compression bz2 : " << (end-start).total_milliseconds() << " ms" << std::endl;

    start = pt::microsec_clock::local_time();
    {
        std::ifstream ifs("data.nav.bz2", std::ios::in|std::ios::binary);
        boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
        in.push(boost::iostreams::bzip2_decompressor());
        in.push(ifs);
        Data data2;
        boost::archive::binary_iarchive ia(in);
        ia >> data2;
        BOOST_ASSERT(data2.stop_times.size() == data.stop_times.size());
        BOOST_ASSERT(data2.stop_areas.size() == data.stop_areas.size());
        BOOST_ASSERT(data2.stop_points.size() == data.stop_points.size());
    }
    end = pt::microsec_clock::local_time();
    std::cout << "Durée pour dé-sérialiser les données avec compression bz2 : " << (end-start).total_milliseconds() << " ms" << std::endl;
*/
    start = pt::microsec_clock::local_time();
    auto lyon = make_index(filter(data.stop_areas, matches(&StopArea::name, ".*Lyon.*")));
    auto lyon_sp = make_join(data.stop_points, lyon, attribute_equals(&StopPoint::stop_area_idx, &StopArea::idx));
    count = 0;
    BOOST_FOREACH(auto l, lyon_sp){
        //std::cout  << join_get<StopArea>(l).name << " " << join_get<StopPoint>(l).name << std::endl;
        count++;
    }
    end = pt::microsec_clock::local_time();
    std::cout << "Durée pour trouver les stopPoints dont le stopArea contient Lyon: " << (end-start).total_milliseconds() << " ms (" << count << ")" << std::endl;
}
