#pragma once
#include <boost/date_time/gregorian/gregorian.hpp>
#include <vector>
#include <bitset>

#include <boost/date_time/gregorian/greg_serialize.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>


namespace navimake{namespace types{

typedef unsigned int idx_t;


struct Nameable{
    std::string name;
};



struct NavitiaObject{
    int id;
    idx_t idx;
    std::string external_code;
    std::string comment;
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
class ValidityPattern;
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

    City() : main_city(false), use_main_stop_area_property(false), department(NULL) {};


};

struct Connection: public NavitiaObject {
    enum ConnectionKind{
        AddressConnection,           //Jonction adresse / arrêt commercial
        SiteConnection,              //Jonction Lieu public / arrêt commercial
        StopAreaConnection,          //Correspondance intra arrêt commercial, entre 2 arrêts phy distincts
        StopPointConnection,         //Correspondance intra-arrêt phy
        VehicleJourneyConnection,    //Liaison en transport en commun
        ProlongationConnection,      //Liaison en transport en commun / prolongement de service
        LinkConnection,              //Liaison marche à pied
        WalkConnection,              //Trajet a pied
        PersonnalCarConnection,      //Liaison en transport personnel (voiture)
        UndefinedConnection,
        BicycleConnection,
        CabConnection,
        ODTConnection,
        VLSConnection,
        DefaultConnection
    };

    StopPoint* departure_stop_point;
    StopPoint* destination_stop_point;
    int duration;
    int max_duration;
    ConnectionKind connection_kind;

    Connection() : departure_stop_point(NULL), destination_stop_point(NULL), duration(0),
        max_duration(0), connection_kind(DefaultConnection){}

};

struct StopArea : public NavitiaObject, Nameable{
    GeographicalCoord coord;
    int properties;
    std::string additional_data;

    bool main_stop_area;
    bool main_connection;

    StopArea(): properties(0), main_stop_area(false), main_connection(false) {}

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
    bool is_frequence;
    bool is_forward;
    bool is_adapted;
    Line* line;
    Mode* mode;   

    Route(): is_frequence(false), is_forward(false), is_adapted(false), line(NULL), mode(NULL){};

 };
struct VehicleJourney: public NavitiaObject, Nameable{
    Route* route;
    Company* company;
    Mode* mode;
    //Vehicle* vehicle;
    bool is_adapted;

    ValidityPattern* validity_pattern;


    VehicleJourney(): route(NULL), company(NULL), mode(NULL), is_adapted(false), validity_pattern(NULL){};
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
    int fare_section;
    idx_t route_idx;
    idx_t stop_point_idx;


    RoutePoint() : main_stop_point(false){}

};

struct ValidityPattern: public NavitiaObject {
private:
    boost::gregorian::date beginning_date;
    std::bitset<366> days;
    bool is_valid(int duration);
public:
    ValidityPattern(){}
    ValidityPattern(boost::gregorian::date beginning_date) : beginning_date(beginning_date){}
    void add(boost::gregorian::date day);
    void add(int day);
    void add(boost::gregorian::date start, boost::gregorian::date end, std::bitset<7> active_days);
    void remove(boost::gregorian::date day);
    void remove(int day);

};

struct StopPoint : public NavitiaObject, Nameable{
    GeographicalCoord coord;
    int fare_zone;

    std::string address_name;
    std::string address_number;
    std::string address_type_name;

    StopArea* stop_area;
    Mode* mode;
    City* city;

    StopPoint(): fare_zone(0), stop_area(NULL), mode(NULL), city(NULL) {}
};

struct StopTime: public NavitiaObject {
    int arrival_time; ///< En secondes depuis minuit
    int departure_time; ///< En secondes depuis minuit
    VehicleJourney* vehicle_journey;
    StopPoint* stop_point;
    int order;
    bool ODT;
    int zone;

    StopTime(): arrival_time(0), departure_time(0), vehicle_journey(NULL), stop_point(NULL), order(0), 
        ODT(false), zone(0){}
};



enum PointType{ptCity, ptSite, ptAddress, ptStopArea, ptAlias, ptUndefined, ptSeparator};
enum Criteria{cInitialization, cAsSoonAsPossible, cLeastInterchange, cLinkTime, cDebug, cWDI};


}}//end namespace navimake::types
