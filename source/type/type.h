#pragma once
#include <boost/date_time/gregorian/gregorian.hpp>
#include <vector>
#include <bitset>

#include <boost/date_time/gregorian/greg_serialize.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/bitset.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/bimap.hpp>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/geometries/register/linestring.hpp>
#include <boost/geometry/geometries/polygon.hpp>

#include "datetime.h"

namespace mpl = boost::mpl;
namespace navitia { namespace georef {
 struct Admin;
 struct GeoRef;
}}
namespace navitia { namespace type {
typedef uint32_t idx_t;

const idx_t invalid_idx = std::numeric_limits<idx_t>::max();

struct Message;

// Types qui sont exclus : JourneyPatternPointConnection
#define ITERATE_NAVITIA_PT_TYPES(FUN)\
    FUN(ValidityPattern, validity_patterns)\
    FUN(Line, lines)\
    FUN(JourneyPattern, journey_patterns)\
    FUN(VehicleJourney, vehicle_journeys)\
    FUN(StopPoint, stop_points)\
    FUN(StopArea, stop_areas)\
    FUN(Network, networks)\
    FUN(PhysicalMode, physical_modes)\
    FUN(CommercialMode, commercial_modes)\
    FUN(JourneyPatternPoint, journey_pattern_points)\
    FUN(Company, companies)\
    FUN(Route, routes)\
    FUN(Contributor, contributors)

enum class Type_e {
    ValidityPattern                 = 0,
    Line                            = 1,
    JourneyPattern                  = 2,
    VehicleJourney                  = 3,
    StopPoint                       = 4,
    StopArea                        = 5,
    Network                         = 6,
    PhysicalMode                    = 7,
    CommercialMode                  = 8,
    Connection                      = 9,
    JourneyPatternPoint             = 10,
    Company                         = 11,
    Route                           = 12,
    POI                             = 13,
    JourneyPatternPointConnection   = 14,
    StopPointConnection             = 15,
    Contributor                     = 16,

    // Objets spéciaux qui ne font pas partie du référentiel TC
    eStopTime                       = 17,
    Address                         = 18,
    Coord                           = 19,
    Unknown                         = 20,
    Way                             = 21,
    Admin                           = 22,
    POIType                         = 23
};

enum class Mode_e{
    Walking = 0,    // Marche à pied
    Bike = 1,       // Vélo
    Car = 2,        // Voiture
    Vls = 3         // Vls
};

struct PT_Data;
template<class T> std::string T::* name_getter(){return &T::name;}
template<class T> int T::* idx_getter(){return &T::idx;}


struct Nameable{
    std::string name;
    std::string comment;
    bool visible;
    Nameable():visible(true){}
};


struct Header{
    std::string id;
    idx_t idx;
    std::string uri;
    Header() : idx(invalid_idx){}
    std::vector<idx_t> get(Type_e, const PT_Data &) const {return std::vector<idx_t>();}

};


typedef std::bitset<10> Properties;
struct hasProperties {
    static const uint8_t WHEELCHAIR_BOARDING = 0;
    static const uint8_t SHELTERED = 1;
    static const uint8_t ELEVATOR = 2;
    static const uint8_t ESCALATOR = 3;
    static const uint8_t BIKE_ACCEPTED = 4;
    static const uint8_t BIKE_DEPOT = 5;
    static const uint8_t VISUAL_ANNOUNCEMENT = 6;
    static const uint8_t AUDIBLE_ANNOUNVEMENT = 7;
    static const uint8_t APPOPRIATE_ESCORT = 8;
    static const uint8_t APPOPRIATE_SIGNAGE = 9;

