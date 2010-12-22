#pragma once

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/bitset.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/date_time/gregorian/greg_serialize.hpp>
//#include <boost/serialization/shared_ptr.hpp>
#include <vector>
#include <bitset>

struct Country;
typedef boost::shared_ptr<Country> Country_ptr;
struct District;
typedef boost::shared_ptr<District> District_ptr;
struct Department;
typedef boost::shared_ptr<Department> Department_ptr;
struct City;
typedef boost::shared_ptr<City> City_ptr;
struct StopArea;
typedef boost::shared_ptr<StopArea> StopArea_ptr;
struct StopPoint;
typedef boost::shared_ptr<StopPoint> StopPoint_ptr;
struct Network;
typedef boost::shared_ptr<Network> Network_ptr;
struct Company;
typedef boost::shared_ptr<Company> Company_ptr;
struct ModeType;
typedef boost::shared_ptr<ModeType> ModeType_ptr;
struct Mode;
typedef boost::shared_ptr<Mode> Mode_ptr;
struct Line;
typedef boost::shared_ptr<Line> Line_ptr;
struct Route;
typedef boost::shared_ptr<Route> Route_ptr;
struct RoutePoint;
typedef boost::shared_ptr<RoutePoint> RoutePoint_ptr;
struct Vehicle;
typedef boost::shared_ptr<Vehicle> Vehicle_ptr;
struct VehicleJourney;
typedef boost::shared_ptr<VehicleJourney> VehicleJourney_ptr;
struct ValidityPattern;
typedef boost::shared_ptr<ValidityPattern> ValidityPattern_ptr;
struct StopTime;
typedef boost::shared_ptr<StopTime> StopTime_ptr;

struct Country {
    std::string name;
    City_ptr main_city;
    std::vector<District_ptr> district_list;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & main_city;// & district_list;
    }

};

struct District {
    std::string name;
    City_ptr main_city;
    Country_ptr country;
    std::vector<Department_ptr> department_list;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & main_city & country;// & department_list;
    }
};

struct Department {
    std::string name;
    City_ptr main_city;
    District_ptr district;
    std::vector<City_ptr> city_list;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & main_city & district;// & city_list;
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
    Department_ptr department;
    Coordinates coord;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & department & coord;
    }
};

struct StopArea {
    std::string code;
    std::string name;
    City_ptr city;
    Coordinates coord;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & code & name & city & coord;
    }
};

struct Network {
    std::string name;
    std::vector<Line_ptr> line_list;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name;// & line_list;
    }
};

struct Company {
    std::string name;
    std::vector<Line_ptr> line_list;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name;// & line_list;
    }
};

struct ModeType {
    std::string name;
    std::vector<Mode_ptr> mode_list;
    std::vector<Line_ptr> line_list;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name;// & mode_list & line_list;
    }
};

struct Mode {
    std::string name;
    ModeType_ptr mode_type;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & mode_type;
    }
};

struct Line {
    std::string name;
    std::string code;
    std::string mode;
    Network_ptr network;
    std::string forward_name;
    std::string backward_name;
    StopArea_ptr forward_thermo;
    StopArea_ptr backward_thermo;
    std::vector<ValidityPattern_ptr> validity_pattern_list; // Euh... y'en a plusieurs ?
    std::string additional_data;
    std::string color;
    int sort;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & code & mode & network & forward_name & backward_name & forward_thermo
                & backward_thermo /*& validity_pattern_list*/ & additional_data & color & sort;
    }
};

struct Route {
    std::string name;
    Line_ptr line;
    ModeType mode_type;
    bool is_frequence;
    bool is_forward;
    std::vector<RoutePoint_ptr> route_point_list;
    std::vector<VehicleJourney_ptr> vehicle_journey_list;
    bool is_adapted;
    Route_ptr associated_route;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & line & mode_type & is_frequence & is_forward /*& route_point_list &
                vehicle_journey_list & is_adapted & associated_route*/;
    }
};

struct RoutePoint {
    std::string external_code;
    int order;
    Route_ptr route;
    StopPoint_ptr stop_point;
    bool main_stop_point;
    int fare_section;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & external_code & order & route /*& stop_point*/ & main_stop_point & fare_section;
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
    Route_ptr route;
    Company_ptr company;
    Mode_ptr mode;
    //Vehicle_ptr vehicle;
    bool is_adapted;
    ValidityPattern_ptr validity_pattern;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & external_code;/* &route & company & mode & vehicle & is_adapted & validity_pattern*/;
    }
};

struct StopPoint {
    std::string code;
    std::string name;
    StopArea_ptr stop_area;
    Mode_ptr mode;
    Coordinates coord;
    int fare_zone;
    std::vector<Line_ptr> lines;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & code & name /*& stop_area*/ & mode & coord & fare_zone;// & lines;
    }
};

struct StopTime {
    int arrival_time; ///< En secondes depuis minuit
    int departure_time; ///< En secondes depuis minuit
    VehicleJourney_ptr vehicle_journey;
    StopPoint_ptr stop_point;
    int order;
    std::string comment;
    bool ODT;
    int zone;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & arrival_time & departure_time /*& vehicle_journey & stop_point*/ & order & comment & ODT & zone;
    }
};
