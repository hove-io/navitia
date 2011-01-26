#include <boost/assign/std/vector.hpp>
#include <boost/assign.hpp>
#include <vector>

struct Stop {
    int id;
    std::string name;
    std::string city;
    Stop() {};
    Stop(int id, const std::string & name, const std::string & city) : id(id), name(name), city(city) {}
};

struct Line {
    std::string name;
    double duration;
    Line(const std::string & name, double duration) : name(name), duration(duration) {}
};

struct City {
    std::string name;
    std::string country;
    City(){};
    City(const std::string & name, const std::string & country) : name(name), country(country) {}
};

struct LineStops {
    int stop_id;
    std::string line_name;
    int order;
    LineStops(int stop_id, const std::string & line_name, int order): stop_id(stop_id), line_name(line_name), order(order) {}
};

struct MyConfig {
    std::vector<Stop> stops;
    std::vector<Line> lines;
    std::vector<City> cities;
    std::vector<LineStops> line_stops;
    MyConfig(){ 
        stops.push_back(Stop(0, "Gare de l'Est", "Paris"));
        stops.push_back(Stop(1, "Gare d'Austeriltz", "Paris"));
        stops.push_back(Stop(2, "Westbahnhof", "Wien"));
        stops.push_back(Stop(3, "Hauptbahnof", "Stuttgart"));
        stops.push_back(Stop(4, "Centre du monde", "Perpinyà"));
        stops.push_back(Stop(5, "Sants", "Barcelona"));

        lines.push_back(Line("Orient Express", 15.5));
        lines.push_back(Line("Joan Miró", 12.1));
        lines.push_back(Line("Metro 5", 0.1));

        cities.push_back(City("Paris", "France"));
        cities.push_back(City("Wien", "Österreich"));
        cities.push_back(City("Stuttgart", "Deutschland"));
        cities.push_back(City("Barcelona", "España"));
        cities.push_back(City("Pepinyà", "France"));


        line_stops.push_back(LineStops(0, "Orient Express", 0));
        line_stops.push_back(LineStops(3, "Orient Express", 1));
        line_stops.push_back(LineStops(2, "Orient Express", 2));
        line_stops.push_back(LineStops(1, "Joan Miró", 1));
        line_stops.push_back(LineStops(5, "Joan Miró", 66));
        line_stops.push_back(LineStops(4, "Joan Miró", 42));
        line_stops.push_back(LineStops(0, "Metro 5", 1));
        line_stops.push_back(LineStops(1, "Metro 5", 2));
    }
    ~MyConfig()  {}
};