    bool wheelchair_boarding() {return _properties[WHEELCHAIR_BOARDING];}
    bool wheelchair_boarding() const {return _properties[WHEELCHAIR_BOARDING];}
    bool sheltered() {return _properties[SHELTERED];}
    bool sheltered() const {return _properties[SHELTERED];}
    bool elevator() {return _properties[ELEVATOR];}
    bool elevator() const {return _properties[ELEVATOR];}
    bool escalator() {return _properties[ESCALATOR];}
    bool escalator() const {return _properties[ESCALATOR];}
    bool bike_accepted() {return _properties[BIKE_ACCEPTED];}
    bool bike_accepted() const {return _properties[BIKE_ACCEPTED];}
    bool bike_depot() {return _properties[BIKE_DEPOT];}
    bool bike_depot() const {return _properties[BIKE_DEPOT];}
    bool visual_announcement() {return _properties[VISUAL_ANNOUNCEMENT];}
    bool visual_announcement() const {return _properties[VISUAL_ANNOUNCEMENT];}
    bool audible_announcement() {return _properties[AUDIBLE_ANNOUNVEMENT];}
    bool audible_announcement() const {return _properties[AUDIBLE_ANNOUNVEMENT];}
    bool appropriate_escort() {return _properties[APPOPRIATE_ESCORT];}
    bool appropriate_escort() const {return _properties[APPOPRIATE_ESCORT];}
    bool appropriate_signage() {return _properties[APPOPRIATE_SIGNAGE];}
    bool appropriate_signage() const {return _properties[APPOPRIATE_SIGNAGE];}

    bool accessible(const Properties &required_properties) const{
        auto mismatched = required_properties & ~_properties;
        return mismatched.none();
    }
    bool accessible(const Properties &required_properties) {
        auto mismatched = required_properties & ~_properties;
        return mismatched.none();
    }

    void set_property(uint8_t property) {
        _properties.set(property, true);
    }

    void set_properties(const Properties &other) {
        this->_properties = other;
    }

    Properties properties() const {
        return this->_properties;
    }

    void unset_property(uint8_t property) {
        _properties.set(property, false);
    }

    bool property(uint8_t property) const {
        return _properties[property];
    }

    idx_t to_ulog(){
        return _properties.to_ulong();
    }

//private: on ne peut pas binaraisé si privé
    Properties _properties;
};

typedef std::bitset<8> VehicleProperties;
struct hasVehicleProperties {
    static const uint8_t WHEELCHAIR_ACCESSIBLE = 0;
    static const uint8_t BIKE_ACCEPTED = 1;
    static const uint8_t AIR_CONDITIONED = 2;
    static const uint8_t VISUAL_ANNOUNCEMENT = 3;
    static const uint8_t AUDIBLE_ANNOUNCEMENT = 4;
    static const uint8_t APPOPRIATE_ESCORT = 5;
    static const uint8_t APPOPRIATE_SIGNAGE = 6;
    static const uint8_t SCHOOL_VEHICLE = 7;

    bool wheelchair_accessible() {return _vehicle_properties[WHEELCHAIR_ACCESSIBLE];}
    bool wheelchair_accessible() const {return _vehicle_properties[WHEELCHAIR_ACCESSIBLE];}
    bool bike_accepted() {return _vehicle_properties[BIKE_ACCEPTED];}
    bool bike_accepted() const {return _vehicle_properties[BIKE_ACCEPTED];}
    bool air_conditioned() {return _vehicle_properties[AIR_CONDITIONED];}
    bool air_conditioned() const {return _vehicle_properties[AIR_CONDITIONED];}
    bool visual_announcement() {return _vehicle_properties[VISUAL_ANNOUNCEMENT];}
    bool visual_announcement() const {return _vehicle_properties[VISUAL_ANNOUNCEMENT];}
    bool audible_announcement() {return _vehicle_properties[AUDIBLE_ANNOUNCEMENT];}
    bool audible_announcement() const {return _vehicle_properties[AUDIBLE_ANNOUNCEMENT];}
    bool appropriate_escort() {return _vehicle_properties[APPOPRIATE_ESCORT];}
    bool appropriate_escort() const {return _vehicle_properties[APPOPRIATE_ESCORT];}
    bool appropriate_signage() {return _vehicle_properties[APPOPRIATE_SIGNAGE];}
    bool appropriate_signage() const {return _vehicle_properties[APPOPRIATE_SIGNAGE];}
    bool school_vehicle() {return _vehicle_properties[SCHOOL_VEHICLE];}
    bool school_vehicle() const {return _vehicle_properties[SCHOOL_VEHICLE];}

