#pragma once
#include <boost/date_time/gregorian/gregorian.hpp>
#include <vector>
#include <bitset>
#include <map>
#include <set>
#include <boost/foreach.hpp>

#include <boost/iterator.hpp>
#include <boost/iterator/filter_iterator.hpp>
#include <boost/regex.hpp>

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

    template<class Value>
    class iter : public boost::iterator_facade<iter<Value>, Value, std::random_access_iterator_tag> {
        friend class boost::iterator_core_access;
        Value* element;
    public:
        iter() : element(0){};
        iter(Value & val) : element(&val){};
        Value & dereference() const { return *element;}
        bool equal(const iter & other) const { return other.element == element;}
        void increment(){element++;}
        void decrement(){element--;}
        void advance(size_t n){element += n;}
        size_t distance_to(const iter & other){return other.element - element;}
    };
public:

    typedef iter<T> iterator;
    typedef iter<const T> const_iterator;

    std::vector<T> items; ///< Eléments à proprement parler
    std::map<std::string, int> items_map; ///< map entre une clef exterene et l'indexe des éléments

    iterator begin() {return iterator(items.front());}
    const_iterator begin() const {return const_iterator(items.front());}
    iterator end() {iterator end(items.back()); end++; return end;}
    const_iterator end() const {const_iterator end(items.back()); end++; return end;}

    ///ajoute un élément dans le container
    int add(const std::string & external_code, const T & item){
        items.push_back(item);
        int position = items.size() - 1;
        items_map[external_code] = position;
        return position;
    }

    /// Retourne l'élément en fonction de son index
    T & operator[](int position){
        return items[position];
    }
    
    /// Retourne l'élément en fonction de son code externe
    T & operator[](const std::string & external_code){
        if(!exist(external_code)){
            throw std::out_of_range();
        }
        return items[get_idx[external_code]];
    }

    /// Retourne l'indexe d'un élément en fonction de son code externe
    int get_idx(const std::string & external_code) {
        return items_map[external_code];
    }

    /// Est-ce que l'élément ayant telle clef exeterne existe ?
    bool exist(const std::string & external_code){
        return (items_map.find(external_code) != items_map.end());
    }

    /// Est-ce que l'élément ayant tel index existe ?
    bool exist(int idx){
        return (idx < items.size());
    }

    /// Fonction interne utilisée par boost pour sérialiser (aka. binariser) la structure de données
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & items & items_map;
    }

    /// Nombre d'éléments dans la structure
    int size() const {
        return items.size();
    }

    /** Functor permettant de tester l'attribut passé en paramètre avec la valeur passée en paramètre
      *
      * par exemple is_equal<std::string>(&StopPoint::name, "Châtelet");
      */
    template<class Attribute>
    struct is_equal{
        const Attribute & ref;
        Attribute T::*attr;
        is_equal(Attribute T::*attr, const Attribute & ref) : ref(ref), attr(attr){}
        bool operator()(const T & elt) const {return ref == elt.*attr;}
    };

    /** Functor qui matche une expression rationnelle sur l'attribut passé en paramètre
      *
      * L'attribut doit être une chaîne de caractères
      */
    struct matches {
        const boost::regex e;
        std::string T::*attr;
        matches(std::string T::*attr, const std::string & e) : e(e), attr(attr){}
        bool operator()(const T & elt) const {return boost::regex_match(elt.*attr, e);}
    };


    /** Permet de filtrer les éléments selon un functor
      *
      * Retourne une paire d'iterateurs vers les éléments filtrés
      */
    template<class Functor>
    std::pair<boost::filter_iterator<Functor, iterator>, boost::filter_iterator<Functor, iterator> >
    filter(Functor f){
        return std::make_pair(boost::make_filter_iterator(f, begin(), end()),
                              boost::make_filter_iterator(f, end(), end()) );
    }

    /** Filtre selon la valeur d'un attribut */
    template<class Attribute>
    std::pair<boost::filter_iterator<is_equal<Attribute>, iterator>, boost::filter_iterator<is_equal<Attribute>, iterator> >
              filter(Attribute T::*attr, Attribute value ) {
        return filter(is_equal<Attribute>(attr, value));
    }

    /** Surcharge de filter pour gérer le cas où on passe un litteral de string */
    std::pair<boost::filter_iterator<is_equal<std::string>, iterator>, boost::filter_iterator<is_equal<std::string>, iterator> >
            filter(std::string T::*attr, const char * str) {
        return filter(is_equal<std::string>(attr, std::string(str)));
    }

    /** Filtre selon la valeur d'un attribut qui matche une regex */
    std::pair<boost::filter_iterator<matches, iterator>, boost::filter_iterator<matches, iterator> >
            filter_match(std::string T::*attr, const std::string & str ) {
        return filter(matches(attr, str));
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
