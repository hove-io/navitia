#pragma once
#include <boost/date_time/gregorian/gregorian.hpp>
#include <vector>
#include <bitset>

#include <boost/date_time/gregorian/greg_serialize.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>


namespace BO{namespace types{

typedef unsigned int idx_t;


struct Nameable{
    std::string name;
    std::string external_code;
};



struct NavitiaObject{
    int id;
    idx_t idx;
    NavitiaObject() : id(0), idx(0){};
};


struct GeographicalCoord{
    double x;
    double y;

    GeographicalCoord() : x(0), y(0) {}
};

//forward declare
class District;
class Department;
class City;
class Connection;
class StopArea;
class Network;
class Company;
class ModeType;
class Mode;
class Line;
class Route;
class VehicleJourney;
class Equipement;
class RoutePoint;
class StopPoint;
class StopTime;


struct Country : public NavitiaObject, Nameable{
    City* main_city;
    std::vector<District*> district_list;

};

struct District {
    idx_t idx;
    std::string name;
    idx_t main_city_idx;
    idx_t country_idx;
    std::vector<idx_t> department_list;
};

struct Department {
    idx_t idx;
    std::string name;
    idx_t main_city_idx;
    idx_t district_idx;
    std::vector<idx_t> city_list;
};


struct City : public NavitiaObject, Nameable {
    std::string main_postal_code;
    bool main_city;
    bool use_main_stop_area_property;

    Department* department;
    GeographicalCoord coord;

    std::vector<std::string> postal_code_list;
    std::vector<StopArea*> stop_area_list;
    std::vector<idx_t> address_list;
    std::vector<idx_t> site_list;
    std::vector<idx_t> stop_point_list;
    std::vector<idx_t> hang_list;
    std::vector<idx_t> odt_list;

    City() : main_city(false), use_main_stop_area_property(false) {};


};

struct Connection {
    idx_t departure_stop_point_idx;
    idx_t destination_stop_point_idx;
    int duration;
    int max_duration;
    idx_t comment_idx;

    Connection() : departure_stop_point_idx(0), destination_stop_point_idx(0), duration(0),
        max_duration(0), comment_idx(0){};

};

struct StopArea : public NavitiaObject, Nameable{
    GeographicalCoord coord;
    int properties;
    std::string additional_data;

    std::vector<idx_t> stop_area_list;
    std::vector<idx_t> stop_point_list;
    idx_t city_idx;

};

struct Network : public NavitiaObject, Nameable{
    std::string address_name;
    std::string address_number;
    std::string address_type_name;
    std::string phone_number;
    std::string mail;
    std::string website;
    std::string fax;

    std::vector<Line*> line_list;


};

struct Company : public NavitiaObject, Nameable{
    idx_t city_idx;
    std::string address_name;
    std::string address_number;
    std::string address_type_name;
    std::string phone_number;
    std::string mail;
    std::string website;
    std::string fax;

    std::vector<idx_t> line_list;

};

struct ModeType : public NavitiaObject, Nameable{
    std::vector<Mode*> mode_list;
    std::vector<Line*> line_list;
};

struct Mode : public NavitiaObject, Nameable{
    ModeType* mode_type;

    Mode(): mode_type(NULL){}
};

struct Line : public NavitiaObject, Nameable {
    std::string code;
    std::string forward_name;
    std::string backward_name;

    idx_t comment_idx;
    std::string additional_data;
    std::string color;
    int sort;
    
    ModeType* mode_type;

    std::vector<Mode*> mode_list;
    std::vector<Company*> company_list;

    Network* network;
    /// StopPoint
    idx_t forward_direction;

    /// StopPoint
    idx_t backward_direction;


    std::vector<idx_t> forward_route;
    std::vector<idx_t> backward_route;

    std::vector<idx_t> validity_pattern_list;

    Line(): sort(0), mode_type(NULL), network(NULL) {}

    bool operator<(const Line & other) const { return name > other.name;}

};

struct Route : public NavitiaObject, Nameable{
    idx_t comment_idx;
    bool is_frequence;
    bool is_forward;
    bool is_adapted;
    idx_t line_idx;
    idx_t mode_type_idx;
    idx_t associated_route_idx;
    
    std::vector<idx_t> route_point_list;
    std::vector<idx_t> freq_route_point_list;
    std::vector<idx_t> freq_setting_list;
    std::vector<idx_t> vehicle_journey_list;

    Route(): is_frequence(false), is_forward(false), is_adapted(false), line_idx(0), associated_route_idx(0){};

 };
struct VehicleJourney {
    idx_t idx;
    std::string name;
    std::string external_code;
    idx_t route_idx;
    idx_t company_idx;
    idx_t mode_idx;
    idx_t vehicle_idx;
    bool is_adapted;
    idx_t validity_pattern_idx;

    VehicleJourney(): idx(0), route_idx(0), company_idx(0), mode_idx(0), vehicle_idx(0), is_adapted(false), validity_pattern_idx(0){};
};

struct Equipement : public NavitiaObject {
    enum EquipementKind{ Sheltred, 
                            MIPAccess, 
                            Escalator, 
                            BikeAccepted, 
                            BikeDepot, 
                            VisualAnnouncement, 
                            AudibleAnnoucement,
                            AppropriateEscort, 
                            AppropriateSignage
    };

    std::bitset<9> equipement_kind;
    
};

struct RoutePoint : public NavitiaObject{
    std::string external_code;
    int order;
    bool main_stop_point;
    idx_t comment_idx;
    int fare_section;
    idx_t route_idx;
    idx_t stop_point_idx;


    RoutePoint() : main_stop_point(false){}

};

struct ValidityPattern {
private:
    boost::gregorian::date beginning_date;
    std::bitset<366> days;
    bool is_valid(int duration);
public:
    idx_t idx;
    ValidityPattern() : idx(0) {}
    ValidityPattern(boost::gregorian::date beginning_date) : beginning_date(beginning_date), idx(0){}
    void add(boost::gregorian::date day);
    void add(int day);
    void add(boost::gregorian::date start, boost::gregorian::date end, std::bitset<7> active_days);
    void remove(boost::gregorian::date day);
    void remove(int day);

};

struct StopPoint : public NavitiaObject, Nameable{
    GeographicalCoord coord;
    idx_t comment_idx;
    int fare_zone;

    std::string address_name;
    std::string address_number;
    std::string address_type_name;

    idx_t stop_area_idx;
    idx_t city_idx;
    idx_t mode_idx;
    idx_t network_idx;
};

struct StopTime {
    int idx;
    int arrival_time; ///< En secondes depuis minuit
    int departure_time; ///< En secondes depuis minuit
    int vehicle_journey_idx;
    int stop_point_idx;
    int order;
    idx_t comment_idx;
    bool ODT;
    int zone;
};



enum PointType{ptCity, ptSite, ptAddress, ptStopArea, ptAlias, ptUndefined, ptSeparator};
enum Criteria{cInitialization, cAsSoonAsPossible, cLeastInterchange, cLinkTime, cDebug, cWDI};


}}//end namespace BO::Type
