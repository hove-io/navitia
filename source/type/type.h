#pragma once
#include <boost/date_time/gregorian/gregorian.hpp>
#include <vector>
#include <bitset>
#include <map>
#include <set>
#include <boost/foreach.hpp>

#include <boost/date_time/gregorian/greg_serialize.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/bitset.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>

struct Country;
struct District;
struct Department;
struct City;
struct StopArea;
struct StopPoint;
struct Network;
struct Company;
struct ModeType;
struct Mode;
struct Line;
struct Route;
struct RoutePoint;
struct Vehicle;
struct VehicleJourney;
struct ValidityPattern;
struct StopTime;


/**
 * Classe servant de container au type T
 * permet via un seul objet d'obtenir un item via sont idx, ou sont externalCode
 */
template<class T>
class Container{
    public:
    std::vector<T> items;
    std::map<std::string, int> items_map;

    ///ajoute un élément dans le container
    int add(const std::string & external_code, const T & item){
        items.push_back(item);
        int position = items.size() - 1;
        items_map[external_code] = position;
        return position;
    }

    T & operator[](int position){
        return items[position];
    }
    
    T & operator[](const std::string & external_code){
        if(!exist(external_code)){
            throw std::out_of_range();
        }
        return items[get_idx[external_code]];
    }

    int get_idx(const std::string & external_code) {
        return items_map[external_code];
    }

    bool exist(const std::string & external_code){
        return (items_map.find(external_code) != items_map.end());
    }

    bool exist(int idx){
        return (idx < items.size());
    }

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & items & items_map;
    }

    int size() const {
        return items.size();
    }
};


template<class From, class Target>
class Index1ToN{
    Container<Target> * targets;

    Container<From> * froms;

    std::vector<std::set<int> > items;
    
    public:

    Index1ToN(){}

    Index1ToN(Container<From> & from, Container<Target> & target,  int Target::* foreign_key){
        create(from, target, foreign_key);
    }


    void create(Container<From> & from, Container<Target> & target,  int Target::* foreign_key){
        targets = &target;
        froms = &from;
        items.resize(from.size());
        for(int i=0; i<target.size(); i++){
            unsigned int from_id = target[i].*foreign_key;
            if(from_id > items.size()){
                continue;
            }
            items[from_id].insert(i);
        }
    }


    std::vector<Target*> get(int idx){
        std::vector<Target*> result;
        BOOST_FOREACH(int i, items[idx]) {
            result.push_back(&(*targets)[i]);
        }
        return result;
    }


    std::vector<Target*> get(const std::string & external_code){
        return get(froms->get_idx(external_code));
    }


    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & targets & froms & items;
    }


};

template<class Type, class Attribute>
class SortedIndex{
    Container<Type> * items;

    std::vector<int> indexes;

    struct Sorter{

        Attribute Type::*key;
        Container<Type> * items;

        Sorter(Container<Type> * items, Attribute Type::*key){
            this->items = items;
            this->key = key;
        }

        bool operator()(int a, int b){
            return (*items)[a].*key < (*items)[b].*key;
        }
    };

    public:
    SortedIndex(){};

    SortedIndex(Container<Type> & from, Attribute Type::*key){
        create(from, key);
    }


    void create(Container<Type> & from, Attribute Type::*key){
        items = &from;
        indexes.resize(from.size());
        for (int i = 0;i < from.size(); i++) {
            indexes[i] = i;
        }
        std::sort(indexes.begin(), indexes.end(), Sorter(items, key));
    }

    Type & get(int idx) {
        return (*items)[indexes[idx]];
    }

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & indexes & items;
    }



};

struct Country {
    std::string name;
    int main_city_idx;
    std::vector<int> district_list;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & main_city_idx & district_list;
    }

};

struct District {
    std::string name;
    int main_city_idx;
    int country_idx;
    std::vector<int> department_list;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & main_city_idx & country_idx & department_list;
    }
};

struct Department {
    std::string name;
    int main_city_idx;
    int district_idx;
    std::vector<int> city_list;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & main_city_idx & district_idx & city_list;
    }
};

struct Coordinates {
    double x, y;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & x & y;
    }
};

struct City {
    std::string name;
    int department_idx;
    Coordinates coord;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & department_idx & coord;
    }
};

struct StopArea {
    std::string code;
    std::string name;
    int city_idx;
    Coordinates coord;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & code & name & city_idx & coord;
    }
};

struct Network {
    std::string name;
    std::vector<int> line_list;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & line_list;
    }
};

struct Company {
    std::string name;
    std::vector<int> line_list;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & line_list;
    }
};

struct ModeType {
    std::string name;
    std::vector<int> mode_list;
    std::vector<int> line_list;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & mode_list & line_list;
    }
};

struct Mode {
    std::string name;
    int mode_type_idx;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & mode_type_idx;
    }
};

struct Line {
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
                & backward_thermo_idx & validity_pattern_list & additional_data & color & sort;
    }
};

struct Route {
    std::string name;
    int line_idx;
    ModeType mode_type;
    bool is_frequence;
    bool is_forward;
    std::vector<int> route_point_list;
    std::vector<int> vehicle_journey_list;
    bool is_adapted;
    int associated_route_idx;

    Route(): is_frequence(false), is_forward(false), is_adapted(false){};

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & line_idx & mode_type & is_frequence & is_forward & route_point_list &
                vehicle_journey_list & is_adapted & associated_route_idx;
    }
};
struct VehicleJourney {
    std::string name;
    std::string external_code;
    int route_idx;
    int company_idx;
    int mode_idx;
    int vehicle_idx;
    bool is_adapted;
    int validity_pattern_idx;

    VehicleJourney(): is_adapted(false){};
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & external_code & route_idx & company_idx & mode_idx & vehicle_idx & is_adapted & validity_pattern_idx;
    }
};


struct RoutePoint {
    std::string external_code;
    int order;
    int route_idx;
    int stop_point_idx;
    bool main_stop_point;
    int fare_section;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & external_code & order & route_idx & stop_point_idx & main_stop_point & fare_section;
    }
};

struct ValidityPattern {
private:
    boost::gregorian::date beginning_date;
    std::bitset<366> days;
    bool is_valid(int duration);
public:
    ValidityPattern() {}
    ValidityPattern(boost::gregorian::date beginning_date) : beginning_date(beginning_date) {}
    void add(boost::gregorian::date day);
    void add(int day);
    void add(boost::gregorian::date start, boost::gregorian::date end, std::bitset<7> active_days);
    void remove(boost::gregorian::date day);
    void remove(int day);
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & beginning_date & days;
    }

};

struct StopPoint {
    std::string code;
    std::string name;
    int stop_area_idx;
    int mode_idx;
    Coordinates coord;
    int fare_zone;
    std::vector<int> lines;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & code & name & stop_area_idx & mode_idx & coord & fare_zone & lines;
    }
};

struct StopTime {
    int arrival_time; ///< En secondes depuis minuit
    int departure_time; ///< En secondes depuis minuit
    int vehicle_journey_idx;
    int stop_point_idx;
    int order;
    std::string comment;
    bool ODT;
    int zone;
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & arrival_time & departure_time & vehicle_journey_idx & stop_point_idx & order & comment & ODT & zone;
    }
};
