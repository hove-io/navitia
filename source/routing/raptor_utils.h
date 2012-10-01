#pragma once
#include <unordered_map>
#include "routing/routing.h"
namespace navitia { namespace routing { namespace raptor{

enum type_idx {
    vj,
    connection,
    uninitialized,
    depart
};
struct type_retour {
    unsigned int stid;
    int spid_emarquement;
    DateTime dt;
    int dist_to_dest;
    int dist_to_dep;
    type_idx type;


    type_retour(int stid, DateTime dt, int dist_to_dest) : stid(stid), spid_emarquement(-1), dt(dt), dist_to_dest(dist_to_dest), dist_to_dep(0), type(vj) {}
    type_retour(int stid, DateTime dt, int dist_to_dest, int dist_to_dep) : stid(stid), spid_emarquement(-1), dt(dt), dist_to_dest(dist_to_dest), dist_to_dep(dist_to_dep), type(vj) {}

    type_retour(int stid, DateTime dt) : stid(stid), spid_emarquement(-1), dt(dt), dist_to_dest(0), dist_to_dep(0), type(vj){}
    type_retour(int stid, DateTime dt, type_idx type) : stid(stid), spid_emarquement(-1), dt(dt), dist_to_dest(0), dist_to_dep(0), type(type){}
    type_retour(int stid, int said_emarquement,DateTime dt) : stid(stid), spid_emarquement(said_emarquement), dt(dt), dist_to_dest(0), dist_to_dep(0), type(vj){}

    type_retour(int stid, int said_emarquement, DateTime dt, type_idx type) : stid(stid), spid_emarquement(said_emarquement), dt(dt), dist_to_dest(0), dist_to_dep(0), type(type){}
    type_retour(unsigned int dist_to_dest) : stid(-1), spid_emarquement(-1), dt(), dist_to_dest(dist_to_dest), dist_to_dep(0), type(vj){}
    type_retour() : stid(-1), spid_emarquement(-1), dt(), dist_to_dest(0), dist_to_dep(0), type(uninitialized) {}
    type_retour(const type_retour & t) : stid(t.stid), spid_emarquement(t.spid_emarquement), dt(t.dt), dist_to_dest(t.dist_to_dest), dist_to_dep(t.dist_to_dep), type(t.type) {}

    bool operator<(type_retour r2) const {
        if(r2.dt == DateTime::inf)
            return true;
        else if(this->dt == DateTime::inf)
            return false;
        else
            return this->dt + this->dist_to_dest < r2.dt + dist_to_dest;
    }

    bool operator>(type_retour r2) const {
        if(r2.dt == DateTime::min)
            return true;
        else if(this->dt == DateTime::min)
            return false;
        else
            return this->dt + this->dist_to_dep > r2.dt + dist_to_dep;
    }

    bool operator>=(type_retour r2) const {
        if(r2.dt == DateTime::min)
            return true;
        else if(this->dt == DateTime::min)
            return false;
        else
            return this->dt - this->dist_to_dep >= r2.dt - dist_to_dep;
    }

    bool operator==(type_retour r2) const { return this->stid == r2.stid && this->dt == r2.dt; }
    bool operator!=(type_retour r2) const { return this->stid != r2.stid || this->dt != r2.dt;}
};

struct best_dest {
//    typedef std::pair<unsigned int, int> idx_dist;

    std::unordered_map<unsigned int, type_retour> map_date_time;
    type_retour best_now;
    unsigned int best_now_spid;
    unsigned int count;

    void ajouter_destination(unsigned int spid, const type_retour &t) { map_date_time[spid] = t;}

    bool is_dest(unsigned int spid) {return map_date_time.find(spid) != map_date_time.end();}

    bool ajouter_best(unsigned int spid, const type_retour &t, int cnt) {
        auto it = map_date_time.find(spid);
        if(it != map_date_time.end()) {
            if(t.dt + it->second.dist_to_dest <= best_now.dt) {
                best_now = t;
                best_now.dt = best_now.dt + it->second.dist_to_dest;
                best_now_spid = spid;
                count = cnt;
            }
            it->second = t;
            return true;
        }
        return false;
    }

    bool ajouter_best_reverse(unsigned int spid, const type_retour &t, int cnt) {
        auto it = map_date_time.find(spid);
        if(it != map_date_time.end()) {
            it->second = t;
            if(t >= best_now && t.dt != DateTime::min) {
                best_now = t;
                best_now_spid = spid;
                count = cnt;
            }
            return true;
        }
        return false;
    }

    void reinit() {
        map_date_time.clear();
        best_now = type_retour();
        best_now_spid = std::numeric_limits<unsigned int>::max();
        count = 0;
    }

    void reinit(DateTime borne) {
        reinit();
        best_now.dt = borne;
    }

    void reverse() {
        best_now.dt = DateTime::min;
    }


};

}}}
