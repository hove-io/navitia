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
    eCoord = 20
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

/**
 * Représente un systéme de projection
 * le constructeur par défaut doit renvoyer le sytéme de projection utilisé par l'application en interne
 * WGS84 dans notre cas
 */
struct Projection{
    std::string name;
    std::string definition;
    bool is_degree;

    Projection(): name("wgs84"), definition("+init=epsg:4326"), is_degree(true){}

    Projection(const std::string& name, const std::string& definition, bool is_degree = false):
        name(name), definition(definition), is_degree(is_degree){}

};

struct GeographicalCoord{
    double x;
    double y;

    /// Est-ce que les coordonnées sont en degres, par défaut on suppose que oui
    /// Cela a des impacts sur le calcul des distances
    /// Si ce n'est pas des degrés, on prend la distance euclidienne
    bool degrees;

    GeographicalCoord() : x(0), y(0), degrees(true) {}
    GeographicalCoord(double x, double y, bool degrees = true) : x(x), y(y), degrees(degrees) {}
    GeographicalCoord(double x, double y, const Projection& projection);

    /* Calcule la distance Grand Arc entre deux nœuds
      *
      * On utilise la formule de Haversine
      * http://en.wikipedia.org/wiki/Law_of_haversines
      *
      * Si c'est des coordonnées non degrés, alors on utilise la distance euclidienne
      */
    double distance_to(const GeographicalCoord & other) const;

    /** Calcule la distance au carré grand arc entre deux points de manière approchée */
    double approx_sqr_distance(const GeographicalCoord &other, double coslat) const{

        if(!degrees)
            return ::pow(x - other.x, 2)+ ::pow(y-other.y, 2);
        static const double EARTH_RADIUS_IN_METERS_SQUARE = 40612548751652.183023;
        double latitudeArc = (this->y - other.y) * 0.0174532925199432958;
        double longitudeArc = (this->x - other.x) * 0.0174532925199432958;
        double tmp = coslat * longitudeArc;
        return EARTH_RADIUS_IN_METERS_SQUARE * (latitudeArc*latitudeArc + tmp*tmp);
    }

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & x & y;
    }

    GeographicalCoord convert_to(const Projection& projection, const Projection& current_projection = Projection()) const;
};

std::ostream & operator<<(std::ostream &_os, const GeographicalCoord & coord);

// Deux points sont considérés comme étant égaux s'ils sont du même type de coordonnées
// et si la distance entre eux est inférieure à 1mm
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
        ar & beginning_date & days & idx;
    }

    bool check(boost::gregorian::date day) const;
    bool check(int day) const;
    bool check2(int day) const;
    bool uncheck2(int day) const;
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
    uint16_t order;
    uint16_t zone;

    std::bitset<8> properties;
    bool pick_up_allowed() const {return properties[PICK_UP];}
    bool drop_off_allowed() const {return properties[DROP_OFF];}
    bool odt() const {return properties[ODT];}

    StopTime(): arrival_time(0), departure_time(0), vehicle_journey_idx(invalid_idx), route_point_idx(invalid_idx), order(0),
        zone(0) {}

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
            ar & arrival_time & departure_time & vehicle_journey_idx & route_point_idx & order & properties & zone & idx;
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
    GeographicalCoord coordinates; //< coordonnées du point d'entrée

    /// Construit le type à partir d'une chaîne
    EntryPoint(const std::string & uri);

    EntryPoint() : type(Type_e::eUnknown), external_code("") {}

};

} } //namespace navitia::type
