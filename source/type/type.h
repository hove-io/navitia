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
namespace mpl = boost::mpl;

namespace navitia { namespace type {
typedef uint32_t idx_t;
const idx_t invalid_idx = std::numeric_limits<idx_t>::max();

enum class Type_e {
    eValidityPattern = 0,
    eLine = 1,
    eRoute = 2,
    eVehicleJourney = 3,
    eStopPoint = 4,
    eStopArea = 5,
    eStopTime = 6,
    eNetwork = 7,
    eMode = 8,
    eModeType = 9,
    eCity = 10,
    eConnection = 11,
    eRoutePoint = 12,
    eDistrict = 13,
    eDepartment = 14,
    eCompany = 15,
    eVehicle = 16,
    eCountry = 17,
    eUnknown = 18,
    eWay = 19,
    eCoord = 20,
    eRoutePointConnection = 21,
    eAddress = 22
};
struct PT_Data;
template<class T> std::string T::* name_getter(){return &T::name;}
template<class T> int T::* idx_getter(){return &T::idx;}


struct Nameable{
    std::string name;
    std::string comment;
};



struct NavitiaHeader{
    std::string id;
    idx_t idx;
    std::string external_code;
    NavitiaHeader() : idx(invalid_idx){}
    std::vector<idx_t> get(Type_e, const PT_Data &) const {return std::vector<idx_t>();}

};


/** Coordonnées géographiques en WGS84
 *
 * Elles sont stockées en virgule fixe pour gagner de la mémoire
 */
struct GeographicalCoord{
    GeographicalCoord() : _lon(0), _lat(0) {}
    GeographicalCoord(double lon, double lat) : _lon(lon*precision), _lat(lat*precision) {}
    GeographicalCoord(double x, double y, bool) {set_xy(x, y);}

    double lon() const { return _lon/precision;}
    double lat() const { return _lat/precision;}
    int32_t raw_lon() const {return _lon;}
    int32_t raw_lat() const {return _lat;}

    void set_lon(double lon) { this->_lon = lon*precision;}
    void set_lat(double lat) { this->_lat = lat*precision;}
    void set_xy(double x, double y){this->set_lon(x*M_TO_DEG); this->set_lat(y*M_TO_DEG);}

    /// Ordre des coordonnées utilisé par ProximityList
    bool operator<(GeographicalCoord other) const {return this->_lon < other._lon;}

    constexpr static double DEG_TO_RAD = 0.01745329238;
    constexpr static double precision = 1e7;
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
       http://paulbourke.net/geometry/pointline/
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
    int32_t _lon;
    int32_t _lat;
};

std::ostream & operator<<(std::ostream &_os, const GeographicalCoord & coord);

/** Deux points sont considérés comme étant égaux s'ils sont à moins de 0.1m */
bool operator==(const GeographicalCoord & a, const GeographicalCoord & b);

struct Country: public NavitiaHeader, Nameable {
    const static Type_e type = Type_e::eCountry;
    idx_t main_city_idx;
    std::vector<idx_t> district_list;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & main_city_idx & district_list & idx;
    }
    std::vector<idx_t> get(Type_e type, const PT_Data & data) const;

    Country() : main_city_idx(invalid_idx) {}
};

struct District : public NavitiaHeader, Nameable {
    const static Type_e type = Type_e::eDistrict;
    idx_t main_city_idx;
    idx_t country_idx;
    std::vector<idx_t> department_list;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & main_city_idx & country_idx & department_list & idx;
    }

    District() : main_city_idx(invalid_idx), country_idx(invalid_idx) {}
};

struct Department : public NavitiaHeader, Nameable {
    const static Type_e type = Type_e::eDepartment;
    idx_t main_city_idx;
    idx_t district_idx;
    std::vector<idx_t> city_list;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & main_city_idx & district_idx & city_list & idx & id;
    }

    Department() : main_city_idx(invalid_idx), district_idx(invalid_idx) {}
};


struct City : public NavitiaHeader, Nameable {
    const static Type_e type = Type_e::eCity;
    std::string main_postal_code;
    bool main_city;
    bool use_main_stop_area_property;

    idx_t department_idx;
    GeographicalCoord coord;

    std::vector<idx_t> postal_code_list;
    std::vector<idx_t> stop_area_list;
    std::vector<idx_t> address_list;
    std::vector<idx_t> site_list;
    std::vector<idx_t> stop_point_list;
    std::vector<idx_t> hang_list;
    std::vector<idx_t> odt_list;

    City() : main_city(false), use_main_stop_area_property(false), department_idx(invalid_idx){}

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & department_idx & coord & idx & external_code & main_postal_code & main_city & id;
    }

    std::vector<idx_t> get(Type_e type, const PT_Data & data) const;
};

enum ConnectionType {
    eStopPointConnection,
    eStopAreaConnection,
    eWalkingConnection,
    eVJConnection,
    eGuaranteedConnection
};

struct Connection: public NavitiaHeader{
    const static Type_e type = Type_e::eConnection;
    idx_t departure_stop_point_idx;
    idx_t destination_stop_point_idx;
    int duration;
    int max_duration;
    ConnectionType connection_type;

