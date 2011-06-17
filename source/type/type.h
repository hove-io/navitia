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

#include "reflexion.h"

namespace navitia { namespace type {
typedef unsigned int idx_t;
enum Type_e {eValidityPattern, eLine, eRoute, eVehicleJourney, eStopPoint, eStopArea, eStopTime,
             eNetwork, eMode, eModeType, eCity, eConnection, eRoutePoint, eDistrict, eDepartment,
             eCompany, eVehicle, eCountry, eUnknown};
struct Data;
template<class T> std::string T::* name_getter(){return &T::name;}
template<class T> int T::* idx_getter(){return &T::idx;}


struct Nameable{
    std::string name;
    std::string comment;
};



struct NavitiaHeader{
    int id;
    idx_t idx;
    std::string external_code;
    NavitiaHeader() : id(0), idx(0){};
};

struct GeographicalCoord{
    double x;
    double y;

    GeographicalCoord() : x(0), y(0) {}
    GeographicalCoord(double x, double y) : x(x), y(y) {}

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & x & y;
    }
};

struct Country{
    idx_t idx;
    std::string name;
    idx_t main_city_idx;
    std::vector<idx_t> district_list;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & main_city_idx & district_list & idx;
    }

    std::vector<idx_t> get(Type_e type, const Data & data) const;
};

struct District {
    idx_t idx;
    std::string name;
    idx_t main_city_idx;
    idx_t country_idx;
    std::vector<idx_t> department_list;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & main_city_idx & country_idx & department_list & idx;
    }

    std::vector<idx_t> get(Type_e type, const Data & data) const;
};

struct Department {
    idx_t idx;
    std::string name;
    idx_t main_city_idx;
    idx_t district_idx;
    std::vector<idx_t> city_list;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & main_city_idx & district_idx & city_list & idx;
    }

    std::vector<idx_t> get(Type_e type, const Data & data) const;
};


struct City : public NavitiaHeader, Nameable {
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

    City() : main_city(false), use_main_stop_area_property(false), department_idx(0){};

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & department_idx & coord & idx;
    }

    std::vector<idx_t> get(Type_e type, const Data & data) const;

};

struct Connection: public NavitiaHeader{
    idx_t departure_stop_point_idx;
    idx_t destination_stop_point_idx;
    int duration;
    int max_duration;

    Connection() : departure_stop_point_idx(0), destination_stop_point_idx(0), duration(0),
        max_duration(0){};
    
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & id & idx & external_code & departure_stop_point_idx & destination_stop_point_idx & duration & max_duration;
    }
};

struct StopArea : public NavitiaHeader, Nameable{
    GeographicalCoord coord;
    int properties;
    std::string additional_data;
    idx_t city_idx;

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & id & idx & external_code & name & city_idx & coord;
    }

    StopArea(): properties(0), city_idx(0){}

    std::vector<idx_t> get(Type_e type, const Data & data) const;
};

struct Network : public NavitiaHeader, Nameable{
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

    std::vector<idx_t> get(Type_e type, const Data & data) const;
};

struct Company : public NavitiaHeader, Nameable{
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
    std::vector<idx_t> get(Type_e type, const Data & data) const;
};

struct ModeType : public NavitiaHeader, Nameable{
    std::vector<idx_t> mode_list;
    std::vector<idx_t> line_list;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & idx & id & name & external_code & mode_list & line_list;
    }

    std::vector<idx_t> get(Type_e type, const Data & data) const;
};

struct Mode : public NavitiaHeader, Nameable{
    idx_t mode_type_idx;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & id & idx & name & external_code & mode_type_idx & idx;
    }
    std::vector<idx_t> get(Type_e type, const Data & data) const;
};

struct Line : public NavitiaHeader, Nameable {
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
    std::vector<idx_t> validity_pattern_list;

    idx_t forward_direction_idx;
    idx_t backward_direction_idx;

