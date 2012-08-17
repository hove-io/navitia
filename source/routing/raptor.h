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

enum type_idx {
    vj,
    connection
};


struct type_retour {
    unsigned int stid;
    int said_emarquement;
    DateTime dt;
    int dist_to_dest;
    int dist_to_dep;
    type_idx type;


    type_retour(int stid, DateTime dt, int dist_to_dest) : stid(stid), said_emarquement(-1), dt(dt), dist_to_dest(dist_to_dest), dist_to_dep(0), type(vj) {}
    type_retour(int stid, DateTime dt, int dist_to_dest, int dist_to_dep) : stid(stid), said_emarquement(-1), dt(dt), dist_to_dest(dist_to_dest), dist_to_dep(dist_to_dep), type(vj) {}

    type_retour(int stid, DateTime dt) : stid(stid), said_emarquement(-1), dt(dt), dist_to_dest(0), dist_to_dep(0), type(vj){}
    type_retour(int stid, int said_emarquement,DateTime dt) : stid(stid), said_emarquement(said_emarquement), dt(dt), dist_to_dest(0), dist_to_dep(0), type(vj){}

    type_retour(int stid, int said_emarquement, DateTime dt, type_idx type) : stid(stid), said_emarquement(said_emarquement), dt(dt), dist_to_dest(0), dist_to_dep(0), type(type){}
    type_retour(unsigned int dist_to_dest) : stid(-1), said_emarquement(-1), dt(), dist_to_dest(dist_to_dest), dist_to_dep(0), type(vj){}
    type_retour() : stid(-1), said_emarquement(-1), dt(), dist_to_dest(0), dist_to_dep(0), type(vj) {}

    bool operator<(type_retour r2) const { return this->dt + this->dist_to_dest < r2.dt + dist_to_dest;}
    bool operator>(type_retour r2) const { return this->dt + this->dist_to_dest > r2.dt + dist_to_dest;}

    bool operator==(type_retour r2) const { return this->stid == r2.stid && this->dt.hour == r2.dt.hour && this->dt.date == r2.dt.date; }
    bool operator!=(type_retour r2) const { return this->stid != r2.stid || this->dt.hour != r2.dt.hour || this->dt.date != r2.dt.date;}
};

struct best_dest {
    typedef std::pair<unsigned int, int> idx_dist;

    std::map<unsigned int, type_retour> map_date_time;
    type_retour best_now;
    unsigned int best_now_said;

    void ajouter_destination(unsigned int said, type_retour &t) { map_date_time[said] = t;}

    bool ajouter_best(unsigned int said, type_retour t) {
        if(map_date_time.find(said) != map_date_time.end()) {
            map_date_time[said] = t;
            if(t < best_now) {
                best_now = t;
                best_now_said = said;
            }
            return true;
        }
        return false;
    }

    void ajouter_best_reverse(unsigned int said, type_retour t) {
        if(map_date_time.find(said) != map_date_time.end()) {
            map_date_time[said] = t;
            if(t > best_now) {
                best_now = t;
                best_now_said = said;
            }
        }
    }

    void reverse() {
        best_now.dt.date = std::numeric_limits<int>::min();
        best_now.dt.hour = std::numeric_limits<int>::min();
    }

};


typedef std::pair<int, int> pair_int;
typedef std::unordered_map<int, int> map_int_int_t;
typedef std::unordered_map<int, type_retour> map_int_pint_t;
typedef std::unordered_map<int, pair_int> map_int_pairint_t;
typedef std::unordered_map<navitia::type::idx_t, int> queue_t;


typedef std::unordered_map<unsigned int, map_int_pint_t> map_retour_t;
//typedef std::pair<map_retour_t, map_best_t> pair_retour_t;








struct communRAPTOR : public AbstractRouter
{
    struct compare_rp {
        unsigned int order;
        const navitia::type::Data &data;
        compare_rp(const navitia::type::Data &data) : order(0), data(data) {}