    Connection() : departure_stop_point_idx(invalid_idx), destination_stop_point_idx(invalid_idx), duration(0),
        max_duration(0){};
    
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & id & idx & external_code & departure_stop_point_idx & destination_stop_point_idx & duration & max_duration;
    }
};

enum ConnectionKind {
 extension,
 guarantee,
 undefined
};

struct RoutePointConnection : public NavitiaHeader {
      const static Type_e type = Type_e::eRoutePointConnection;

      idx_t departure_route_point_idx;
      idx_t destination_route_point_idx;
      ConnectionKind connection_kind;
  
      RoutePointConnection() : departure_route_point_idx(invalid_idx),  destination_route_point_idx(invalid_idx),
                               connection_kind(undefined){};
  
      template<class Archive> void serialize(Archive & ar, const unsigned int) {
          ar & id & idx & external_code & departure_route_point_idx & destination_route_point_idx & connection_kind;
      }
};
 

struct StopArea : public NavitiaHeader, Nameable{
    const static Type_e type = Type_e::eStopArea;
    GeographicalCoord coord;
    int properties;
    std::string additional_data;
    idx_t city_idx;

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & id & idx & external_code & name & city_idx & coord & stop_point_list;
    }

    StopArea(): properties(0), city_idx(invalid_idx){}

    std::vector<idx_t> stop_point_list;
    std::vector<idx_t> get(Type_e type, const PT_Data & data) const;


};

struct Network : public NavitiaHeader, Nameable{
    const static Type_e type = Type_e::eNetwork;
    std::string address_name;
    std::string address_number;
    std::string address_type_name;
    std::string phone_number;
    std::string mail;
    std::string website;
    std::string fax;

    std::vector<idx_t> line_list;

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & idx & id & name & external_code & address_name & address_number & address_type_name 
            & mail & website & fax & line_list;
    }

    std::vector<idx_t> get(Type_e type, const PT_Data & data) const;
};

struct Company : public NavitiaHeader, Nameable{
    const static Type_e type = Type_e::eCompany;
    std::string address_name;
    std::string address_number;
    std::string address_type_name;
    std::string phone_number;
    std::string mail;
    std::string website;
    std::string fax;

    std::vector<idx_t> line_list;

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & idx & id & name & external_code & address_name & address_number & address_type_name & phone_number
                & mail & website & fax;
    }
    std::vector<idx_t> get(Type_e type, const PT_Data & data) const;
};

struct ModeType : public NavitiaHeader, Nameable{
    const static Type_e type = Type_e::eModeType;
    std::vector<idx_t> mode_list;
    std::vector<idx_t> line_list;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & idx & id & name & external_code & mode_list & line_list;
    }
    std::vector<idx_t> get(Type_e type, const PT_Data & data) const;
};

struct Mode : public NavitiaHeader, Nameable{
    const static Type_e type = Type_e::eMode;
    idx_t mode_type_idx;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & id & idx & name & external_code & mode_type_idx & idx;
    }
    std::vector<idx_t> get(Type_e type, const PT_Data & data) const;

    Mode() : mode_type_idx(invalid_idx) {}
};

struct Line : public NavitiaHeader, Nameable {
    const static Type_e type = Type_e::eLine;
    std::string code;
    std::string forward_name;
    std::string backward_name;

    std::string additional_data;
    std::string color;
    int sort;

    idx_t mode_type_idx;

    std::vector<idx_t> mode_list;
    std::vector<idx_t> company_list;
    idx_t network_idx;

    std::vector<idx_t> forward_route;
    std::vector<idx_t> backward_route;

    std::vector<idx_t> impact_list;

    idx_t forward_direction_idx;
    idx_t backward_direction_idx;

    Line(): sort(0), mode_type_idx(invalid_idx), network_idx(invalid_idx), forward_direction_idx(invalid_idx), backward_direction_idx(invalid_idx){}

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & id & idx & name & external_code & code & forward_name & backward_name & additional_data & color
                & sort & mode_type_idx & mode_list & company_list & network_idx & forward_direction_idx & backward_direction_idx
                & impact_list;
    }
    std::vector<idx_t> get(Type_e type, const PT_Data & data) const;
};

struct Route : public NavitiaHeader, Nameable{
    const static Type_e type = Type_e::eRoute;
    bool is_frequence;
    bool is_forward;
    bool is_adapted;
    idx_t line_idx;
    idx_t mode_type_idx;
    
    std::vector<idx_t> route_point_list;
    std::vector<idx_t> freq_route_point_list;
    std::vector<idx_t> freq_setting_list;
    std::vector<idx_t> vehicle_journey_list;
    std::vector<idx_t> impact_list;

    std::vector<idx_t> vehicle_journey_list_arrival;


    Route(): is_frequence(false), is_forward(false), is_adapted(false), line_idx(invalid_idx), mode_type_idx(invalid_idx) {};

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & id & idx & name & external_code & is_frequence & is_forward & is_adapted & mode_type_idx
                & line_idx & route_point_list & freq_route_point_list & freq_setting_list
                & vehicle_journey_list & vehicle_journey_list_arrival & impact_list;
    }

    std::vector<idx_t> get(Type_e type, const PT_Data & data) const;
};