    bool accessible(const VehicleProperties &required_vehicles) const{
        auto mismatched = required_vehicles & ~_vehicle_properties;
        return mismatched.none();
    }
    bool accessible(const VehicleProperties &required_vehicles) {
        auto mismatched = required_vehicles & ~_vehicle_properties;
        return mismatched.none();
    }

    void set_vehicle(uint8_t vehicle) {
        _vehicle_properties.set(vehicle, true);
    }

    void set_vehicles(const VehicleProperties &other) {
        this->_vehicle_properties = other;
    }

    VehicleProperties vehicles() const {
        return this->_vehicle_properties;
    }

    void unset_vehicle(uint8_t vehicle) {
        _vehicle_properties.set(vehicle, false);
    }

    bool vehicle(uint8_t vehicle) const {
        return _vehicle_properties[vehicle];
    }

    idx_t to_ulog(){
        return _vehicle_properties.to_ulong();
    }

//private: on ne peut pas binaraisé si privé
    VehicleProperties _vehicle_properties;
};

struct HasMessages{
    //on utilise des smart pointer boost car ils sont sérializable
    //si les weak_ptr était géré, c'est eux qu'ils faudrait utiliser
    std::vector<boost::shared_ptr<Message>> messages;

    std::vector<boost::shared_ptr<Message>> get_applicable_messages(
            const boost::posix_time::ptime& current_time,
            const boost::posix_time::time_period& action_period)const;

};

/** Coordonnées géographiques en WGS84
 */
struct GeographicalCoord{
    GeographicalCoord() : _lon(0), _lat(0) {}
    GeographicalCoord(double lon, double lat) : _lon(lon), _lat(lat) {}
    GeographicalCoord(const GeographicalCoord& coord) : _lon(coord.lon()), _lat(coord.lat()){}
    GeographicalCoord(double x, double y, bool) {set_xy(x, y);}

    double lon() const { return _lon;}
    double lat() const { return _lat;}

    void set_lon(double lon) { this->_lon = lon;}
    void set_lat(double lat) { this->_lat = lat;}
    void set_xy(double x, double y){this->set_lon(x*M_TO_DEG); this->set_lat(y*M_TO_DEG);}

    /// Ordre des coordonnées utilisé par ProximityList
    bool operator<(GeographicalCoord other) const {return this->_lon < other._lon;}
    bool operator != (GeographicalCoord other) const {return this->_lon != other._lon;}


    constexpr static double DEG_TO_RAD = 0.01745329238;
    constexpr static double M_TO_DEG = 1.0/111319.9;
    /** Calcule la distance Grand Arc entre deux nœuds
      *
      * On utilise la formule de Haversine
      * http://en.wikipedia.org/wiki/Law_of_haversines
      *
      * Si c'est des coordonnées non degrés, alors on utilise la distance euclidienne
      */
    double distance_to(const GeographicalCoord & other) const;


    /** Projette un point sur un segment

       Retourne les coordonnées projetées et la distance au segment
       Si le point projeté tombe en dehors du segment, alors ça tombe sur le nœud le plus proche
       htCommercialtp://paulbourke.net/geometry/pointline/
       */
    std::pair<type::GeographicalCoord, float> project(GeographicalCoord segment_start, GeographicalCoord segment_end) const;

    /** Calcule la distance au carré grand arc entre deux points de manière approchée

        Cela sert essentiellement lorsqu'il faut faire plein de comparaisons de distances à un point (par exemple pour proximity list)
    */
    double approx_sqr_distance(const GeographicalCoord &other, double coslat) const{
        static const double EARTH_RADIUS_IN_METERS_SQUARE = 40612548751652.183023;
        double latitudeArc = (this->lat() - other.lat()) * DEG_TO_RAD;
        double longitudeArc = (this->lon() - other.lon()) * DEG_TO_RAD;
        double tmp = coslat * longitudeArc;
        return EARTH_RADIUS_IN_METERS_SQUARE * (latitudeArc*latitudeArc + tmp*tmp);
    }

