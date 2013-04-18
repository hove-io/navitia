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


namespace mpl = boost::mpl;

namespace navitia { namespace type {
typedef uint32_t idx_t;

const idx_t invalid_idx = std::numeric_limits<idx_t>::max();

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
    FUN(Connection, connections)\
    FUN(JourneyPatternPoint, journey_pattern_points)\
    FUN(Company, companies)\
    FUN(Route, routes)

enum class Type_e {
    ValidityPattern = 0,
    Line = 1,
    JourneyPattern = 2,
    VehicleJourney = 3,
    StopPoint = 4,
    StopArea = 5,
    Network = 7,
    PhysicalMode = 8,
    CommercialMode = 9,
    Connection = 11,
    JourneyPatternPoint = 12,
    Company = 15,   
    Route = 23,
    POI = 24,

    // Objets spéciaux qui ne font pas partie du référentiel TC
    eStopTime = 6,
    Address = 22,
    Coord = 20,
    Unknown = 18,
    Way = 19,
    Admin=21
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


typedef std::bitset<7> Properties;
struct hasProperties {
    static const uint8_t WHEELCHAIR_BOARDING = 0;

    bool wheelchair_boarding() {return _properties[WHEELCHAIR_BOARDING];}
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


private:
    Properties _properties;
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
    Default = 5
};


struct Connection: public Header, hasProperties{
    const static Type_e type = Type_e::Connection;
    idx_t departure_stop_point_idx;
    idx_t destination_stop_point_idx;
    int duration;
    int max_duration;
    ConnectionType connection_type;

    Connection() : departure_stop_point_idx(invalid_idx), destination_stop_point_idx(invalid_idx), duration(0),
        max_duration(0){};
    
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & id & idx & uri & departure_stop_point_idx & destination_stop_point_idx & duration & max_duration;
    }

    std::vector<idx_t> get(Type_e type, const PT_Data & data) const;
};

enum ConnectionKind {
 extension,
 guarantee,
 undefined
};

struct JourneyPatternPointConnection : public Header {
      idx_t departure_journey_pattern_point_idx;
      idx_t destination_journey_pattern_point_idx;
      ConnectionKind connection_kind;
      int length;
  
      JourneyPatternPointConnection() : departure_journey_pattern_point_idx(invalid_idx),  destination_journey_pattern_point_idx(invalid_idx),
                            connection_kind(undefined), length(0){}
  
      template<class Archive> void serialize(Archive & ar, const unsigned int) {
          ar & id & idx & uri & departure_journey_pattern_point_idx & destination_journey_pattern_point_idx & connection_kind & length;
      }
};
 

struct StopArea : public Header, Nameable, hasProperties{
    const static Type_e type = Type_e::StopArea;
    GeographicalCoord coord;
    std::string additional_data;
    std::vector<idx_t> admin_list;
    bool wheelchair_boarding;

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & id & idx & uri & name & coord & stop_point_list & admin_list &
            wheelchair_boarding;
    }

    StopArea(): wheelchair_boarding(false) {}

    std::vector<idx_t> stop_point_list;
    std::vector<idx_t> get(Type_e type, const PT_Data & data) const;
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

    std::vector<idx_t> line_list;

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & idx & id & name & uri & address_name & address_number & address_type_name
            & mail & website & fax & line_list;
    }

    std::vector<idx_t> get(Type_e type, const PT_Data & data) const;
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

    std::vector<idx_t> line_list;

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & idx & id & name & uri & address_name & address_number & address_type_name & phone_number
                & mail & website & fax;
    }
    std::vector<idx_t> get(Type_e type, const PT_Data & data) const;
};

struct CommercialMode : public Header, Nameable{
    const static Type_e type = Type_e::CommercialMode;
    std::vector<idx_t> line_list;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & idx & id & name & uri & line_list;
    }
    std::vector<idx_t> get(Type_e type, const PT_Data & data) const;
};

