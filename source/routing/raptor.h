#pragma once
#include "type/type.h"
#include "type/data.h"
#include "routing.h"
#include "utils/timer.h"
#include <unordered_map>

namespace navitia { namespace routing { namespace raptor{

template<class T>
size_t hash_value(T t) {
    return static_cast<size_t>(t);
}



struct type_retour {
    int stid;
    DateTime dt;
    type_retour(int stid, DateTime dt) : stid(stid), dt(dt) {}
    type_retour() : stid(-1), dt() {}

    bool operator<(type_retour r2) const { return this->dt < r2.dt;}

    bool operator==(type_retour r2) const { return this->stid == r2.stid;}
    bool operator!=(type_retour r2) const { return !(this->stid == r2.stid);}
};


typedef std::pair<int, int> pair_int;
typedef std::unordered_map<int, int> map_int_int_t;
typedef std::unordered_map<int, type_retour> map_int_pint_t;


typedef std::unordered_map<unsigned int, map_int_pint_t> map_retour_t;
//typedef std::pair<map_retour_t, map_best_t> pair_retour_t;


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





struct RAPTOR : public AbstractRouter
{
    const navitia::type::Data &data;


    RAPTOR(const navitia::type::Data &data);
    Path compute(idx_t departure_idx, idx_t destination_idx, int departure_hour, int departure_day);


    Path compute(const type::GeographicalCoord departure, double radius, idx_t destination_idx, int departure_hour, int departure_day);

    Path compute(map_int_pint_t bests_, idx_t destination_idx);

    std::pair<unsigned int, bool> earliest_trip(unsigned int route, unsigned int stop_area, map_retour_t &retour, unsigned int count);
    std::pair<unsigned int, bool> earliest_trip(unsigned int route, unsigned int stop_area, int time, int day);
    int get_rp_order(const navitia::type::Route &route, unsigned int stop_area);
    int get_rp_order(unsigned int route, unsigned int stop_area);
    int get_rp_id(const type::Route &route, unsigned int stop_area);
    int get_rp_id(unsigned int route, unsigned int stop_area);
    int get_sa_rp(unsigned int order, int route);


    map_int_int_t make_queue(std::vector<unsigned int> stops) ;
    void McRAPTOR(unsigned int depart, int arrivee, int debut, int date, unsigned int said_via = std::numeric_limits<int>::max());




    typedef std::unordered_map<int, label> map_label_t;
    typedef std::vector<label> list_label_t;



    class Parent_Bag {
    public:
        list_label_t labels;
        label best;
        RAPTOR * raptor;

        Parent_Bag() : labels(), best() {}
        Parent_Bag(RAPTOR * raptor) :labels(),  best(), raptor(raptor) {

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
        Best_Bag() {}
        Best_Bag(RAPTOR * raptor) : Parent_Bag(raptor) {}
        bool ajouter_label(label lbl);

    };

    class Bag : public Parent_Bag {
    public :
        Best_Bag &bp;
        Best_Bag &bpt;

        Bag(RAPTOR * raptor, Best_Bag &bp, Best_Bag &bpt) : Parent_Bag(raptor), bp(bp), bpt(bpt) { }

        bool ajouter_label(label lbl);
        bool merge(Bag_route &bagr);
    };

    class Bag_route : public Parent_Bag {
    public:
        unsigned int route;

        Bag_route(RAPTOR * raptor) : Parent_Bag(raptor)  {}
        Bag_route(RAPTOR * raptor, unsigned int route) : Parent_Bag(raptor), route(route) {}

        bool ajouter_label(label lbl) ;
        void update(unsigned int order, unsigned int said_via) ;
        bool merge(Bag &bagr, unsigned int order) ;
    };


    struct bags_t {
        unsigned int pt;
        RAPTOR * raptor;
        boost::unordered_map<std::pair<unsigned int, unsigned int>, Bag> bags;
        boost::unordered_map<int, Best_Bag>best_bags;

        bags_t(unsigned int p, unsigned int pt, RAPTOR * raptor) : pt(pt), raptor(raptor) {
            for(unsigned int i = 0; i < p; ++i) {
                best_bags.insert(std::make_pair(i, Best_Bag(raptor)));
            }

        }

        bool exists(std::pair<unsigned int, unsigned int> index) {
            return bags.find(index) != bags.end();
        }

        Bag &operator[](std::pair<unsigned int, unsigned int> index) {
            auto iter = bags.find(index);
            if(iter == bags.end()) {
                return (*bags.insert(std::make_pair(index, Bag(raptor, best_bags[index.second], best_bags[pt]))).first).second;
            } else {
                return (*iter).second;
            }
        }

    };


    struct compare_rp {
        const navitia::type::RoutePoint & rp;
        const navitia::type::Data &data;
        compare_rp(const navitia::type::RoutePoint & rp, const navitia::type::Data &data) : rp(rp), data(data) {}

        bool operator ()(unsigned int vj1, int time) {
            return (data.pt_data.stop_times.at(data.pt_data.vehicle_journeys.at(vj1).stop_time_list.at(rp.order)).departure_time %86400) < time;
        }
    };


};

}}}


