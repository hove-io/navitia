#pragma once
#include <boost/date_time/gregorian/gregorian.hpp>
#include <bitset>

#include <boost/date_time/gregorian/greg_serialize.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "type/type.h"

namespace nt = navitia::type;
using nt::idx_t;

namespace navimake{ namespace types{

/** En-tête commun pour tous les objets TC portant un nom*/
struct Nameable{
    std::string name;
    std::string comment;
};

struct hasProperties {
    std::bitset<7>  const properties;
};

/** En tête de tous les objets TC.
  *
  * Cette classe est héritée par \b tous les objets TC
  */
struct TransmodelHeader{
    std::string id; //< Identifiant de l'objet par le fournisseur de la donnée
    idx_t idx; //< Indexe de l'objet dans le tableau
    std::string uri; //< Code pérène
    TransmodelHeader() : idx(0){}
};


//forward declare
class District;
class Department;
class City;
class Connection;
class JourneyPatternPointConnection;
class StopArea;
class Network;
class Company;
class CommercialMode;
class PhysicalMode;
class Line;
class JourneyPattern;
class VehicleJourney;
class ValidityPattern;
class Equipement;
class JourneyPatternPoint;
class StopPoint;
class StopTime;


struct Country : public TransmodelHeader, Nameable{
    City* main_city;

    std::vector<District*> district_list;
    bool operator<(const Country& other){ return this->name < other.name;}

    struct Transformer{
        inline navitia::type::Country operator()(const Country* country){return this->operator()(*country);}
        navitia::type::Country operator()(const Country& country);
    };
};

struct District: public TransmodelHeader, Nameable{
    City* main_city;
    Country* country;

    bool operator<(const District& other) const;

    struct Transformer{
        inline navitia::type::District operator()(const District* district){return this->operator()(*district);}
        navitia::type::District operator()(const District& district);
    };
};

struct Department: public TransmodelHeader, Nameable{
    City* main_city;
    District *district;
    bool operator<(const Department& other) const;

    struct Transformer{
        inline navitia::type::Department operator()(const Department* department){return this->operator()(*department);}
        navitia::type::Department operator()(const Department& department);
    };
};


struct City : public TransmodelHeader, Nameable {
    std::string main_postal_code;
    bool main_city;
    bool use_main_stop_area_property;

    Department* department;
    nt::GeographicalCoord coord;

    struct Transformer{
        inline navitia::type::City operator()(const City* city){return this->operator()(*city);}
        navitia::type::City operator()(const City& city);
    };

    City() : main_city(false), use_main_stop_area_property(false), department(NULL) {};

    bool operator<(const City& other) const {return this->name < other.name;}


};

struct Connection: public TransmodelHeader , hasProperties{
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

struct JourneyPatternPointConnection: public TransmodelHeader {
    enum JourneyPatternPointConnectionKind {
        Extension,  //Prolongement de service
        Guarantee,   //Correspondance garantie
        UndefinedJourneyPatternPointConnectionKind
    };

    JourneyPatternPoint *departure_journey_pattern_point;
    JourneyPatternPoint *destination_journey_pattern_point;
    JourneyPatternPointConnectionKind journey_pattern_point_connection_kind;
    int length;

    struct Transformer {
        inline navitia::type::JourneyPatternPointConnection operator()(const JourneyPatternPointConnection* journey_pattern_point_connection) 
        {return this->operator()(*journey_pattern_point_connection);}
        navitia::type::JourneyPatternPointConnection operator()(const JourneyPatternPointConnection &journey_pattern_point_connection);
    };


    JourneyPatternPointConnection() : departure_journey_pattern_point(NULL), destination_journey_pattern_point(NULL),
                            journey_pattern_point_connection_kind(UndefinedJourneyPatternPointConnectionKind), length(0) {}

    bool operator<(const JourneyPatternPointConnection &other) const;
};


struct StopArea : public TransmodelHeader, Nameable, hasProperties{
    nt::GeographicalCoord coord;
    std::string additional_data;

    bool main_stop_area;
    bool main_connection;
    bool wheelchair_boarding;

    struct Transformer{
        inline navitia::type::StopArea operator()(const StopArea* stop_area){return this->operator()(*stop_area);}
        navitia::type::StopArea operator()(const StopArea& stop_area);
    };


    StopArea(): main_stop_area(false), main_connection(false), wheelchair_boarding(false) {}

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

    struct Transformer{
        inline nt::Company operator()(const Company* company){return this->operator()(*company);}
        nt::Company operator()(const Company& company);
    };
    bool operator<(const Company& other)const{ return this->name < other.name;}
};

struct CommercialMode : public TransmodelHeader, Nameable{

    struct Transformer{
        inline nt::CommercialMode operator()(const CommercialMode* commercial_mode){return this->operator()(*commercial_mode);}
        nt::CommercialMode operator()(const CommercialMode& commercial_mode);
    };

    bool operator<(const CommercialMode& other)const ;
};

struct PhysicalMode : public TransmodelHeader, Nameable{

    struct Transformer{
        inline nt::PhysicalMode operator()(const PhysicalMode* physical_mode){return this->operator()(*physical_mode);}
        nt::PhysicalMode operator()(const PhysicalMode& physical_mode);
    };

    PhysicalMode() {}

    bool operator<(const PhysicalMode& other) const;
};

struct Line : public TransmodelHeader, Nameable {
    std::string code;
    std::string forward_name;
    std::string backward_name;

    std::string additional_data;
    std::string color;
    int sort;
    
    CommercialMode* commercial_mode;
    Network* network;
    Company* company;


    struct Transformer{
        inline nt::Line operator()(const Line* line){return this->operator()(*line);}   
        nt::Line operator()(const Line& line);   
    };

