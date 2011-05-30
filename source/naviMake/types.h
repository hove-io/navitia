#pragma once
#include <boost/date_time/gregorian/gregorian.hpp>
#include <vector>
#include <bitset>

#include <boost/date_time/gregorian/greg_serialize.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "type/type.h"

namespace nt = navitia::type;

namespace navimake{namespace types{

typedef unsigned int idx_t;


struct Nameable{
    std::string name;
    std::string comment;
};



struct TransmodelHeader{
    int id;
    idx_t idx;
    std::string external_code;
    TransmodelHeader() : id(0), idx(0){}
};


struct GeographicalCoord{
    double x;
    double y;

    GeographicalCoord() : x(0), y(0) {}
    
    nt::GeographicalCoord transform() const;
    
};
//typedef navitia::type::GeographicalCoord GeographicalCoord;

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


struct Country : public TransmodelHeader, Nameable{
    City* main_city;
    bool operator<(const Country& other){ return this->name < other.name;}

};

struct District: public TransmodelHeader, Nameable{
    City* main_city;
    Country* country;

    bool operator<(const District& other) const;
};

struct Department: public TransmodelHeader, Nameable{
    City* main_city;
    District district;

    bool operator<(const Department& other) const;
};


struct City : public TransmodelHeader, Nameable {
    std::string main_postal_code;
    bool main_city;
    bool use_main_stop_area_property;

    Department* department;
    GeographicalCoord coord;

    std::vector<std::string> postal_code_list;
    std::vector<StopArea*> stop_area_list;
    

    struct Transformer{
        inline navitia::type::City operator()(const City* city){return this->operator()(*city);}
        navitia::type::City operator()(const City& city);
    };

    City() : main_city(false), use_main_stop_area_property(false), department(NULL) {};

    bool operator<(const City& other) const {return this->name < other.name;}


};

struct Connection: public TransmodelHeader {
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

    struct Transformer{
        inline navitia::type::Connection operator()(const Connection* connection){return this->operator()(*connection);}
        navitia::type::Connection operator()(const Connection& connection);
    };

    Connection() : departure_stop_point(NULL), destination_stop_point(NULL), duration(0),
        max_duration(0), connection_kind(DefaultConnection){}

   bool operator<(const Connection& other) const;

};

struct StopArea : public TransmodelHeader, Nameable{
    GeographicalCoord coord;
    int properties;
    std::string additional_data;

    bool main_stop_area;
    bool main_connection;

    struct Transformer{
        inline navitia::type::StopArea operator()(const StopArea* stop_area){return this->operator()(*stop_area);}
        navitia::type::StopArea operator()(const StopArea& stop_area);
    };


    StopArea(): properties(0), main_stop_area(false), main_connection(false) {}

    bool operator<(const StopArea& other) const;
};

struct Network : public TransmodelHeader, Nameable{
    std::string address_name;
    std::string address_number;
    std::string address_type_name;
    std::string phone_number;
    std::string mail;
    std::string website;
    std::string fax;

    std::vector<Line*> line_list;

    struct Transformer{
        inline navitia::type::Network operator()(const Network* network){return this->operator()(*network);}
        navitia::type::Network operator()(const Network& network);
    };

    bool operator<(const Network& other)const{ return this->name < other.name;}


};

struct Company : public TransmodelHeader, Nameable{
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

struct ModeType : public TransmodelHeader, Nameable{
    std::vector<Mode*> mode_list;
    std::vector<Line*> line_list;

    struct Transformer{
        inline nt::ModeType operator()(const ModeType* mode_type){return this->operator()(*mode_type);}   
        nt::ModeType operator()(const ModeType& mode_type);   
    };

    bool operator<(const ModeType& other)const ;
};

struct Mode : public TransmodelHeader, Nameable{
    ModeType* mode_type;

    struct Transformer{
        inline nt::Mode operator()(const Mode* mode){return this->operator()(*mode);}   
        nt::Mode operator()(const Mode& mode);   
    };