struct PhysicalMode : public Header, Nameable{
    const static Type_e type = Type_e::PhysicalMode;

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & id & idx & name & uri & idx;
    }
    std::vector<idx_t> get(Type_e type, const PT_Data & data) const;

    PhysicalMode() {}
};

struct Line : public Header, Nameable {
    const static Type_e type = Type_e::Line;
    std::string code;
    std::string forward_name;
    std::string backward_name;

    std::string additional_data;
    std::string color;
    int sort;

    idx_t commercial_mode_idx;

    std::vector<idx_t> company_list;
    idx_t network_idx;

    std::vector<idx_t> route_list;

    Line(): sort(0), commercial_mode_idx(invalid_idx), network_idx(invalid_idx){}

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & id & idx & name & uri & code & forward_name & backward_name & additional_data & color
                & sort & commercial_mode_idx  & company_list & network_idx
                & route_list;
    }
    std::vector<idx_t> get(Type_e type, const PT_Data & data) const;
};

struct Route : public Header, Nameable {
    const static Type_e type = Type_e::Route;
    idx_t line_idx;
    std::vector<idx_t> journey_pattern_list;

    Route() : line_idx(invalid_idx) {}

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & id & idx & name & uri & line_idx & journey_pattern_list;
    }

    std::vector<idx_t> get(Type_e type, const PT_Data & data) const;
};

struct JourneyPattern : public Header, Nameable{
    const static Type_e type = Type_e::JourneyPattern;
    bool is_frequence;
    idx_t route_idx;
    idx_t commercial_mode_idx;

    std::vector<idx_t> journey_pattern_point_list;
    std::vector<idx_t> vehicle_journey_list;

    JourneyPattern(): is_frequence(false), route_idx(invalid_idx), commercial_mode_idx(invalid_idx) {};

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & id & idx & name & uri & is_frequence & route_idx & commercial_mode_idx
                & journey_pattern_point_list & vehicle_journey_list;
    }

    std::vector<idx_t> get(Type_e type, const PT_Data & data) const;
};

struct VehicleJourney: public Header, Nameable, hasProperties{
    const static Type_e type = Type_e::VehicleJourney;
    idx_t journey_pattern_idx;
    idx_t company_idx;
    idx_t physical_mode_idx;
    idx_t validity_pattern_idx;
    bool wheelchair_boarding;
    std::vector<idx_t> stop_time_list;


    bool is_adapted;
    idx_t adapted_validity_pattern_idx;
    std::vector<idx_t> adapted_vehicle_journey_list;
    idx_t theoric_vehicle_journey_idx;

    VehicleJourney(): journey_pattern_idx(invalid_idx), company_idx(invalid_idx), physical_mode_idx(invalid_idx), /*vehicle_idx(invalid_idx), */validity_pattern_idx(invalid_idx) , wheelchair_boarding(false), is_adapted(false), adapted_validity_pattern_idx(invalid_idx), theoric_vehicle_journey_idx(invalid_idx){}
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & uri & journey_pattern_idx & company_idx & physical_mode_idx & validity_pattern_idx & idx & wheelchair_boarding & stop_time_list
            & is_adapted & adapted_validity_pattern_idx & adapted_vehicle_journey_list & theoric_vehicle_journey_idx;
    }
    std::vector<idx_t> get(Type_e type, const PT_Data & data) const;
};



struct JourneyPatternPoint : public Header{
    const static Type_e type = Type_e::JourneyPatternPoint;
    int order;
    bool main_stop_point;
    int fare_section;
    idx_t journey_pattern_idx;
    idx_t stop_point_idx;

    JourneyPatternPoint() : order(0), main_stop_point(false), fare_section(0), journey_pattern_idx(invalid_idx), stop_point_idx(invalid_idx){}

    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar & id & idx & uri & order & main_stop_point & fare_section & journey_pattern_idx
                & stop_point_idx & order ;
    }
    std::vector<idx_t> get(Type_e type, const PT_Data & data) const;
};