    Line(): color(""), sort(0), commercial_mode(NULL), network(NULL), company(NULL){}

    bool operator<(const Line & other) const;

};

struct Route : public TransmodelHeader, Nameable{
    Line * line;

    struct Transformer{
        inline navitia::type::Route operator()(const Route* route){return this->operator()(*route);}
        navitia::type::Route operator()(const Route& route);
    };

    bool operator<(const Route& other) const;
};

struct JourneyPattern : public TransmodelHeader, Nameable{
    bool is_frequence;
    Route* route;
    PhysicalMode* physical_mode;
    std::vector<JourneyPatternPoint*> journey_pattern_point_list;

    struct Transformer{
        inline navitia::type::JourneyPattern operator()(const JourneyPattern* journey_pattern){return this->operator()(*journey_pattern);}
        navitia::type::JourneyPattern operator()(const JourneyPattern& journey_pattern);
    };

    JourneyPattern(): is_frequence(false), route(NULL), physical_mode(NULL){};

    bool operator<(const JourneyPattern& other) const;

 };
struct VehicleJourney: public TransmodelHeader, Nameable, hasProperties{
    JourneyPattern* journey_pattern;
    Company* company;
    PhysicalMode* physical_mode;
    Line * tmp_line; // N'est pas à remplir obligatoirement
    //Vehicle* vehicle;
    bool wheelchair_boarding;

    ValidityPattern* validity_pattern;
    std::vector<StopTime*> stop_time_list; // N'est pas à remplir obligatoirement
    StopTime * first_stop_time;
    std::string block_id;

    struct Transformer{
        inline navitia::type::VehicleJourney operator()(const VehicleJourney* vj){return this->operator()(*vj);}
        navitia::type::VehicleJourney operator()(const VehicleJourney& vj);
    };

    VehicleJourney(): journey_pattern(NULL), company(NULL), physical_mode(NULL), wheelchair_boarding(false), validity_pattern(NULL), stop_time_list(), block_id(""){}

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

struct JourneyPatternPoint : public TransmodelHeader, Nameable{
    int order;
    bool main_stop_point;
    int fare_section;
    JourneyPattern* journey_pattern;
    StopPoint* stop_point;

    struct Transformer{
        inline nt::JourneyPatternPoint operator()(const JourneyPatternPoint* journey_pattern_point){return this->operator()(*journey_pattern_point);}   
        nt::JourneyPatternPoint operator()(const JourneyPatternPoint& journey_pattern_point);
    };

    JourneyPatternPoint() : order(0), main_stop_point(false), fare_section(0), journey_pattern(NULL), stop_point(NULL){}

    bool operator<(const JourneyPatternPoint& other) const;

};

struct ValidityPattern: public TransmodelHeader {
private:
    bool is_valid(int duration);
public:
    std::bitset<366> days;
    boost::gregorian::date beginning_date;
    ValidityPattern(){}
    ValidityPattern(boost::gregorian::date beginning_date, const std::string & vp = "") : days(vp), beginning_date(beginning_date){}
    void add(boost::gregorian::date day);
    void add(int day);
    void add(boost::gregorian::date start, boost::gregorian::date end, std::bitset<7> active_days);
    void remove(boost::gregorian::date day);
    void remove(int day);

    bool check(int day) const;

    struct Transformer{
        inline nt::ValidityPattern operator()(const ValidityPattern* validity_pattern){return this->operator()(*validity_pattern);}
        nt::ValidityPattern operator()(const ValidityPattern& validity_pattern);
    };

    bool operator<(const ValidityPattern& other) const;


};

struct StopPoint : public TransmodelHeader, Nameable, hasProperties{
    nt::GeographicalCoord coord;
    int fare_zone;

    std::string address_name;
    std::string address_number;
    std::string address_type_name;

    StopArea* stop_area;
    PhysicalMode* physical_mode;
    City* city;
    Network* network; 

    bool wheelchair_boarding;

    struct Transformer{
        inline nt::StopPoint operator()(const StopPoint* stop_point){return this->operator()(*stop_point);}   
        nt::StopPoint operator()(const StopPoint& stop_point);   
    };

    StopPoint(): fare_zone(0), stop_area(NULL), physical_mode(NULL), city(NULL), network(NULL), wheelchair_boarding(false) {}

    bool operator<(const StopPoint& other) const;

};

struct StopTime {
    int idx;
    int arrival_time; ///< En secondes depuis minuit
    int departure_time; ///< En secondes depuis minuit
    int start_time; /// Si horaire en fréquence
    int end_time; /// Si horaire en fréquence
    int headway_secs; /// Si horaire en fréquence
    VehicleJourney* vehicle_journey;
    JourneyPatternPoint* journey_pattern_point;
    StopPoint * tmp_stop_point;// ne pas remplir obligatoirement
    int order;
    bool ODT;
    bool pick_up_allowed;
    bool drop_off_allowed;
    bool is_frequency;
    bool wheelchair_boarding;
    
    uint32_t local_traffic_zone;

    struct Transformer{
        inline navitia::type::StopTime operator()(const StopTime* stop){return this->operator()(*stop);}
        navitia::type::StopTime operator()(const StopTime& stop);
    };

    StopTime(): arrival_time(0), departure_time(0), start_time(std::numeric_limits<int>::max()), end_time(std::numeric_limits<int>::max()),
        headway_secs(std::numeric_limits<int>::max()), vehicle_journey(NULL), journey_pattern_point(NULL), order(0),
        ODT(false), pick_up_allowed(false), drop_off_allowed(false), is_frequency(false), wheelchair_boarding(false),
                local_traffic_zone(std::numeric_limits<uint32_t>::max()) {}



    bool operator<(const StopTime& other) const;
};

}}//end namespace navimake::types
