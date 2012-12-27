#pragma once
#include <unordered_map>
#include "routing/routing.h"
namespace navitia { namespace routing { namespace raptor{
//Forward Declaration
struct type_retour;

typedef std::pair<int, int> pair_int;
typedef std::vector<type_retour> map_int_pint_t;
typedef std::map<navitia::type::idx_t, navitia::type::Connection> list_connections;
typedef std::vector<navitia::type::idx_t> vector_idx;
typedef std::pair<navitia::type::idx_t, int> pair_idx_int;
typedef std::vector<int> queue_t;
typedef std::vector<map_int_pint_t> map_retour_t;
typedef std::vector<pair_int> vector_pairint;
typedef std::pair<navitia::type::idx_t, type_retour> idx_retour;
typedef std::vector<idx_retour> vector_idxretour;
typedef std::pair<navitia::type::idx_t, double> idx_distance;
typedef std::vector<idx_distance> vec_idx_distance;

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

//    std::unordered_map<unsigned int, float> map_date_time;
    std::vector<float> rpidx_distance;
    type_retour best_now;
    unsigned int best_now_rpid;
    unsigned int count;

    void ajouter_destination(unsigned int rpid, const float dist_to_dest) {
//        map_date_time[rpid] = dist_to_dest;
        rpidx_distance[rpid] = dist_to_dest;
    }

    bool is_dest(unsigned int rpid) const {return rpidx_distance[rpid] != std::numeric_limits<float>::max();}

    bool ajouter_best(unsigned int rpid, const type_retour &t, int cnt) {
//        auto it = map_date_time.find(rpid);
        if(rpidx_distance[rpid] != std::numeric_limits<float>::max()) {
            if(t.arrival + rpidx_distance[rpid] <= best_now.arrival) {
                best_now = t;
                best_now.arrival = best_now.arrival + rpidx_distance[rpid];
                best_now.departure = best_now.departure + rpidx_distance[rpid];
                best_now_rpid = rpid;
                count = cnt;
                return true;
            }
        }
        return false;
    }

    bool ajouter_best_reverse(unsigned int rpid, const type_retour &t, int cnt) {
//        auto it = map_date_time.find(rpid);
//        if(it != map_date_time.end()) {

        if(rpidx_distance[rpid] != std::numeric_limits<float>::max()) {
            if(t.departure != DateTime::min && (t.departure - rpidx_distance[rpid]) >= best_now.departure) {
                best_now = t;
                best_now.arrival = t.arrival - rpidx_distance[rpid];
                best_now.departure = t.departure - rpidx_distance[rpid];
                best_now_rpid = rpid;
                count = cnt;
                    return true;
                }
            }
        return false;
    }

    void reinit(size_t nb_rpid) {
//        map_date_time.clear();
        rpidx_distance.clear();
        rpidx_distance.insert(rpidx_distance.end(), nb_rpid, std::numeric_limits<float>::max());
        best_now = type_retour();
        best_now_rpid = std::numeric_limits<unsigned int>::max();
        count = 0;
    }

    void reinit(size_t nb_rpid, const DateTime &borne, const bool clockwise) {
        reinit(nb_rpid);
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