struct VehicleJourney: public NavitiaHeader, Nameable {
    const static Type_e type = Type_e::eVehicleJourney;
    idx_t route_idx;
    idx_t company_idx;
    idx_t mode_idx;
    idx_t vehicle_idx;
    bool is_adapted;
    idx_t validity_pattern_idx;
    std::vector<idx_t> stop_time_list;


    VehicleJourney(): route_idx(invalid_idx), company_idx(invalid_idx), mode_idx(invalid_idx), vehicle_idx(invalid_idx), is_adapted(false), validity_pattern_idx(invalid_idx) {}
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & external_code & route_idx & company_idx & mode_idx & vehicle_idx & is_adapted & validity_pattern_idx & idx & stop_time_list;
    }
    std::vector<idx_t> get(Type_e type, const PT_Data & data) const;
};

struct Vehicle: public NavitiaHeader, Nameable {
    const static Type_e type = Type_e::eVehicle;
};

struct Equipement : public NavitiaHeader {
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

struct RoutePoint : public NavitiaHeader{
    const static Type_e type = Type_e::eRoutePoint;
    int order;
    bool main_stop_point;
    int fare_section;
    idx_t route_idx;
    idx_t stop_point_idx;

    std::vector<idx_t> impact_list;

    RoutePoint() : order(0), main_stop_point(false), fare_section(0), route_idx(invalid_idx), stop_point_idx(invalid_idx){}

    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar & id & idx & external_code & order & main_stop_point & fare_section & route_idx 
                & stop_point_idx & impact_list & order ;
    }
    std::vector<idx_t> get(Type_e type, const PT_Data & data) const;
};

struct ValidityPattern : public NavitiaHeader {
    const static Type_e type = Type_e::eValidityPattern;
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
        ar & beginning_date & days & idx & external_code;
    }

    bool check(boost::gregorian::date day) const;
    bool check(unsigned int day) const;
    bool check2(unsigned int day) const;
    bool uncheck2(unsigned int day) const;
    //void add(boost::gregorian::date start, boost::gregorian::date end, std::bitset<7> active_days);
};

struct StopPoint : public NavitiaHeader, Nameable{
    const static Type_e type = Type_e::eStopPoint;
    GeographicalCoord coord;
    int fare_zone;

    std::string address_name;
    std::string address_number;
    std::string address_type_name;

    idx_t stop_area_idx;
    idx_t city_idx;
    idx_t mode_idx;
    idx_t network_idx;
    std::vector<idx_t> impact_list;
    std::vector<idx_t> route_point_list;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & external_code & name & stop_area_idx & mode_idx & coord & fare_zone & idx & route_point_list;
    }

    StopPoint(): fare_zone(0),  stop_area_idx(invalid_idx), city_idx(invalid_idx), mode_idx(invalid_idx), network_idx(invalid_idx){}

    std::vector<idx_t> get(Type_e type, const PT_Data & data) const;

};

struct StopTime {
    static const uint8_t PICK_UP = 0;
    static const uint8_t DROP_OFF = 1;
    static const uint8_t ODT = 2;

    idx_t idx;
    uint32_t arrival_time; ///< En secondes depuis minuit
    uint32_t departure_time; ///< En secondes depuis minuit
    idx_t vehicle_journey_idx;
    idx_t route_point_idx;
    uint32_t local_traffic_zone;

    std::bitset<8> properties;
    bool pick_up_allowed() const {return properties[PICK_UP];}
    bool drop_off_allowed() const {return properties[DROP_OFF];}
    bool odt() const {return properties[ODT];}

    StopTime(): arrival_time(0), departure_time(0), vehicle_journey_idx(invalid_idx), route_point_idx(invalid_idx),
                local_traffic_zone(std::numeric_limits<uint32_t>::max()) {}

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        // Les idx sont volontairement pas sérialisés. On les reconstruit. Ça permet de gagner 5Mo compressé pour l'Île-de-France
            ar & arrival_time & departure_time & vehicle_journey_idx & route_point_idx & properties & local_traffic_zone/*& idx*/;
    }
};


struct static_data {
private:
    static static_data * instance;
public:
    static static_data * get();
    static std::string getListNameByType(Type_e type);
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
    std::string external_code; //< Le code externe de l'objet
    int house_number;
    GeographicalCoord coordinates; //< coordonnées du point d'entrée

    /// Construit le type à partir d'une chaîne
    EntryPoint(const std::string & uri);

    EntryPoint() : type(Type_e::eUnknown), external_code("") {}

};

} } //namespace navitia::type


// Adaptateurs permettant d'utiliser boost::geometry avec les geographical coord
BOOST_GEOMETRY_REGISTER_POINT_2D_GET_SET(navitia::type::GeographicalCoord, double, boost::geometry::cs::cartesian, lon, lat, set_lon, set_lat)
BOOST_GEOMETRY_REGISTER_LINESTRING(std::vector<navitia::type::GeographicalCoord>)