    Mode(): mode_type(NULL){}

    bool operator<(const Mode& other) const;
};

struct Line : public TransmodelHeader, Nameable {
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

    StopPoint* forward_direction;

    StopPoint* backward_direction;


    std::vector<idx_t> forward_route;
    std::vector<idx_t> backward_route;

    std::vector<idx_t> validity_pattern_list;

    struct Transformer{
        inline nt::Line operator()(const Line* line){return this->operator()(*line);}   
        nt::Line operator()(const Line& line);   
    };

    Line(): sort(0), mode_type(NULL), network(NULL), forward_direction(NULL), backward_direction(NULL){}

    bool operator<(const Line & other) const;

};

struct Route : public TransmodelHeader, Nameable{
    bool is_frequence;
    bool is_forward;
    bool is_adapted;
    Line* line;
    Mode* mode;   

    struct Transformer{
        inline navitia::type::Route operator()(const Route* route){return this->operator()(*route);}
        navitia::type::Route operator()(const Route& route);
    };

    Route(): is_frequence(false), is_forward(false), is_adapted(false), line(NULL), mode(NULL){};

    bool operator<(const Route& other) const;

 };
struct VehicleJourney: public TransmodelHeader, Nameable{
    Route* route;
    Company* company;
    Mode* mode;
    //Vehicle* vehicle;
    bool is_adapted;

    ValidityPattern* validity_pattern;

    struct Transformer{
        inline navitia::type::VehicleJourney operator()(const VehicleJourney* vj){return this->operator()(*vj);}
        navitia::type::VehicleJourney operator()(const VehicleJourney& vj);
    };

    VehicleJourney(): route(NULL), company(NULL), mode(NULL), is_adapted(false), validity_pattern(NULL){};

    bool operator<(const VehicleJourney& other) const;
};

struct Equipement : public TransmodelHeader {
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

struct RoutePoint : public TransmodelHeader, Nameable{
    int order;
    bool main_stop_point;
    int fare_section;
    Route* route;
    StopPoint* stop_point;

    struct Transformer{
        inline nt::RoutePoint operator()(const RoutePoint* route_point){return this->operator()(*route_point);}   
        nt::RoutePoint operator()(const RoutePoint& route_point);
    };

    RoutePoint() : order(0), main_stop_point(false), fare_section(0), route(NULL), stop_point(NULL){}

    bool operator<(const RoutePoint& other) const;

};

struct ValidityPattern: public TransmodelHeader {
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

struct StopPoint : public TransmodelHeader, Nameable{
    GeographicalCoord coord;
    int fare_zone;

    std::string address_name;
    std::string address_number;
    std::string address_type_name;

    StopArea* stop_area;
    Mode* mode;
    City* city;

    struct Transformer{
        inline nt::StopPoint operator()(const StopPoint* stop_point){return this->operator()(*stop_point);}   
        nt::StopPoint operator()(const StopPoint& stop_point);   
    };

    StopPoint(): fare_zone(0), stop_area(NULL), mode(NULL), city(NULL) {}

    bool operator<(const StopPoint& other) const;

};

struct StopTime: public TransmodelHeader {
    int arrival_time; ///< En secondes depuis minuit
    int departure_time; ///< En secondes depuis minuit
    VehicleJourney* vehicle_journey;
    StopPoint* stop_point;
    int order;
    bool ODT;
    int zone;

    struct Transformer{
        inline navitia::type::StopTime operator()(const StopTime* stop){return this->operator()(*stop);}
        navitia::type::StopTime operator()(const StopTime& stop);
    };

    StopTime(): arrival_time(0), departure_time(0), vehicle_journey(NULL), stop_point(NULL), order(0), 
        ODT(false), zone(0){}


    bool operator<(const StopTime& other){return this->departure_time < other.departure_time;}
};



}}//end namespace navimake::types