        bool operator ()(unsigned int vj1, int time) {
            return (data.pt_data.stop_times[data.pt_data.vehicle_journeys[vj1].stop_time_list[order]].departure_time %86400) < time;
        }
    };
    navitia::type::Data &data;
    compare_rp cp;
    communRAPTOR(navitia::type::Data &data);
    virtual Path compute_raptor(map_int_pint_t departs, map_int_pint_t destinations) = 0;
    Path compute(idx_t departure_idx, idx_t destination_idx, int departure_hour, int departure_day);
    Path compute(const type::GeographicalCoord & departure, double radius, idx_t destination_idx, int departure_hour, int departure_day);
    Path compute(const type::GeographicalCoord & departure, double radius_depart, const type::GeographicalCoord & destination, double radius_destination
                 , int departure_hour, int departure_day);

    void trouverGeo(const type::GeographicalCoord & departure, double radius_depart, const type::GeographicalCoord & destination, double radius_destination,
                    int departure_hour, int departure_day, map_int_pint_t &departs, map_int_pint_t &destinations);



    std::pair<navitia::type::idx_t, bool>  earliest_trip(unsigned int route, unsigned int order, const type_retour &retour, unsigned int count, type::idx_t vjidx);
    std::pair<navitia::type::idx_t, bool>  earliest_trip(unsigned int route, unsigned int order, map_int_pint_t &best, unsigned int count, type::idx_t vjidx);
    std::pair<navitia::type::idx_t, bool>  earliest_trip(unsigned int route, unsigned int order, DateTime dt, type::idx_t vjidx);
    std::pair<unsigned int, bool> tardiest_trip(unsigned int route, unsigned int stop_area, map_retour_t &retour, unsigned int count);
    std::pair<unsigned int, bool> tardiest_trip(unsigned int route, unsigned int stop_area, map_int_pint_t &best, unsigned int count);
    std::pair<unsigned int, bool> tardiest_trip(unsigned int route, unsigned int stop_area, DateTime dt);
    int get_rp_order(const navitia::type::Route &route, unsigned int stop_area);
    int get_rp_order(unsigned int route, unsigned int stop_area);
    int get_rp_id(const type::Route &route, unsigned int stop_area);
    int get_rp_id(unsigned int route, unsigned int stop_area);
    int get_sa_rp(unsigned int order, int route);
    int get_temps_depart(navitia::type::idx_t t, int order);


    map_int_int_t make_queue(std::vector<unsigned int> stops) ;
    queue_t make_queue2(std::vector<unsigned int> stops) ;


    typedef std::vector<unsigned int> list_connections;
    std::map<unsigned int, list_connections> foot_path;
    std::map<unsigned int, map_int_int_t> map_sa_r;



    struct compare_rp_reverse {
        const int order;
        const navitia::type::Data &data;
        compare_rp_reverse(const int order, const navitia::type::Data &data) : order(order), data(data) {}

        bool operator ()(unsigned int vj1, int time) {
            return (data.pt_data.stop_times.at(data.pt_data.vehicle_journeys.at(vj1).stop_time_list.at(order)).arrival_time %86400) > time;
        }
    };

};

struct departureNotFound {};
struct destinationNotFound {};
struct monoRAPTOR : public communRAPTOR {
    monoRAPTOR(navitia::type::Data &data) : communRAPTOR(data){}
    Path compute_raptor(map_int_pint_t departs, map_int_pint_t destinations);
    virtual void boucleRAPTOR(std::vector<unsigned int> &marked_stop, map_retour_t &retour, map_int_pint_t &best, best_dest &b_dest, unsigned int & count) = 0;
    virtual Path makePath(const map_retour_t &retour, const map_int_pint_t &best, map_int_pint_t departs, unsigned int destination_idx, unsigned int count) = 0;
    Path makeBestPath(const map_retour_t &retour, const map_int_pint_t &best, map_int_pint_t departs, unsigned int destination_idx, unsigned int count);
    std::vector<Path> makePathes(const map_retour_t &retour, const map_int_pint_t &best, map_int_pint_t departs, best_dest &b_dest, unsigned int count);
    std::vector<Path> compute_all(const type::GeographicalCoord & departure, double radius_depart, const type::GeographicalCoord & destination, double radius_destination
                               , int departure_hour, int departure_day);
    std::vector<Path> compute_all(map_int_pint_t departs, map_int_pint_t destinations);
    std::vector<Path> compute_all(navitia::type::EntryPoint departure, navitia::type::EntryPoint destination, int departure_hour, int departure_day) ;
};