    bool is_initialized() const {
        return distance_to(GeographicalCoord()) > 1;
    }
    bool is_default_coord()const{
        return ((this->lat() == 0) || (this->lon() == 0));
    }

    bool is_valid() const{
        return this->lon() >= -180 && this->lon() <= 180 &&
               this->lat() >= -90 && this->lat() <= 90;
    }

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & _lon & _lat;
    }

private:
    double _lon;
    double _lat;
};

std::ostream & operator<<(std::ostream &_os, const GeographicalCoord & coord);

/** Deux points sont considérés comme étant égaux s'ils sont à moins de 0.1m */
bool operator==(const GeographicalCoord & a, const GeographicalCoord & b);


enum class ConnectionType {
    StopPoint = 0,
    StopArea = 1,
    Walking = 2,
    VJ = 3,
    Guaranteed = 4,
    Default = 5,
    stay_in,
    guarantee,
    undefined
};

enum class VehicleJourneyType {
    regular = 0,                    // ligne régulière
    virtual_with_stop_time = 1,       // TAD virtuel avec horaires
    virtual_without_stop_time = 2,    // TAD virtuel sans horaires
    stop_point_to_stop_point = 3,     // TAD rabattement arrêt à arrêt
    adress_to_stop_point = 4,         // TAD rabattement adresse à arrêt
    odt_point_to_point = 5            // TAD point à point (Commune à Commune)
};

struct StopPoint;
struct Line;
struct JourneyPattern;
struct ValidityPattern;
struct Route;
struct JourneyPatternPoint;
struct VehicleJourney;
struct StopTime;

template<typename T>
struct Connection: public Header, hasProperties{
    const static Type_e type = Type_e::Connection;
    T* departure;
    T* destination;
    int duration;
    int max_duration;
    ConnectionType connection_type;

    Connection() : departure(nullptr), destination(nullptr), duration(0),
        max_duration(0){};

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & id & idx & uri & departure & destination & duration & max_duration & _properties;
    }

    std::vector<idx_t> get(Type_e type, const PT_Data & data) const;

    bool operator<(const Connection<T> &other) const;

};

typedef Connection<JourneyPatternPoint>  JourneyPatternPointConnection;
typedef Connection<StopPoint>  StopPointConnection;



struct StopArea : public Header, Nameable, hasProperties, HasMessages{
    const static Type_e type = Type_e::StopArea;
    GeographicalCoord coord;
    std::string additional_data;
    std::vector<navitia::georef::Admin*> admin_list;
    bool wheelchair_boarding;

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & id & idx & uri & name & coord & stop_point_list & admin_list
        & _properties & wheelchair_boarding & messages;
    }

    StopArea(): wheelchair_boarding(false) {}

    std::vector<StopPoint*> stop_point_list;
    std::vector<idx_t> get(Type_e type, const PT_Data & data) const;
    bool operator<(const StopArea & other) const { return this < &other; }
};

struct Network : public Header, Nameable{
    const static Type_e type = Type_e::Network;
    std::string address_name;
    std::string address_number;
    std::string address_type_name;
    std::string phone_number;
    std::string mail;
    std::string website;
    std::string fax;

    std::vector<Line*> line_list;

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & idx & id & name & uri & address_name & address_number & address_type_name
            & mail & website & fax & line_list;
    }

    std::vector<idx_t> get(Type_e type, const PT_Data & data) const;
    bool operator<(const Network & other) const { return this < &other; }

};

struct Contributor : public Header, Nameable{
    const static Type_e type = Type_e::Contributor;

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & idx & id & name & uri;
    }
    bool operator<(const Contributor & other) const { return this < &other; }
};

struct Company : public Header, Nameable{
    const static Type_e type = Type_e::Company;
    std::string address_name;
    std::string address_number;
    std::string address_type_name;
    std::string phone_number;
    std::string mail;
    std::string website;
    std::string fax;

