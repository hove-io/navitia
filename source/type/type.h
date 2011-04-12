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

/// Exception levée lorsqu'on demande un membre qu'on ne connait pas
struct unknown_member{};

typedef boost::variant<std::string, int>  col_t;

template<class T> std::string T::* name_getter(){return &T::name;}
template<class T> int T::* idx_getter(){return &T::idx;}

struct Country {
    int idx;
    std::string name;
    int main_city_idx;
    std::vector<int> district_list;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & main_city_idx & district_list & idx;
    }

};

struct District {
    int idx;
    std::string name;
    int main_city_idx;
    int country_idx;
    std::vector<int> department_list;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & main_city_idx & country_idx & department_list & idx;
    }
};

struct Department {
    int idx;
    std::string name;
    int main_city_idx;
    int district_idx;
    std::vector<int> city_list;
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

struct City {
    int idx;
    std::string name;
    int department_idx;
    Coordinates coord;
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

struct StopArea{
    int idx;
    std::string name;
    std::string code;
    int city_idx;
    Coordinates coord;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & code & name & city_idx & coord & idx;
    }

    col_t get(const std::string & member) const{
        if(member == "idx") return idx;
        else if(member == "name") return name;
        else if(member == "code") return code;
        else if(member == "city_idx") return city_idx;
        else throw unknown_member();
    }
    static boost::variant<int StopArea::*, double StopArea::*, std::string StopArea::*> get2(const std::string & member) {
        if(member == "idx") return &StopArea::idx;
        else if(member == "name") return &StopArea::name;
    }
};

struct Network {
    int idx;
    std::string name;
    std::vector<int> line_list;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & line_list & idx;
    }
};

struct Company {
    int idx;
    std::string name;
    std::vector<int> line_list;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & line_list & idx;
    }
};

struct ModeType {
    int idx;
    std::string name;
    std::vector<int> mode_list;
    std::vector<int> line_list;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & mode_list & line_list & idx;
    }
};

struct Mode {
    int idx;
    std::string name;
    int mode_type_idx;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & mode_type_idx & idx;
    }
};

struct Line {
    int idx;
    std::string name;
    std::string code;
    std::string mode;
    int network_idx;
    std::string forward_name;
    std::string backward_name;
    int forward_thermo_idx;
    int backward_thermo_idx;
    std::vector<int> validity_pattern_list;
    std::string additional_data;
    std::string color;
    int sort;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & code & mode & network_idx & forward_name & backward_name & forward_thermo_idx
                & backward_thermo_idx & validity_pattern_list & additional_data & color & sort & idx;
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

struct Route {
    int idx;
    std::string name;
    int line_idx;
    ModeType mode_type;
    bool is_frequence;
    bool is_forward;
    std::vector<int> route_point_list;
    std::vector<int> vehicle_journey_list;
    bool is_adapted;
    int associated_route_idx;

    Route(): idx(0), line_idx(0), is_frequence(false), is_forward(false), is_adapted(false), associated_route_idx(0){};

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & line_idx & mode_type & is_frequence & is_forward & route_point_list &
                vehicle_journey_list & is_adapted & associated_route_idx & idx;
    }
    col_t get(const std::string & member) const{
        if(member == "idx") return idx;
        else if(member == "name") return name;
        else if(member == "line_idx") return line_idx;
        else throw unknown_member();
    }
 };
struct VehicleJourney {
    int idx;
    std::string name;
    std::string external_code;
    int route_idx;
    int company_idx;
    int mode_idx;
    int vehicle_idx;
    bool is_adapted;
    int validity_pattern_idx;

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


struct RoutePoint {
    int idx;
    std::string name;
    std::string external_code;
    int order;
    int route_idx;
    int stop_point_idx;
    bool main_stop_point;
    int fare_section;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & external_code & order & route_idx & stop_point_idx & main_stop_point & fare_section & idx;
    }
};

struct ValidityPattern {
private:
    boost::gregorian::date beginning_date;
    std::bitset<366> days;
    bool is_valid(int duration);
public:
    int idx;
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

struct StopPoint  {
    int idx;
    std::string name;
    std::string code;
    int stop_area_idx;
    int mode_idx;
    Coordinates coord;
    int fare_zone;
    std::vector<int> lines;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & code & name & stop_area_idx & mode_idx & coord & fare_zone & lines & idx;
    }

    col_t get(const std::string & member) const {
        if(member == "idx") return idx;
        else if(member == "name") return name;
        else if(member == "code") return code;
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
    std::string comment;
    bool ODT;
    int zone;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & arrival_time & departure_time & vehicle_journey_idx & stop_point_idx & order & comment & ODT & zone & idx;
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