struct RAPTOR : public monoRAPTOR {
    RAPTOR(navitia::type::Data &data) : monoRAPTOR(data){}
    void boucleRAPTOR(std::vector<unsigned int> &marked_stop, map_retour_t &retour, map_int_pint_t &best, best_dest &b_dest, unsigned int & count);
    Path makePath(const map_retour_t &retour, const map_int_pint_t &best, map_int_pint_t departs, unsigned int destination_idx, unsigned int countb);
    void marcheapied(std::vector<unsigned int> & marked_stop, map_retour_t &retour, map_int_pint_t &best, best_dest &b_dest, unsigned int count);

};

struct reverseRAPTOR : public monoRAPTOR {
    reverseRAPTOR(navitia::type::Data &data) : monoRAPTOR(data){}
    void boucleRAPTOR(std::vector<unsigned int> &marked_stop, map_retour_t &retour, map_int_pint_t &best, best_dest &b_dest, unsigned int & count);
    Path makePath(const map_retour_t &retour, const map_int_pint_t &best, map_int_pint_t departs, unsigned int destination_idx, unsigned int countb);
};





//struct FpRAPTOR {
//    std::vector<Bag> bags;

//    void earliest_trip();
//    void update();
//    void compute();

//    struct label : public label_v {

//        boost::unordered_map<critere, int> criteres;
//        int stid;

//        label() : criteres() {
//            criteres[temps_trajet] = std::numeric_limits<int>::max();
//            criteres[via] = std::numeric_limits<int>::max();
//            criteres[jour] = std::numeric_limits<int>::max();
//            criteres[heure] = std::numeric_limits<int>::max();
//            stid = -1;
//        }
//        label(critere crit, int value) : criteres() {
//            criteres[temps_trajet] = std::numeric_limits<int>::max();
//            criteres[via] = std::numeric_limits<int>::max();
//            criteres[jour] = std::numeric_limits<int>::max();
//            criteres[heure] = std::numeric_limits<int>::max();
//            stid = -1;

//            criteres[crit] = value; }

//        label& ajouter_critere(critere crit, int value) {
//            criteres[crit] = value;
//            return *this;
//        }

//        int& operator[](critere crit) {return criteres[crit];}

//        int operator[](critere const crit) const {return criteres.at(crit);}

//        bool operator<(const label &lbl) const {
//            BOOST_FOREACH(auto p_crit, this->criteres) {
//                if(p_crit.first != via) {
//                    if(lbl.criteres.at(p_crit.first) >= this->criteres.at(p_crit.first))
//                        return false;
//                }
//                else {
//                    if(lbl.criteres.at(p_crit.first) <= this->criteres.at(p_crit.first))
//                        return false;
//                }

//            }
//            return true;
//        }
//        std::ostream& operator<<(std::ostream& out) {
//            out << this->criteres[temps_trajet] << " " << this->criteres[jour] << " " << this->criteres[heure];
//            return out;
//        }

//        bool dominated_by(label lbl) {
//            return ((this->criteres[temps_trajet] >= lbl[temps_trajet])
//                    && ((this->criteres[jour] > lbl[jour]) || (this->criteres[jour] == lbl[jour] && this->criteres[heure]%86400 >= lbl[heure]%86400) ))
//                    || (this->criteres[via] > lbl[via]);
//        }

//        bool dominates(label lbl) {
//            return this->criteres[temps_trajet] < lbl[temps_trajet] && ((this->criteres[jour] < lbl[jour]) ||((this->criteres[jour] == lbl[jour]) && (this->criteres[heure] < lbl[heure])));
//        }


//    };

//};

}}}
