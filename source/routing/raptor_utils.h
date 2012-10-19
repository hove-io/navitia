#pragma once
#include <unordered_map>
#include "routing/routing.h"
namespace navitia { namespace routing { namespace raptor{

enum type_idx {
    vj,
    connection,
    uninitialized,
    depart,
    connection_extension,
    connection_guarantee
};
struct type_retour {
    type::idx_t stop_time_idx;
    int rpid_embarquement;
    DateTime arrival, departure;
    type_idx type;

    type_retour(const DateTime & arrival, const DateTime & departure) : stop_time_idx(navitia::type::invalid_idx),
        rpid_embarquement(navitia::type::invalid_idx), arrival(arrival), departure(departure), type(depart) {}

    type_retour(const type::StopTime & st, const DateTime & date, int embarquement, bool clockwise) : 
        stop_time_idx(st.idx), rpid_embarquement(embarquement),
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

    type_retour(const DateTime & arrival, const DateTime & departure, int embarquement) : 
        stop_time_idx(navitia::type::invalid_idx), rpid_embarquement(embarquement),
        arrival(arrival), departure(departure), type(connection) {}

    type_retour(const DateTime & arrival, const DateTime & departure, int embarquement, const type_idx & type) :
        stop_time_idx(navitia::type::invalid_idx), rpid_embarquement(embarquement),
        arrival(arrival), departure(departure), type(type) {}

    type_retour() : 
        stop_time_idx(type::invalid_idx), rpid_embarquement(type::invalid_idx), arrival(),
        departure(DateTime::min), type(uninitialized) {}

    type_retour(const type_retour & t) : 
        stop_time_idx(t.stop_time_idx), rpid_embarquement(t.rpid_embarquement), arrival(t.arrival),
        departure(t.departure), type(t.type) {}

};

struct best_dest {
//    typedef std::pair<unsigned int, int> idx_dist;

    std::/*unordered_*/map<unsigned int, float> map_date_time;
    type_retour best_now;
    unsigned int best_now_rpid;
    unsigned int count;

    void ajouter_destination(unsigned int rpid, const float dist_to_dest) {
        if(rpid == 127839)
            std::cout << "Ajouter distance " << dist_to_dest << std::endl;
        map_date_time[rpid] = dist_to_dest;
    }

    bool is_dest(unsigned int rpid) const {return map_date_time.find(rpid) != map_date_time.end();}

    bool ajouter_best(unsigned int rpid, const type_retour &t, int cnt) {
        auto it = map_date_time.find(rpid);
        if(it != map_date_time.end()) {
            if(t.arrival + it->second <= best_now.arrival) {
                best_now = t;
                best_now.arrival = best_now.arrival + it->second;
                best_now.departure = best_now.departure + it->second;
                best_now_rpid = rpid;
                count = cnt;
                return true;
            }
        }
        return false;
    }

    bool ajouter_best_reverse(unsigned int rpid, const type_retour &t, int cnt) {
        auto it = map_date_time.find(rpid);
        if(it != map_date_time.end()) {
            if(t.departure != DateTime::min && (t.departure - it->second) >= best_now.departure) {
                best_now = t;
                best_now.arrival = t.arrival - it->second;
                best_now.departure = t.departure - it->second;
                best_now_rpid = rpid;
                count = cnt;
                return true;
            }
        }
        return false;
    }

    void reinit() {
        map_date_time.clear();
        best_now = type_retour();
        best_now_rpid = std::numeric_limits<unsigned int>::max();
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
