#pragma once
#include <boost/date_time/gregorian/gregorian.hpp>
#include <vector>
#include <bitset>
#include <map>
#include <set>


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
/*
template<class T>
class Container{
public:
    std::vector<T> items; ///< Eléments �  proprement parler
    std::map<std::string, int> items_map; ///< map entre une clef exterene et l'indexe des éléments


    template<class Value>
    class iter : public boost::iterator_facade<iter<Value>, Value, std::random_access_iterator_tag> {
        typedef iter<const T> const_iterator;
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
        size_t distance_to(const iter & other) const {return other.element - element;}
    };

   
    typedef iter<T> iterator;
    typedef iter<const T> const_iterator;


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
*/
    /** Permet de filtrer les éléments selon un functor
      *
      * Retourne une paire d'iterateurs vers les éléments filtrés
      */
  /*  template<class Functor>
    Subset<boost::filter_iterator<Functor, iterator> >
            filter(Functor f){
        return Subset<iterator>(begin(), end()).filter(f);
    }
*/
    /** Filtre selon la valeur d'un attribut */
  /*  template<class Attribute, class Param>
    Subset<boost::filter_iterator<is_equal<Attribute, T>, iterator> >
            filter(Attribute T::*attr, Param value ) {
        return Subset<iterator>(begin(), end()).filter(attr, value);
    }
*/
    /** Filtre selon la valeur d'un attribut qui matche une regex */
  /*  Subset<boost::filter_iterator<matches<T>, iterator> >
             filter_match(std::string T::*attr, const std::string & str ) {
         return Subset<iterator>(begin(), end()).filter_match(attr, str);
    }

    template<class Functor>
    Subset<boost::permutation_iterator<iterator, boost::shared_container_iterator< std::vector<ptrdiff_t> > > >
             order(Functor f) {
         return Subset<iterator>(begin(), end()).order(f);
    }

    Subset<boost::permutation_iterator<iterator, boost::shared_container_iterator< std::vector<ptrdiff_t> > > >
             order() {
                 return Subset<iterator>(begin(), end()).order(std::less<T>());
    }
};
*/

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

    Route(): is_frequence(false), is_forward(false), is_adapted(false){};

    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & line_idx & mode_type & is_frequence & is_forward & route_point_list &
                vehicle_journey_list & is_adapted & associated_route_idx & idx;
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

    VehicleJourney(): is_adapted(false){};
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & name & external_code & route_idx & company_idx & mode_idx & vehicle_idx & is_adapted & validity_pattern_idx & idx;
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
    ValidityPattern() {}
    ValidityPattern(boost::gregorian::date beginning_date) : beginning_date(beginning_date) {}
    void add(boost::gregorian::date day);
    void add(int day);
    void add(boost::gregorian::date start, boost::gregorian::date end, std::bitset<7> active_days);
    void remove(boost::gregorian::date day);
    void remove(int day);
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & beginning_date & days & idx;
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
	double X;
	double Y;
};

enum PointType{ptCity, ptSite, ptAddress, ptStopArea, ptAlias, ptUndefined, ptSeparator};
enum Criteria{cInitialization, cAsSoonAsPossible, cLeastInterchange, cLinkTime, cDebug, cWDI};