    std::vector<Line*> line_list;

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & idx & id & name & uri & address_name & address_number & address_type_name & phone_number
                & mail & website & fax;
    }
    std::vector<idx_t> get(Type_e type, const PT_Data & data) const;
    bool operator<(const Company & other) const { return this < &other; }
};

struct CommercialMode : public Header, Nameable{
    const static Type_e type = Type_e::CommercialMode;
    std::vector<Line*> line_list;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & idx & id & name & uri & line_list;
    }
    std::vector<idx_t> get(Type_e type, const PT_Data & data) const;
    bool operator<(const CommercialMode & other) const { return this < &other; }

};

struct PhysicalMode : public Header, Nameable{
    const static Type_e type = Type_e::PhysicalMode;
    std::vector<JourneyPattern*> journey_pattern_list;

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & id & idx & name & uri & journey_pattern_list;
    }
    std::vector<idx_t> get(Type_e type, const PT_Data & data) const;

    PhysicalMode() {}
    bool operator<(const PhysicalMode & other) const { return this < &other; }

};

struct Line : public Header, Nameable, HasMessages{
    const static Type_e type = Type_e::Line;
    std::string code;
    std::string forward_name;
    std::string backward_name;

    std::string additional_data;
    std::string color;
    int sort;

    CommercialMode* commercial_mode;

    std::vector<Company*> company_list;
    Network* network;

    std::vector<Route*> route_list;
    std::vector<PhysicalMode*> physical_mode_list;

    Line(): sort(0), commercial_mode(nullptr), network(nullptr){}

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & id & idx & name & uri & code & forward_name & backward_name
                & additional_data & color & sort & commercial_mode
                & company_list & network & route_list & physical_mode_list
                & messages;
    }
    std::vector<idx_t> get(Type_e type, const PT_Data & data) const;

    bool operator<(const Line & other) const { return this < &other; }

};

struct Route : public Header, Nameable, HasMessages{
    const static Type_e type = Type_e::Route;
    Line* line;
    std::vector<JourneyPattern*> journey_pattern_list;

    Route() : line(nullptr) {}

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & id & idx & name & uri & line & journey_pattern_list & messages;
    }

    std::vector<idx_t> get(Type_e type, const PT_Data & data) const;
    bool operator<(const Route & other) const { return this < &other; }

};

struct JourneyPattern : public Header, Nameable{
    const static Type_e type = Type_e::JourneyPattern;
    bool is_frequence;
    Route* route;
    CommercialMode* commercial_mode;
    PhysicalMode* physical_mode;

    std::vector<JourneyPatternPoint*> journey_pattern_point_list;
    std::vector<VehicleJourney*> vehicle_journey_list;

    JourneyPattern(): is_frequence(false), route(nullptr), commercial_mode(nullptr), physical_mode(nullptr) {};

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & id & idx & name & uri & is_frequence & route & commercial_mode
                & physical_mode & journey_pattern_point_list & vehicle_journey_list;
    }

    std::vector<idx_t> get(Type_e type, const PT_Data & data) const;
    bool operator<(const JourneyPattern & other) const { return this < &other; }

};

struct VehicleJourney: public Header, Nameable, hasVehicleProperties, HasMessages{
    const static Type_e type = Type_e::VehicleJourney;
    JourneyPattern* journey_pattern;
    Company* company;
    ValidityPattern* validity_pattern;
    std::vector<StopTime*> stop_time_list;
    VehicleJourneyType vehicle_journey_type;
    std::string odt_message;

    bool is_adapted;
    ValidityPattern* adapted_validity_pattern;
    std::vector<VehicleJourney*> adapted_vehicle_journey_list;
    VehicleJourney* theoric_vehicle_journey;

