#pragma once
#include <algorithm>
#include "type/type.h"
#include "type/data.h"
#include <unordered_map>
#include <boost/unordered_map.hpp>

namespace raptor {

template<class T>
size_t hash_value(T t) {
    return static_cast<size_t>(t);
}


struct type_retour {
    int stid,
    temps,
    jour;
    type_retour(int stid, int temps, int jour) : stid(stid), temps(temps), jour(jour) {}
    type_retour() : stid(-1), temps(-1), jour(-1) {}

    bool operator<(type_retour r2) const { return this->jour < r2.jour ||((this->jour == r2.jour) && (this->temps < r2.temps));}
};

struct type_best {
    int jour,
    temps;
    type_best(int jour, int temps) : jour(jour), temps(temps) {}
    type_best() : jour(-1), temps(-1) {}

    bool operator<(type_best b2) const {
        if(this->jour < b2.jour)
            return true;
        else if(this->jour == b2.jour)
            return (this->temps < b2.temps);
        else return false;
        /*return this->jour < b2.jour ||((this->jour == b2.jour) && (this->temps < b2.temps));*/}
};

typedef std::pair<int, int> pair_int;
typedef std::unordered_map<int, int> map_int_int_t;
typedef std::unordered_map<int, type_retour> map_int_pint_t;
typedef std::unordered_map<int, type_best> map_best_t;

typedef std::unordered_map<unsigned int, map_int_pint_t> map_retour_t;
typedef std::pair<map_retour_t, map_best_t> pair_retour_t;
typedef std::vector<type_retour> itineraire_t;


std::pair<unsigned int, bool> earliest_trip(unsigned int route, unsigned int stop_area, map_retour_t &retour, navitia::type::Data &data);
std::pair<unsigned int, bool> earliest_trip(unsigned int route, unsigned int stop_area, int time, int day, navitia::type::Data &data);
int get_rp_order(navitia::type::Route &route, unsigned int stop_area, navitia::type::Data &data);
int get_rp_order(unsigned int route, unsigned int stop_area, navitia::type::Data &data);
int get_rp_id(navitia::type::Route &route, unsigned int stop_area, navitia::type::Data &data);
int get_rp_id(unsigned int route, unsigned int stop_area, navitia::type::Data &data);
int get_sa_rp(unsigned int order, int route, navitia::type::Data & data);


map_int_int_t make_queue(std::vector<unsigned int> stops, navitia::type::Data &data) ;

pair_retour_t RAPTOR(unsigned int depart, int arrivee, int debut, int date, navitia::type::Data &data);
void McRAPTOR(unsigned int depart, int arrivee, int debut, int date, navitia::type::Data &data, unsigned int said_via = std::numeric_limits<int>::max());
void afficher_chemin(unsigned int depart, int arrivee, pair_retour_t &pair_retour, unsigned int corr, navitia::type::Data &data);
itineraire_t make_itineraire(unsigned int depart, int arrivee, pair_retour_t &pair_retour, unsigned int corr, navitia::type::Data &data);
void afficher_itineraire(unsigned int depart, int arrivee, pair_retour_t &pair_retour, unsigned int corr, navitia::type::Data &data);

enum critere {
    temps_trajet = 0,
        via = 1,
    jour = 2,
    heure = 3
};


struct label {
    boost::unordered_map<critere, int> criteres;
    int stid;

    label() : criteres() {
        criteres[temps_trajet] = std::numeric_limits<int>::max();
        criteres[via] = std::numeric_limits<int>::max();
        criteres[jour] = std::numeric_limits<int>::max();
        criteres[heure] = std::numeric_limits<int>::max();
        stid = -1;
    }
    label(critere crit, int value) : criteres() {
        criteres[temps_trajet] = std::numeric_limits<int>::max();
        criteres[via] = std::numeric_limits<int>::max();
        criteres[jour] = std::numeric_limits<int>::max();
        criteres[heure] = std::numeric_limits<int>::max();
        stid = -1;

        criteres[crit] = value; }

    label& ajouter_critere(critere crit, int value) {
        criteres[crit] = value;
        return *this;
    }

    int& operator[](critere crit) {return criteres[crit];}

