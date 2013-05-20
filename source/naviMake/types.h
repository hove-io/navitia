#pragma once
#include <boost/date_time/gregorian/gregorian.hpp>
#include <bitset>

#include <boost/date_time/gregorian/greg_serialize.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "type/type.h"

namespace nt = navitia::type;
using nt::idx_t;

namespace navimake{ namespace types{

// On importe quelques éléments de Navitia::type pour éviter les redondances
using nt::Nameable;
using nt::Header;
using nt::hasProperties;


#define FORWARD_CLASS_DECLARE(type_name, collection_name) class type_name;
ITERATE_NAVITIA_PT_TYPES(FORWARD_CLASS_DECLARE)
class JourneyPatternPointConnection;
class StopTime;

using nt::ConnectionType;
struct Connection: public Header, hasProperties {
    const static nt::Type_e type = nt::Type_e::Connection;

    StopPoint* departure_stop_point;
    StopPoint* destination_stop_point;
    int duration;
    int max_duration;
    ConnectionType connection_kind;

    navitia::type::Connection get_navitia_type() const;

    Connection() : departure_stop_point(NULL), destination_stop_point(NULL), duration(0),
        max_duration(0), connection_kind(ConnectionType::Default){}

   bool operator<(const Connection& other) const;

};

struct JourneyPatternPointConnection: public Header {
    enum JourneyPatternPointConnectionKind {
        Extension,  //Prolongement de service
        Guarantee,   //Correspondance garantie
        UndefinedJourneyPatternPointConnectionKind
    };

    JourneyPatternPoint *departure_journey_pattern_point;
    JourneyPatternPoint *destination_journey_pattern_point;
    JourneyPatternPointConnectionKind journey_pattern_point_connection_kind;
    int length;

    navitia::type::Connection get_navitia_type() const;

    JourneyPatternPointConnection() : departure_journey_pattern_point(NULL), destination_journey_pattern_point(NULL),
                            journey_pattern_point_connection_kind(UndefinedJourneyPatternPointConnectionKind), length(0) {}

    bool operator<(const JourneyPatternPointConnection &other) const;
};


struct StopArea : public Header, Nameable, hasProperties{
    const static nt::Type_e type = nt::Type_e::StopArea;
    nt::GeographicalCoord coord;

    navitia::type::StopArea get_navitia_type() const;

    StopArea() {}

    bool operator<(const StopArea& other) const;
};

struct Network : public Header, Nameable{
    const static nt::Type_e type = nt::Type_e::Network;
    std::string address_name;
    std::string address_number;
    std::string address_type_name;
    std::string phone_number;
    std::string mail;
    std::string website;
    std::string fax;

    navitia::type::Network get_navitia_type() const;

    bool operator<(const Network& other)const{ return this->name < other.name;}
};

struct Company : public Header, Nameable{
    const static nt::Type_e type = nt::Type_e::Company;
    std::string address_name;
    std::string address_number;
    std::string address_type_name;
    std::string phone_number;
    std::string mail;
    std::string website;
    std::string fax;

    nt::Company get_navitia_type() const;

    bool operator<(const Company& other)const{ return this->name < other.name;}
};

struct CommercialMode : public Header, Nameable{
    const static nt::Type_e type = nt::Type_e::CommercialMode;
    nt::CommercialMode get_navitia_type() const;

    bool operator<(const CommercialMode& other)const ;
};

struct PhysicalMode : public Header, Nameable{
    const static nt::Type_e type = nt::Type_e::PhysicalMode;
    PhysicalMode() {}

    nt::PhysicalMode get_navitia_type() const;

    bool operator<(const PhysicalMode& other) const;
};

struct Line : public Header, Nameable {
    const static nt::Type_e type = nt::Type_e::Line;
    std::string code;
    std::string forward_name;
    std::string backward_name;

    std::string additional_data;
    std::string color;
    int sort;
    
    CommercialMode* commercial_mode;
    Network* network;
    Company* company;

    Line(): color(""), sort(0), commercial_mode(NULL), network(NULL), company(NULL){}

    nt::Line get_navitia_type() const;

    bool operator<(const Line & other) const;
};

struct Route : public Header, Nameable{
    const static nt::Type_e type = nt::Type_e::Route;
    Line * line;

    navitia::type::Route get_navitia_type() const;

    bool operator<(const Route& other) const;
};

struct JourneyPattern : public Header, Nameable{
    const static nt::Type_e type = nt::Type_e::JourneyPattern;
    bool is_frequence;
    Route* route;
    PhysicalMode* physical_mode;
    std::vector<JourneyPatternPoint*> journey_pattern_point_list;

    JourneyPattern(): is_frequence(false), route(NULL), physical_mode(NULL){};

    navitia::type::JourneyPattern get_navitia_type() const;

    bool operator<(const JourneyPattern& other) const;
 };

struct VehicleJourney: public Header, Nameable, hasProperties{
    const static nt::Type_e type = nt::Type_e::VehicleJourney;
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

    bool is_adapted;
    ValidityPattern* adapted_validity_pattern;
    std::vector<VehicleJourney*> adapted_vehicle_journey_list;
    VehicleJourney* theoric_vehicle_journey;


    VehicleJourney(): journey_pattern(NULL), company(NULL), physical_mode(NULL), tmp_line(NULL), wheelchair_boarding(false),
    validity_pattern(NULL), first_stop_time(NULL), is_adapted(false), adapted_validity_pattern(NULL), theoric_vehicle_journey(NULL){}

    navitia::type::VehicleJourney get_navitia_type() const;

    bool operator<(const VehicleJourney& other) const;
};

struct Equipement : public Header {
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

struct JourneyPatternPoint : public Header, Nameable{
    const static nt::Type_e type = nt::Type_e::JourneyPatternPoint;
    int order;
    bool main_stop_point;
    int fare_section;
    JourneyPattern* journey_pattern;
    StopPoint* stop_point;

    nt::JourneyPatternPoint get_navitia_type() const;

    JourneyPatternPoint() : order(0), main_stop_point(false), fare_section(0), journey_pattern(NULL), stop_point(NULL){}

    bool operator<(const JourneyPatternPoint& other) const;
};

struct ValidityPattern: public Header {
    const static nt::Type_e type = nt::Type_e::ValidityPattern;
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

    nt::ValidityPattern get_navitia_type() const;

    bool operator<(const ValidityPattern& other) const;
    bool operator==(const ValidityPattern& other) const;
};

struct StopPoint : public Header, Nameable, hasProperties{
    const static nt::Type_e type = nt::Type_e::StopPoint;
    nt::GeographicalCoord coord;
    int fare_zone;

    std::string address_name;
    std::string address_number;
    std::string address_type_name;

    StopArea* stop_area;
    Network* network;

    StopPoint(): fare_zone(0), stop_area(NULL), network(NULL) {}

    nt::StopPoint get_navitia_type() const;

    bool operator<(const StopPoint& other) const;
};

struct StopTime {
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

    StopTime(): arrival_time(0), departure_time(0), start_time(std::numeric_limits<int>::max()), end_time(std::numeric_limits<int>::max()),
        headway_secs(std::numeric_limits<int>::max()), vehicle_journey(NULL), journey_pattern_point(NULL), tmp_stop_point(NULL), order(0),
        ODT(false), pick_up_allowed(false), drop_off_allowed(false), is_frequency(false), wheelchair_boarding(false),
                local_traffic_zone(std::numeric_limits<uint32_t>::max()) {}

    navitia::type::StopTime get_navitia_type() const;

    bool operator<(const StopTime& other) const;
};

}}//end namespace navimake::types