struct ValidityPattern : public Header {
    const static Type_e type = Type_e::ValidityPattern;
private:
    bool is_valid(int duration) const;
public:
    std::bitset<366> days;
    boost::gregorian::date beginning_date;
    idx_t idx;
    ValidityPattern() : idx(invalid_idx) {}
    ValidityPattern(boost::gregorian::date beginning_date) : beginning_date(beginning_date), idx(0){}
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
};

struct StopPoint : public Header, Nameable, hasProperties{
    const static Type_e type = Type_e::StopPoint;
    GeographicalCoord coord;
    int fare_zone;

    idx_t stop_area_idx;
    std::vector<idx_t> admin_list;
    idx_t network_idx;
    std::vector<idx_t> journey_pattern_point_list;

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & uri & name & stop_area_idx & coord & fare_zone & idx & journey_pattern_point_list & admin_list;
    }

    StopPoint(): fare_zone(0),  stop_area_idx(invalid_idx), network_idx(invalid_idx) {}

    std::vector<idx_t> get(Type_e type, const PT_Data & data) const;

};

struct StopTime {
    static const uint8_t PICK_UP = 0;
    static const uint8_t DROP_OFF = 1;
    static const uint8_t ODT = 2;
    static const uint8_t IS_FREQUENCY = 3;
    static const uint8_t WHEELCHAIR_BOARDING = 4;

    idx_t idx;
    uint32_t arrival_time; ///< En secondes depuis minuit
    uint32_t departure_time; ///< En secondes depuis minuit
    uint32_t start_time; /// Si horaire en fréquence
    uint32_t end_time; /// Si horaire en fréquence
    uint32_t headway_secs; /// Si horaire en fréquence
    idx_t vehicle_journey_idx;
    idx_t journey_pattern_point_idx;
    uint32_t local_traffic_zone;

    std::bitset<8> properties;

    bool pick_up_allowed() const {return properties[PICK_UP];}
    bool drop_off_allowed() const {return properties[DROP_OFF];}
    bool odt() const {return properties[ODT];}
    bool is_frequency() const{return properties[IS_FREQUENCY];}
    /// Est-ce qu'on peut finir par ce stop_time : dans le sens avant on veut descendre
    bool valid_end(bool clockwise) const {return clockwise ? drop_off_allowed() : pick_up_allowed();}
    /// Heure de fin de stop_time : dans le sens avant, c'est la fin, sinon le départ
    uint32_t section_end_time(bool clockwise) const {return clockwise ? arrival_time : departure_time;}

    StopTime(): arrival_time(0), departure_time(0), start_time(std::numeric_limits<uint32_t>::max()), end_time(std::numeric_limits<uint32_t>::max()),
        headway_secs(std::numeric_limits<uint32_t>::max()), vehicle_journey_idx(invalid_idx), journey_pattern_point_idx(invalid_idx),
        local_traffic_zone(std::numeric_limits<uint32_t>::max()) {}

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        // Les idx sont volontairement pas sérialisés. On les reconstruit. Ça permet de gagner 5Mo compressé pour l'Île-de-France
            ar & arrival_time & departure_time & start_time & end_time & headway_secs & vehicle_journey_idx & journey_pattern_point_idx & properties & local_traffic_zone/*& idx*/;
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
    GeographicalCoord coordinates; //< coordonnées du point d'entrée

    /// Construit le type à partir d'une chaîne
    EntryPoint(const std::string & uri);

    EntryPoint() : type(Type_e::Unknown), uri("") {}
};

} } //namespace navitia::type


// Adaptateurs permettant d'utiliser boost::geometry avec les geographical coord
BOOST_GEOMETRY_REGISTER_POINT_2D_GET_SET(navitia::type::GeographicalCoord, double, boost::geometry::cs::cartesian, lon, lat, set_lon, set_lat)
BOOST_GEOMETRY_REGISTER_LINESTRING(std::vector<navitia::type::GeographicalCoord>)
