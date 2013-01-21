#pragma once
#include <unordered_map>
#include "routing/routing.h"
namespace navitia { namespace routing { namespace raptor{
//Forward Declaration
struct label;

typedef std::pair<int, int> pair_int;
typedef std::vector<label> map_int_pint_t;
typedef std::map<navitia::type::idx_t, navitia::type::Connection> list_connections;
typedef std::vector<navitia::type::idx_t> vector_idx;
typedef std::pair<navitia::type::idx_t, int> pair_idx_int;
typedef std::vector<int> queue_t;
typedef std::vector<map_int_pint_t> map_labels_t;
typedef std::vector<pair_int> vector_pairint;
typedef std::pair<navitia::type::idx_t, label> idx_label;
typedef std::vector<idx_label> vector_idxlabel;
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

struct label {
    type::idx_t stop_time_idx;
    int rpid_embarquement;
    DateTime arrival, departure;
    type_idx type;

    label(const DateTime & arrival, const DateTime & departure) : stop_time_idx(navitia::type::invalid_idx),
        rpid_embarquement(navitia::type::invalid_idx), arrival(arrival), departure(departure), type(depart) {}

    label(const type::StopTime & st, const DateTime & date, int embarquement, bool clockwise, uint32_t gap) :
        stop_time_idx(st.idx), rpid_embarquement(embarquement),
        type(vj) {
        if(clockwise) {
            arrival = date;
            departure = arrival;
            departure.update(!st.is_frequency() ? st.departure_time : st.start_time +gap);
        } else {
            departure = date;
            arrival = departure;
            arrival.updatereverse(!st.is_frequency() ? st.arrival_time : st.start_time + gap);
        }
    }

    label(const DateTime & arrival, const DateTime & departure, int embarquement) :
        stop_time_idx(navitia::type::invalid_idx), rpid_embarquement(embarquement),
        arrival(arrival), departure(departure), type(connection) {}

    label(const DateTime & arrival, const DateTime & departure, int embarquement, const type_idx & type) :
        stop_time_idx(navitia::type::invalid_idx), rpid_embarquement(embarquement),
        arrival(arrival), departure(departure), type(type) {}

    label() :
        stop_time_idx(type::invalid_idx), rpid_embarquement(type::invalid_idx), arrival(),
        departure(DateTime::min), type(uninitialized) {}

    label(const label & t) :
        stop_time_idx(t.stop_time_idx), rpid_embarquement(t.rpid_embarquement), arrival(t.arrival),
        departure(t.departure), type(t.type) {}

};

struct best_dest {
    std::vector<float> rpidx_distance;
    label best_now;
    unsigned int best_now_rpid;
    unsigned int count;

    void add_destination(unsigned int rpid, const float dist_to_dest) {
        rpidx_distance[rpid] = dist_to_dest;
    }

    bool is_dest(unsigned int rpid) const {return rpidx_distance[rpid] != std::numeric_limits<float>::max();}

    bool add_best(unsigned int rpid, const label &t, int cnt, bool clockwise = true) {
        if(!clockwise)
            return add_best_reverse(rpid, t, cnt);
        if(rpidx_distance[rpid] != std::numeric_limits<float>::max()) {
            if((t.arrival + rpidx_distance[rpid]) <= best_now.arrival) {
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

    bool add_best_reverse(unsigned int rpid, const label &t, int cnt) {
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
        best_now = label();
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