    VehicleJourney(): journey_pattern(nullptr), company(nullptr), validity_pattern(nullptr) /*, wheelchair_boarding(false)*/, is_adapted(false), adapted_validity_pattern(nullptr), theoric_vehicle_journey(nullptr){}
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & uri & journey_pattern & company & validity_pattern
            & idx /*& wheelchair_boarding*/ & stop_time_list & is_adapted
            & adapted_validity_pattern & adapted_vehicle_journey_list
            & theoric_vehicle_journey & comment & vehicle_journey_type
            & odt_message & _vehicle_properties & messages;
    }
    std::string get_direction() const;
    bool has_date_time_estimated() const;
    std::vector<idx_t> get(Type_e type, const PT_Data & data) const;

    bool operator<(const VehicleJourney& other) const {
        if(this->journey_pattern == other.journey_pattern){
            // On compare les pointeurs pour avoir un ordre total (fonctionnellement osef du tri, mais techniquement c'est important)
            return this->stop_time_list.front() < other.stop_time_list.front();
        }else{
            return this->journey_pattern->uri < other.journey_pattern->uri;
        }
    }
};

struct ValidityPattern : public Header {
    const static Type_e type = Type_e::ValidityPattern;
private:
    bool is_valid(int duration) const;
public:
    std::bitset<366> days;
    boost::gregorian::date beginning_date;

    ValidityPattern()  {}
    ValidityPattern(const boost::gregorian::date& beginning_date) : beginning_date(beginning_date){}
    ValidityPattern(const boost::gregorian::date& beginning_date, const std::string & vp) : days(vp), beginning_date(beginning_date){}
    ValidityPattern(const ValidityPattern & vp) : days(vp.days), beginning_date(vp.beginning_date){}
    ValidityPattern(const ValidityPattern* vp) : days(vp->days), beginning_date(vp->beginning_date){}

    int slide(boost::gregorian::date day) const;
    void add(boost::gregorian::date day);
    void add(int day);
    void add(boost::gregorian::date start, boost::gregorian::date end, std::bitset<7> active_days);
    void remove(boost::gregorian::date day);
    void remove(int day);
    std::string str() const;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & beginning_date & days & idx & uri;
    }

    bool check(boost::gregorian::date day) const;
    bool check(unsigned int day) const;
    bool check2(unsigned int day) const;
    bool uncheck2(unsigned int day) const;
    //void add(boost::gregorian::date start, boost::gregorian::date end, std::bitset<7> active_days);
    bool operator<(const ValidityPattern & other) const { return this < &other; }
    bool operator==(const ValidityPattern & other) const { return (this->beginning_date == other.beginning_date) && (this->days == other.days);}
};

struct StopPoint : public Header, Nameable, hasProperties, HasMessages{
    const static Type_e type = Type_e::StopPoint;
    GeographicalCoord coord;
    int fare_zone;

    StopArea* stop_area;
    std::vector<navitia::georef::Admin*> admin_list;
    Network* network;
    std::vector<JourneyPatternPoint*> journey_pattern_point_list;

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        journey_pattern_point_list.resize(0);
        ar & uri & name & stop_area & coord & fare_zone & idx
            & journey_pattern_point_list & admin_list & _properties & messages;
    }

    StopPoint(): fare_zone(0),  stop_area(nullptr), network(nullptr) {}

    std::vector<idx_t> get(Type_e type, const PT_Data & data) const;
    bool operator<(const StopPoint & other) const { return this < &other; }


};

struct JourneyPatternPoint : public Header{
    const static Type_e type = Type_e::JourneyPatternPoint;
    int order;
    bool main_stop_point;
    int fare_section;
    JourneyPattern* journey_pattern;
    StopPoint* stop_point;

    JourneyPatternPoint() : order(0), main_stop_point(false), fare_section(0), journey_pattern(nullptr), stop_point(nullptr){}