    Line(): sort(0), mode_type_idx(0), network_idx(0), forward_direction_idx(0), backward_direction_idx(0){}

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & id & idx & name & external_code & code & forward_name & backward_name & additional_data & color
                & sort & mode_type_idx & mode_list & company_list & network_idx & forward_direction_idx & backward_direction_idx
                & impact_list & validity_pattern_list;
    }

    std::vector<idx_t> get(Type_e type, const Data & data) const;
};

struct Route : public NavitiaHeader, Nameable{
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
    std::vector<idx_t> impact_list;

    Route(): is_frequence(false), is_forward(false), is_adapted(false), line_idx(0), mode_type_idx(0), associated_route_idx(0){};

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & id & idx & name & external_code & is_frequence & is_forward & is_adapted & mode_type_idx
                & line_idx & associated_route_idx & route_point_list & freq_route_point_list & freq_setting_list
                & vehicle_journey_list & impact_list;
    }

    std::vector<idx_t> get(Type_e type, const Data & data) const;
};

struct VehicleJourney: public NavitiaHeader, Nameable {
    idx_t route_idx;
    idx_t company_idx;
    idx_t mode_idx;
    idx_t vehicle_idx;
    bool is_adapted;
    idx_t validity_pattern_idx;

    VehicleJourney(): route_idx(0), company_idx(0), mode_idx(0), vehicle_idx(0), is_adapted(false), validity_pattern_idx(0){};
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & external_code & route_idx & company_idx & mode_idx & vehicle_idx & is_adapted & validity_pattern_idx & idx;
    }

    std::vector<idx_t> get(Type_e type, const Data & data) const;
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
    int order;
    bool main_stop_point;
    int fare_section;
    idx_t route_idx;
    idx_t stop_point_idx;

    std::vector<idx_t> impact_list;

    RoutePoint() : order(0), main_stop_point(false), fare_section(0), route_idx(0), stop_point_idx(0){}

    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar & id & idx & external_code & order & main_stop_point & fare_section & route_idx 
                & stop_point_idx & impact_list;
    }

    std::vector<idx_t> get(Type_e type, const Data & data) const;
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
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & beginning_date & days & idx;
    }
};

struct StopPoint : public NavitiaHeader, Nameable{
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
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & external_code & name & stop_area_idx & mode_idx & coord & fare_zone & idx;
    }

    StopPoint(): fare_zone(0),  stop_area_idx(0), city_idx(0), mode_idx(0), network_idx(0){}

    std::vector<idx_t> get(Type_e type, const Data & data) const;
};

struct StopTime: public NavitiaHeader{
    int arrival_time; ///< En secondes depuis minuit
    int departure_time; ///< En secondes depuis minuit
    int vehicle_journey_idx;
    int stop_point_idx;
    int order;
    bool ODT;
    int zone;


    StopTime(): arrival_time(0), departure_time(0), vehicle_journey_idx(0), stop_point_idx(0), order(0), 
        ODT(false), zone(0){}

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
            ar & arrival_time & departure_time & vehicle_journey_idx & stop_point_idx & order & ODT & zone 
                & idx & id & external_code;
    }

    std::vector<idx_t> get(Type_e type, const Data & data) const;
};



enum PointType{ptCity, ptSite, ptAddress, ptStopArea, ptAlias, ptUndefined, ptSeparator};
enum Criteria{cInitialization, cAsSoonAsPossible, cLeastInterchange, cLinkTime, cDebug, cWDI};


struct static_data {
private:
    static static_data * instance;
public:
    static static_data * get();
    static PointType getpointTypeByCaption(const std::string & strPointType);
    static Criteria getCriteriaByCaption(const std::string & strCriteria);
    static boost::posix_time::ptime parse_date_time(const std::string& s);
    static bool strToBool(const std::string &strValue);
    static Type_e typeByCaption(const std::string & type_str);
    static std::string captionByType(Type_e type);

    boost::bimap<Criteria, std::string> criterias;
    boost::bimap<PointType, std::string> point_types;
    boost::bimap<Type_e, std::string> types_string;
    std::vector<std::string> true_strings;
    std::vector<std::locale> date_locales;


};


} } //namespace navitia::type
