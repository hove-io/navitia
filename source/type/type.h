#pragma once
#include <boost/date_time/gregorian/gregorian.hpp>
#include <vector>
#include <bitset>

#include <boost/date_time/gregorian/greg_serialize.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/bitset.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/variant.hpp>
#include <boost/any.hpp>
#include <boost/bimap.hpp>

#include "reflexion.h"

/// Exception levée lorsqu'on demande un membre qu'on ne connait pas
struct unknown_member{};

typedef boost::variant<std::string, int>  col_t;


typedef unsigned int idx_t;


template<class T> std::string T::* name_getter(){return &T::name;}
template<class T> int T::* idx_getter(){return &T::idx;}


struct Nameable{
    std::string name;
    std::string external_code;
};



struct NavitiaObject{
    int id;
    idx_t idx;
    NavitiaObject() : id(0), idx(0){};
};

struct Country {
    idx_t idx;
    std::string name;
    idx_t main_city_idx;
    std::vector<int> district_list;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & main_city_idx & district_list & idx;
    }

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
};

struct Coordinates {
    double x, y;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & x & y;
    }
};

struct City : public NavitiaObject, Nameable {
    std::string main_postal_code;
    bool main_city;
    bool use_main_stop_area_property;

    idx_t department_idx;
    Coordinates coord;

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

    /// Retourne la valeur d'un membre en fonction du nom
    col_t get(const std::string & member) {
        if(member == "idx") return idx;
        else if(member == "department_idx") return department_idx;
        else if(member == "name") return name;
        else throw unknown_member();
    }
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
    Coordinates coord;
    int properties;
    std::string additional_data;

    std::vector<idx_t> stop_area_list;
    std::vector<idx_t> stop_point_list;
    std::vector<idx_t> impact_list;
    idx_t city_idx;

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & id & idx & external_code & name & city_idx & coord & idx;
    }

    StopArea(): properties(0), city_idx(0){}

    col_t get(const std::string & member) const{
        if(member == "idx") return idx;
        else if(member == "name") return name;
        else if(member == "external_code") return external_code;
        else if(member == "city_idx") return city_idx;
        else throw unknown_member();
    }
    static boost::variant<int StopArea::*, idx_t StopArea::*, double StopArea::*, std::string StopArea::*> get2(const std::string & member) {
        if(member == "idx") return &StopArea::idx;
        else if(member == "name") return &StopArea::name;
    }
};

struct Network : public NavitiaObject, Nameable{
    std::string address_name;
    std::string address_number;
    std::string address_type_name;
    std::string phone_number;
    std::string mail;
    std::string website;
    std::string fax;

    std::vector<idx_t> line_list;
    std::vector<idx_t> odt_list;
    std::vector<idx_t> impact_list;

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & idx & id & name & external_code & address_name & address_number & address_type_name 
            & mail & website & fax & line_list & odt_list & impact_list;
    }
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

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & idx & id & name & external_code & address_name & address_number & address_type_name & phone_number 
            & mail & website & fax;
    }
};

struct ModeType : public NavitiaObject, Nameable{
    std::vector<idx_t> mode_list;
    std::vector<idx_t> line_list;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & idx & id & name & external_code & mode_list & line_list;
    }
};

struct Mode : public NavitiaObject, Nameable{
    idx_t mode_type_idx;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & id & idx & name & external_code & mode_type_idx & idx;
    }
};

struct Line : public NavitiaObject, Nameable {
    std::string code;
    std::string forward_name;
    std::string backward_name;

    idx_t comment_idx;
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

    idx_t forward_direction;
    idx_t backward_direction;

    Line(): comment_idx(0), sort(0), mode_type_idx(0), network_idx(0), forward_direction(0), backward_direction(0){}

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & id & idx & name & external_code & code & forward_name & backward_name & comment_idx & additional_data & color 
            & sort & mode_type_idx & mode_list & company_list & network_idx & forward_direction & backward_direction 
            & impact_list & validity_pattern_list;
    }

    bool operator<(const Line & other) const { return name > other.name;}

    col_t get(const std::string & member) const{
        if(member == "idx") return idx;
        else if(member == "name") return name;
        else if(member == "code") return code;
        else if(member == "network_idx") return network_idx;
        else if(member == "forward_name") return forward_name;
        else if(member == "backward_name") return backward_name;
        else if(member == "additional_data") return additional_data;
        else if(member == "color") return color;
        else if(member == "sort") return sort;
        else throw unknown_member();
    }

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
    std::vector<idx_t> impact_list;

    Route(): comment_idx(0),  is_frequence(false), is_forward(false), is_adapted(false), line_idx(0), mode_type_idx(0), associated_route_idx(0){};

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & id & idx & name & external_code & comment_idx & is_frequence & is_forward & is_adapted & mode_type_idx
            & line_idx & associated_route_idx & route_point_list & freq_route_point_list & freq_setting_list
            & vehicle_journey_list & impact_list;
    }
    col_t get(const std::string & member) const{
        if(member == "idx") return idx;
        else if(member == "name") return name;
        else if(member == "line_idx") return line_idx;
        else throw unknown_member();
    }
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
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & external_code & route_idx & company_idx & mode_idx & vehicle_idx & is_adapted & validity_pattern_idx & idx;
    }
    col_t get(const std::string & member) const {
        if(member == "idx") return idx;
        else if(member == "name") return name;
        else if(member == "external_code") return external_code;
        else throw unknown_member();
    }
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

    std::vector<idx_t> impact_list;

    RoutePoint() : order(0), main_stop_point(false), comment_idx(0), fare_section(0), route_idx(0), stop_point_idx(0){}

    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar & id & idx & external_code & order & main_stop_point & comment_idx & fare_section & route_idx 
            & stop_point_idx & impact_list;
    }
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
    col_t get(const std::string & member) const {
        if(member == "idx") return idx;
        else if(member == "pattern") return days.to_string();
        else throw unknown_member();
    }

};

struct StopPoint : public NavitiaObject, Nameable{
    Coordinates coord;
    idx_t comment_idx;
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

    StopPoint(): comment_idx(0), fare_zone(0),  stop_area_idx(0), city_idx(0), mode_idx(0), network_idx(0){}

    col_t get(const std::string & member) const {
        if(member == "idx") return idx;
        else if(member == "name") return name;
        else if(member == "external_code") return external_code;
        else throw unknown_member();
    }
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

    StopTime(): idx(0), arrival_time(0), departure_time(0), vehicle_journey_idx(0), stop_point_idx(0), order(0), 
        comment_idx(0), ODT(false), zone(0){}

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & arrival_time & departure_time & vehicle_journey_idx & stop_point_idx & order & comment_idx & ODT & zone & idx;
    }
};

struct GeographicalCoord{
    double x;
    double y;

    GeographicalCoord() : x(0), y(0) {}
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

    boost::bimap<Criteria, std::string> criterias;
    boost::bimap<PointType, std::string> point_types;
    std::vector<std::string> true_strings;
    std::vector<std::locale> date_locales;
};