    // Attention la sérialisation est répartrie dans deux methode: save et load
    template<class Archive> void save(Archive & ar, const unsigned int) const{
        ar & id & idx & uri & order & main_stop_point & fare_section & journey_pattern
                & stop_point & order ;
    }
    template<class Archive> void load(Archive & ar, const unsigned int) {
        ar & id & idx & uri & order & main_stop_point & fare_section & journey_pattern
                & stop_point & order;
        //on remplit le tableau des stoppoints, bizarrement ca segfault au chargement si on le fait à la bina...
        this->stop_point->journey_pattern_point_list.push_back(this);
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
    std::vector<idx_t> get(Type_e type, const PT_Data & data) const;

    bool operator<(const JourneyPatternPoint& jpp2) const {
        return this->journey_pattern < jpp2.journey_pattern  || (this->journey_pattern == jpp2.journey_pattern && this->order < jpp2.order);}

};

struct StopTime : public Nameable {
    static const uint8_t PICK_UP = 0;
    static const uint8_t DROP_OFF = 1;
    static const uint8_t ODT = 2;
    static const uint8_t IS_FREQUENCY = 3;
    static const uint8_t WHEELCHAIR_BOARDING = 4;
    static const uint8_t DATE_TIME_ESTIMATED = 5;

    uint32_t arrival_time; ///< En secondes depuis minuit
    uint32_t departure_time; ///< En secondes depuis minuit
    uint32_t start_time; ///< Si horaire en fréquence, premiere arrivee
    uint32_t end_time; ///< Si horaire en fréquence, dernier depart
    uint32_t headway_secs; ///< Si horaire en fréquence
    VehicleJourney* vehicle_journey;
    JourneyPatternPoint* journey_pattern_point;
    uint32_t local_traffic_zone;
    std::bitset<8> properties;

    ValidityPattern* departure_validity_pattern;
    ValidityPattern* arrival_validity_pattern;

    bool pick_up_allowed() const {return properties[PICK_UP];}
    bool drop_off_allowed() const {return properties[DROP_OFF];}
    bool odt() const {return properties[ODT];}
    bool is_frequency() const {return properties[IS_FREQUENCY];}
    bool date_time_estimated() const {return properties[DATE_TIME_ESTIMATED];}

    inline void set_pick_up_allowed(bool value) {properties[PICK_UP] = value;}
    inline void set_drop_off_allowed(bool value) {properties[DROP_OFF] = value;}
    inline void set_odt(bool value) {properties[ODT] = value;}
    inline void set_is_frequency(bool value) {properties[IS_FREQUENCY] = value;}
    inline void set_date_time_estimated(bool value) {properties[DATE_TIME_ESTIMATED] = value;}



    /// Est-ce qu'on peut finir par ce stop_time : dans le sens avant on veut descendre
    bool valid_end(bool clockwise) const {return clockwise ? drop_off_allowed() : pick_up_allowed();}

    /// Heure de fin de stop_time : dans le sens avant, c'est la fin, sinon le départ
    uint32_t section_end_time(bool clockwise, const u_int32_t hour = 0) const {
        if(this->is_frequency())
            return clockwise ? this->f_arrival_time(hour) : this->f_departure_time(hour);
        else
            return clockwise ? arrival_time : departure_time;
    }

    inline uint32_t f_arrival_time(const u_int32_t hour, bool clockwise = true) const {
        if(clockwise) {
            if (this == this->vehicle_journey->stop_time_list.front())
                return hour;
            auto prec_st = this->vehicle_journey->stop_time_list[this->journey_pattern_point->order-1];
            return hour + this->arrival_time - prec_st->arrival_time;
        } else {
            if (this == this->vehicle_journey->stop_time_list.back())
                return hour;
            auto next_st = this->vehicle_journey->stop_time_list[this->journey_pattern_point->order+1];
            return hour - (next_st->arrival_time - this->arrival_time);
        }
    }

    inline uint32_t f_departure_time(const u_int32_t hour, bool clockwise = false) const {
        if(clockwise) {
            if (this == this->vehicle_journey->stop_time_list.front())
                return hour;
            auto prec_st = this->vehicle_journey->stop_time_list[this->journey_pattern_point->order-1];
            return hour + this->departure_time - prec_st->departure_time;
        } else {
            if (this == this->vehicle_journey->stop_time_list.back())
                return hour;
            auto next_st = this->vehicle_journey->stop_time_list[this->journey_pattern_point->order+1];
            return hour - (next_st->departure_time - this->departure_time);
        }
    }

    DateTime section_end_date(int date, bool clockwise) const {return DateTimeUtils::set(date, this->section_end_time(clockwise));}


