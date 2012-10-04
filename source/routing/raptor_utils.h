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
    type::idx_t stop_time_idx;
    int spid_embarquement;
    DateTime arrival, departure;
    type_idx type;

    type_retour(const DateTime & arrival, const DateTime & departure) : stop_time_idx(navitia::type::invalid_idx),
        spid_embarquement(navitia::type::invalid_idx), arrival(arrival), departure(departure), type(depart) {}

    type_retour(const type::StopTime & st, const DateTime & date, int embarquement, bool clockwise) : stop_time_idx(st.idx), spid_embarquement(embarquement),
        type(vj) {
        if(clockwise) {
            arrival = date;
            departure = arrival;
            departure.update(st.departure_time);
        } else {
            departure = date;
            arrival = departure;
            arrival.updatereverse(st.arrival_time);
        }
        departure.normalize();
        arrival.normalize();
    }

    type_retour(const DateTime & arrival, const DateTime & departure, int embarquement) : stop_time_idx(navitia::type::invalid_idx),
        spid_embarquement(embarquement), arrival(arrival), departure(departure), type(connection) {}

    type_retour() : stop_time_idx(type::invalid_idx), spid_embarquement(-1), arrival(), departure(DateTime::min), type(uninitialized) {}
    type_retour(const type_retour & t) : stop_time_idx(t.stop_time_idx), spid_embarquement(t.spid_embarquement), arrival(t.arrival),
        departure(departure), type(t.type) {}

};

struct best_dest {
//    typedef std::pair<unsigned int, int> idx_dist;

    std::/*unordered_*/map<unsigned int, std::pair<int, int>> map_date_time;
    type_retour best_now;
    unsigned int best_now_spid;
    unsigned int count;

    void ajouter_destination(unsigned int spid, const int dist_to_dep, const int dist_to_dest) { 
        map_date_time[spid] = std::make_pair(dist_to_dep, dist_to_dest);
    }

    bool is_dest(unsigned int spid) const {return map_date_time.find(spid) != map_date_time.end();}

    bool ajouter_best(unsigned int spid, const type_retour &t, int cnt) {
        auto it = map_date_time.find(spid);
        if(it != map_date_time.end()) {
            if(t.arrival + it->second.second <= best_now.arrival) {
                best_now = t;
                best_now.arrival = best_now.arrival + it->second.second;
                best_now_spid = spid;
                count = cnt;
                return true;
            }
        }
        return false;
    }

    bool ajouter_best_reverse(unsigned int spid, const type_retour &t, int cnt) {
        auto it = map_date_time.find(spid);
        if(it != map_date_time.end()) {
            if(t.departure != DateTime::min && (t.departure - it->second.first) >= best_now.departure) {
                best_now = t;
                best_now.departure = best_now.departure - it->second.first;
                best_now_spid = spid;
                count = cnt;
                return true;
            }
        }
        return false;
    }

    void reinit() {
        map_date_time.clear();
        best_now = type_retour();
        best_now_spid = std::numeric_limits<unsigned int>::max();
        count = 0;
    }

    void reinit(const DateTime &borne, const bool clockwise) {
        reinit();
        if(clockwise)
            best_now.arrival = borne;
        else
            best_now.departure = borne;
    }

    void reverse() {
        best_now.departure = DateTime::min;
    }


};

}}}
