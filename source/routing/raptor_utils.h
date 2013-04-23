#pragma once
#include <unordered_map>
#include "type/data.h"
#include "type/datetime.h"
#include "best_trip.h"

namespace navitia { namespace routing {

typedef std::pair<int, int> pair_int;
typedef std::vector<type::DateTime> label_vector_t;
typedef std::map<navitia::type::idx_t, navitia::type::Connection> list_connections;
typedef std::vector<navitia::type::idx_t> vector_idx;
typedef std::pair<navitia::type::idx_t, int> pair_idx_int;
typedef std::vector<int> queue_t;
typedef std::vector<pair_int> vector_pairint;
typedef std::pair<navitia::type::idx_t, type::DateTime> idx_label;
typedef std::vector<idx_label> vector_idxlabel;
typedef std::pair<navitia::type::idx_t, double> idx_distance;
typedef std::vector<idx_distance> vec_idx_distance;

enum class boarding_type {
    vj,
    connection,
    uninitialized,
    departure,
    connection_extension,
    connection_guarantee
};

struct best_dest {
    std::vector<float> rpidx_distance;
    type::DateTime best_now;
    unsigned int best_now_rpid;
    unsigned int count;
    float max_walking;

    void add_destination(unsigned int rpid, const float dist_to_dest) {
        rpidx_distance[rpid] = dist_to_dest;
    }

    bool is_dest(unsigned int rpid) const {return rpidx_distance[rpid] != std::numeric_limits<float>::max();}

    bool add_best(unsigned int rpid, const type::DateTime &t, int cnt, bool clockwise = true) {
        if(!clockwise)
            return add_best_reverse(rpid, t, cnt);
        if(rpidx_distance[rpid] != std::numeric_limits<float>::max()) {
            if((best_now == navitia::type::DateTime::inf) ||
               ((t != navitia::type::DateTime::inf) && (t + rpidx_distance[rpid]) <= (best_now + max_walking))) {
                best_now = t + rpidx_distance[rpid];
                //best_now.departure = best_now.departure + rpidx_distance[rpid];
                best_now_rpid = rpid;
                count = cnt;
                return true;
            }
        }
        return false;
    }

    bool add_best_reverse(unsigned int rpid, const type::DateTime &t, int cnt) {
        if(rpidx_distance[rpid] != std::numeric_limits<float>::max()) {
            if((best_now.datetime <= max_walking) ||
               (t != navitia::type::DateTime::min && (t - rpidx_distance[rpid]) >= (best_now - max_walking))) {
                best_now = t - rpidx_distance[rpid];
                //best_now.departure = t.departure - rpidx_distance[rpid];
                best_now_rpid = rpid;
                count = cnt;
                    return true;
                }
            }
        return false;
    }

    void reinit(const size_t nb_rpid, const float max_walking_ = 0) {
        rpidx_distance.clear();
        rpidx_distance.insert(rpidx_distance.end(), nb_rpid, std::numeric_limits<float>::max());
        best_now = type::DateTime();
        best_now_rpid = type::invalid_idx;
        count = 0;
        max_walking = max_walking_;
    }

    void reinit(size_t nb_rpid, const type::DateTime &borne, const bool /*clockwise*/, const float max_walking = 0) {
        reinit(nb_rpid, max_walking);
        best_now = borne;
    }

    void reverse() {
        best_now = type::DateTime::min;
    }


};

boarding_type get_type(size_t count, type::idx_t journey_pattern_point, const std::vector<label_vector_t> &labels, const std::vector<vector_idx> &boardings, const navitia::type::Data &data);
type::idx_t get_boarding_jpp(size_t count, type::idx_t journey_pattern_point, const std::vector<label_vector_t> &labels, const std::vector<vector_idx> &boardings, const navitia::type::Data &data);
std::pair<type::idx_t, uint32_t> get_current_stidx_gap(size_t count, type::idx_t journey_pattern_point, const std::vector<label_vector_t> &labels, const std::vector<vector_idx> &boardings, const type::Properties &required_properties, bool clockwise, const navitia::type::Data &data);

}}