    /** Is this hour valid : only concerns frequency data
     * Does the hour falls inside of the validity period of the frequency
     * The difficult part is when the validity period goes over midnight
    */
    bool valid_hour(uint hour, bool clockwise) const {
        if(!this->is_frequency())
            return true;
        else
            return clockwise ? hour <= this->end_time : this->start_time <= hour;
    }

    //@TODO construire ces putin de validy pattern!!
    StopTime(): arrival_time(0), departure_time(0), start_time(std::numeric_limits<uint32_t>::max()), end_time(std::numeric_limits<uint32_t>::max()),
        headway_secs(std::numeric_limits<uint32_t>::max()), vehicle_journey(nullptr), journey_pattern_point(nullptr),
        local_traffic_zone(std::numeric_limits<uint32_t>::max()), departure_validity_pattern(nullptr), arrival_validity_pattern(nullptr){}

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
            ar & arrival_time & departure_time & start_time & end_time & headway_secs & vehicle_journey & journey_pattern_point & properties & local_traffic_zone & departure_validity_pattern & arrival_validity_pattern & comment;
    }

    bool operator<(const StopTime& other) const {
        if(this->vehicle_journey == other.vehicle_journey){
            return this->journey_pattern_point->order < other.journey_pattern_point->order;
        } else {
            return *this->vehicle_journey < *other.vehicle_journey;
        }
    }

};


struct static_data {
private:
    static static_data * instance;
public:
    static static_data * get();
    // static std::string getListNameByType(Type_e type);
    static boost::posix_time::ptime parse_date_time(const std::string& s);
    static Type_e typeByCaption(const std::string & type_str);
    static std::string captionByType(Type_e type);
    boost::bimap<Type_e, std::string> types_string;
    static Mode_e modeByCaption(const std::string & mode_str);
    boost::bimap<Mode_e, std::string> modes_string;
};

/**

Gestion des paramètres de rabattement

*/
struct StreetNetworkParams{
    Mode_e mode;
    idx_t offset;
    float speed;
    float distance;
    Type_e type_filter; // filtre sur le départ/arrivée : exemple les arrêts les plus proches à une site type
    std::string uri_filter; // l'uri de l'objet
    float radius_filter; // ce paramètre est utilisé pour le filtre
    StreetNetworkParams():
                mode(Mode_e::Walking),
                offset(0),
                speed(10),
                distance(10),
                type_filter(Type_e::Unknown),
                uri_filter(""),
                radius_filter(150){}
    void set_filter(const std::string & param_uri);
};
/**
  Gestion de l'accessibilié
  */
struct AccessibiliteParams{
    Properties properties;  // Accissibilié StopPoint, Correspondance, ..
    VehicleProperties vehicle_properties; // Accissibilié VehicleJourney

    AccessibiliteParams(){}
};

/** Type pour gérer le polymorphisme en entrée de l'API
  *
  * Les objets on un identifiant universel de type stop_area:872124
  * Ces identifiants ne devraient pas être générés par le média
  * C'est toujours NAViTiA qui le génère pour être repris tel quel par le média
  */
struct EntryPoint {
    Type_e type;//< Le type de l'objet
    std::string uri; //< Le code externe de l'objet
    int house_number;
    GeographicalCoord coordinates;  // < coordonnées du point d'entrée
    StreetNetworkParams streetnetwork_params;        // < paramètres de rabatement du point d'entrée

    /// Construit le type à partir d'une chaîne
    EntryPoint(const Type_e type, const std::string & uri);

    EntryPoint() : type(Type_e::Unknown), house_number(-1) {}
};

} } //namespace navitia::type


// Adaptateurs permettant d'utiliser boost::geometry avec les geographical coord
BOOST_GEOMETRY_REGISTER_POINT_2D_GET_SET(navitia::type::GeographicalCoord, double, boost::geometry::cs::cartesian, lon, lat, set_lon, set_lat)
BOOST_GEOMETRY_REGISTER_LINESTRING(std::vector<navitia::type::GeographicalCoord>)
