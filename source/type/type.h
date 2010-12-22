#pragma once

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/bitset.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/date_time/gregorian/greg_serialize.hpp>
#include <vector>
#include <bitset>

struct Country;
struct District;
struct Department;
struct City;
struct StopArea;
struct StopPoint;
struct Network;
struct Company;
struct ModeType;
struct Mode;
struct Line;
struct Route;
struct RoutePoint;
struct Vehicle;
struct VehicleJourney;
struct ValidityPattern;
struct StopTime;

struct Country {
    std::string name;
    int main_city_idx;
    std::vector<int> district_list;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & main_city_idx & district_list;
    }

};

struct District {
    std::string name;
    int main_city_idx;
    int country_idx;
    std::vector<int> department_list;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & main_city_idx & country_idx & department_list;
    }
};

struct Department {
    std::string name;
    int main_city_idx;
    int district_idx;
    std::vector<int> city_list;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & main_city_idx & district_idx & city_list;
    }
};

struct Coordinates {
    double x, y;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & x & y;
    }
};

struct City {
    std::string name;
    int department_idx;
    Coordinates coord;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & department_idx & coord;
    }
};

struct StopArea {
    std::string code;
    std::string name;
    int city_idx;
    Coordinates coord;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & code & name & city_idx & coord;
    }
};

struct Network {
    std::string name;
    std::vector<int> line_list;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & line_list;
    }
};

struct Company {
    std::string name;
    std::vector<int> line_list;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & line_list;
    }
};

struct ModeType {
    std::string name;
    std::vector<int> mode_list;
    std::vector<int> line_list;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & mode_list & line_list;
    }
};

struct Mode {
    std::string name;
    int mode_type_idx;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & mode_type_idx;
    }
};

struct Line {
    std::string name;
    std::string code;
    std::string mode;
    int network_idx;
    std::string forward_name;
    std::string backward_name;
    int forward_thermo_idx;
    int backward_thermo_idx;
    std::vector<int> validity_pattern_list; // Euh... y'en a plusieurs ?
    std::string additional_data;
    std::string color;
    int sort;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & code & mode & network_idx & forward_name & backward_name & forward_thermo_idx
                & backward_thermo_idx & validity_pattern_list & additional_data & color & sort;
    }
};

struct Route {
    std::string name;
    int line_idx;
    ModeType mode_type;
    bool is_frequence;
    bool is_forward;
    std::vector<int> route_point_list;
    std::vector<int> vehicle_journey_list;
    bool is_adapted;
    int associated_route_idx;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & line_idx & mode_type & is_frequence & is_forward & route_point_list &
                vehicle_journey_list & is_adapted & associated_route_idx;
    }
};

struct RoutePoint {
    std::string external_code;
    int order;
    int route_idx;
    int stop_point_idx;
    bool main_stop_point;
    int fare_section;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & external_code & order & route_idx & stop_point_idx & main_stop_point & fare_section;
    }
};

struct ValidityPattern {
private:
    boost::gregorian::date beginning_date;
    std::bitset<366> days;
    bool is_valid(int duration);
public:
    ValidityPattern() {}
    ValidityPattern(boost::gregorian::date beginning_date) : beginning_date(beginning_date) {}
    void add(boost::gregorian::date day);
    void add(int day);
    void add(boost::gregorian::date start, boost::gregorian::date end, std::bitset<7> active_days);
    void remove(boost::gregorian::date day);
    void remove(int day);
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & beginning_date & days;
    }

};

struct VehicleJourney {
    std::string name;
    std::string external_code;
    int route_idx;
    int company_idx;
    int mode_idx;
    int vehicle_idx;
    bool is_adapted;
    int validity_pattern_idx;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & external_code & route_idx & company_idx & mode_idx & vehicle_idx & is_adapted & validity_pattern_idx;
    }
};

struct StopPoint {
    std::string code;
    std::string name;
    int stop_area_idx;
    int mode_idx;
    Coordinates coord;
    int fare_zone;
    std::vector<int> lines;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & code & name & stop_area_idx & mode_idx & coord & fare_zone & lines;
    }
};

struct StopTime {
    int arrival_time; ///< En secondes depuis minuit
    int departure_time; ///< En secondes depuis minuit
    int vehicle_journey_idx;
    int stop_point_idx;
    int order;
    std::string comment;
    bool ODT;
    int zone;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & arrival_time & departure_time & vehicle_journey_idx & stop_point_idx & order & comment & ODT & zone;
    }
};
