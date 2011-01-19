#include "indexes.h"
#include <iostream>
#include <boost/foreach.hpp>
#include <boost/assign/std/vector.hpp>
#include <boost/assign.hpp>
#include <vector>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <fstream>
#include <boost/serialization/vector.hpp>

struct Stop {
    int id;
    std::string name;
    std::string city;
    Stop(){}
    Stop(int id, const std::string & name, const std::string & city) : id(id), name(name), city(city) {}
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & id & name & city;
    }
};

struct Line {
    std::string name;
    float duration;
    Line(const std::string & name, float duration) : name(name), duration(duration) {}
};

struct City {
    std::string name;
    std::string country;
    City(const std::string & name, const std::string & country) : name(name), country(country) {}
};

struct LineStops {
    int stop_id;
    std::string line_name;
    int order;
    LineStops(int stop_id, const std::string & line_name, int order): stop_id(stop_id), line_name(line_name), order(order) {}
};

using namespace boost::assign;

int main(int, char**) {
    std::vector<Stop> stops;
    std::vector<Line> lines;
    std::vector<City> cities;
    std::vector<LineStops> line_stops;

    stops += Stop(0, "Gare de l'Est", "Paris"),
    Stop(1, "Gare d'Austeriltz", "Paris"),
    Stop(2, "Westbahnhof", "Wien"),
    Stop(3, "Hauptbahnof", "Stuttgart"),
    Stop(4, "Centre du monde", "Perpinyà"),
    Stop(5, "Sants", "Barcelona");

    lines += Line("Orient Express", 15.5),
    Line("Joan Miró", 12.1),
    Line("Metro 5", 0.1);

    cities += City("Paris", "France"),
    City("Wien", "Österreich"),
    City("Stuttgart", "Deutschland"),
    City("Barcelona", "España"),
    City("Pepinyà", "France");

    line_stops += LineStops(0, "Orient Express", 0),
    LineStops(3, "Orient Express", 1),
    LineStops(2, "Orient Express", 2),
    LineStops(1, "Joan Miró", 1),
    LineStops(5, "Joan Miró", 66),
    LineStops(4, "Joan Miró", 42),
    LineStops(0, "Metro 5", 1),
    LineStops(1, "Metro 5", 2);


    std::cout << "=== Cities ordered by name ===" << std::endl;
    BOOST_FOREACH(auto city, order(cities, &City::name)){
        std::cout << "  * " << city.name << " (" << city.country << ")" << std::endl;
    }
    std::cout << std::endl << std::endl;

    std::cout << "=== Stations of Paris ===" << std::endl;
    BOOST_FOREACH(auto stop, filter(stops, &Stop::city, std::string("Paris"))){
        std::cout << "  * " << stop.name << std::endl;
    }
    std::cout << std::endl << std::endl;

    std::cout << "=== Stops of line Orient Express ===" << std::endl;
    auto join = make_join(stops,
                          filter(line_stops, &LineStops::line_name, std::string("Orient Express")),
                          attribute_equals(&Stop::id, &LineStops::stop_id));
    BOOST_FOREACH(auto stop, join){
        std::cout << "  * " << join_get<Stop>(stop).name << " " << join_get<LineStops>(stop).order << std::endl;
    }

    std::cout << "=== Stops of line Orient Express (bis) ===" << std::endl;
    auto join2 = make_join(filter(line_stops, &LineStops::line_name, std::string("Orient Express")),
                           stops,
                           attribute_equals(&LineStops::stop_id, &Stop::id));
    BOOST_FOREACH(auto stop, join2){
        std::cout << "  * " << join_get<Stop>(stop).name << " " << join_get<LineStops>(stop).order << std::endl;
    }

    std::cout << "=== Stops of line Orient Express in Paris ===" << std::endl;
    auto join3 = make_join(filter(line_stops, &LineStops::line_name, std::string("Orient Express")),
                           filter(stops, &Stop::city, std::string("Paris")),
                           attribute_equals(&LineStops::stop_id, &Stop::id));
    BOOST_FOREACH(auto stop, join3){
        std::cout << "  * " << join_get<Stop>(stop).name << " " << join_get<LineStops>(stop).order << std::endl;
    }

    std::cout << "=== (Stops, Lines) couples ===" << std::endl;
    auto join4 = make_join(line_stops,
                           stops,
                           attribute_equals(&LineStops::stop_id, &Stop::id));
    BOOST_FOREACH(auto stop, join4){
        std::cout << "  * " << join_get<Stop>(stop).name << ", " << join_get<LineStops>(stop).line_name << std::endl;
    }

    std::cout << "=== Lines stoping in France ===" << std::endl;
    auto join6 = make_join(filter(cities, &City::country, std::string("France")), stops, attribute_equals(&City::name, &Stop::city));
    auto join5 = make_join( line_stops,
                            join6,
                            attribute_equals(&LineStops::stop_id, &Stop::id));
    BOOST_FOREACH(auto stop, join5){
        std::cout << "  * " << join_get<LineStops>(stop).line_name << ", " << join_get<Stop>(stop).name << std::endl;
    }

    Index<Stop> paris_stops_idx = make_index(filter(stops, &Stop::city, std::string("Paris")));
    std::cout << "=== Stations of Paris ===" << std::endl;
    BOOST_FOREACH(auto stop, paris_stops_idx){
        std::cout << "  * " << stop.name << std::endl;
    }
    std::cout << std::endl << std::endl;

    {
        std::ofstream ofs("idx_serialize",  std::ios::out | std::ios::binary);
        boost::archive::binary_oarchive oa(ofs);
        oa << stops << paris_stops_idx;
    }

    decltype(paris_stops_idx) paris_stops_idx2;
    std::vector<Stop> stops2;

    std::ifstream ifs("idx_serialize",  std::ios::in | std::ios::binary);
    boost::archive::binary_iarchive ia(ifs);
    ia >> stops2 >> paris_stops_idx2;

    std::cout << "=== Stations of Paris ===" << std::endl;
    BOOST_FOREACH(auto stop, paris_stops_idx2){
        std::cout << "  * " << stop.name << std::endl;
    }
    std::cout << std::endl << std::endl;

    std::cout << "=== Stations by alphabetical order (and indexed by their name) ===" << std::endl;
    StringIndex<Stop> stops_str_idx = make_string_index(stops, &Stop::name);
    BOOST_FOREACH(auto stop, stops_str_idx) {
        std::cout << "  * " << stop.name << std::endl;
    }
    std::cout << "Sants lies in " << stops_str_idx["Sants"].city << std::endl << std::endl;

    std::cout << "=== Stations and their country ===" << std::endl;
    auto join_idx = make_join_index(join6);
    typedef boost::tuple<City*, Stop*> elt_t;
    BOOST_FOREACH(elt_t city_stop, join_idx) {
        std::cout << "  * " << join_get<Stop>(city_stop).name << " (" << join_get<City>(city_stop).country << ")" << std::endl;
    }
    return 0;
}