    int operator[](critere const crit) const {return criteres.at(crit);}

    bool operator<(const label &lbl) const {
        BOOST_FOREACH(auto p_crit, this->criteres) {
            if(p_crit.first != via) {
                if(lbl.criteres.at(p_crit.first) >= this->criteres.at(p_crit.first))
                    return false;
            }
            else {
                if(lbl.criteres.at(p_crit.first) <= this->criteres.at(p_crit.first))
                    return false;
            }

        }
        return true;
    }
    std::ostream& operator<<(std::ostream& out) {
        out << this->criteres[temps_trajet] << " " << this->criteres[jour] << " " << this->criteres[heure];
        return out;
    }

    bool dominated_by(label lbl) {
        return ((this->criteres[temps_trajet] >= lbl[temps_trajet])
                && ((this->criteres[jour] > lbl[jour]) || (this->criteres[jour] == lbl[jour] && this->criteres[heure]%86400 >= lbl[heure]%86400) ))
                || (this->criteres[via] > lbl[via]);
    }

    bool dominates(label lbl) {
        return this->criteres[temps_trajet] < lbl[temps_trajet] && ((this->criteres[jour] < lbl[jour]) ||((this->criteres[jour] == lbl[jour]) && (this->criteres[heure] < lbl[heure])));
    }


};

typedef std::unordered_map<int, label> map_label_t;
typedef std::vector<label> list_label_t;






class Parent_Bag {
public:
    list_label_t labels;
    label best;
    Parent_Bag() :labels(),  best() {

    }

    bool dominated_by(label lbl) {
        return best.dominated_by(lbl);
    }

    int get_best(critere crit) {return best[crit];}

    label& operator[](const int i) {
        return labels[i] = label();
    }

    virtual bool ajouter_label(label lbl) = 0;
};

//Forward declaration
class Bag_route;

class Best_Bag : public Parent_Bag {
public:
    Best_Bag() : Parent_Bag() {}
    bool ajouter_label(label lbl);

};

class Bag : public Parent_Bag {
public :
    Best_Bag &bp;
    Best_Bag &bpt;

    Bag(Best_Bag &bp, Best_Bag &bpt) : Parent_Bag(), bp(bp), bpt(bpt) { }

    bool ajouter_label(label lbl);
    bool merge(Bag_route &bagr, navitia::type::Data & data);
};

class Bag_route : public Parent_Bag {
public:
    unsigned int route;

    Bag_route()  {}
    Bag_route(unsigned int route) : Parent_Bag(), route(route) {}

    bool ajouter_label(label lbl) ;
    void update(unsigned int order, unsigned int said_via, navitia::type::Data &data) ;
    bool merge(Bag &bagr, unsigned int order, navitia::type::Data &data) ;
};


struct bags_t {
    unsigned int pt;
    navitia::type::Data &data;
    boost::unordered_map<std::pair<unsigned int, unsigned int>, Bag> bags;
    boost::unordered_map<int, Best_Bag>best_bags;

    bags_t(unsigned int p, unsigned int pt, navitia::type::Data &data) : pt(pt), data(data) {
        for(unsigned int i = 0; i < p; ++i) {
            best_bags.insert(std::make_pair(i, Best_Bag()));
        }

    }

    bool exists(std::pair<unsigned int, unsigned int> index) {
        return bags.find(index) != bags.end();
    }

    Bag &operator[](std::pair<unsigned int, unsigned int> index) {
        auto iter = bags.find(index);
        if(iter == bags.end()) {
            return (*bags.insert(std::make_pair(index, Bag(best_bags[index.second], best_bags[pt]))).first).second;
        } else {
            return (*iter).second;
        }
    }

};



struct compare_rp {
    navitia::type::RoutePoint & rp;
    navitia::type::Data &data;
    compare_rp(navitia::type::RoutePoint & rp, navitia::type::Data &data) : rp(rp), data(data) {}

    bool operator ()(unsigned int vj1, int time) {
        return (data.pt_data.stop_times.at(data.pt_data.vehicle_journeys.at(vj1).stop_time_list.at(rp.order)).departure_time %86400) < time;
    }
};


}
