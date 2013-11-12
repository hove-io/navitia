#pragma once
#include <unordered_map>
#include "type/data.h"
#include "type/datetime.h"
#include "best_stoptime.h"

namespace navitia { namespace routing {

typedef std::pair<int, int> pair_int;
typedef std::vector<DateTime> label_vector_t;
typedef std::vector<navitia::type::idx_t> vector_idx;
typedef std::pair<navitia::type::idx_t, int> pair_idx_int;
typedef std::vector<int> queue_t;
typedef std::vector<pair_int> vector_pairint;
typedef std::pair<navitia::type::idx_t, DateTime> idx_label;
typedef std::vector<idx_label> vector_idxlabel;
typedef std::pair<navitia::type::idx_t, double> idx_distance;
typedef std::vector<idx_distance> vec_idx_distance;

template<typename T>
inline void memset32(T*buf, uint n, T c)
{
    for(register uint i = 0; i < n; i++) *buf++ = c;
}

enum class boarding_type {
    vj,
    connection,
    uninitialized,
    departure,
    connection_stay_in,
    connection_guarantee
};

struct best_dest {
    std::vector<u_int32_t> jpp_idx_duration;
    DateTime best_now;
    unsigned int best_now_jpp_idx;
    size_t count;
    float max_walking;

    void add_destination(type::idx_t jpp_idx, const float duration_to_dest, bool clockwise) {
        jpp_idx_duration[jpp_idx] = clockwise ? std::ceil(duration_to_dest) : std::floor(duration_to_dest);
    }


    inline bool is_eligible_solution(type::idx_t jpp_idx) {
        return jpp_idx_duration[jpp_idx] != std::numeric_limits<u_int32_t>::max();
    }


    template<typename Visitor>
    inline bool add_best(const Visitor & v, type::idx_t jpp_idx, const DateTime &t, size_t cnt) {
        if(is_eligible_solution(jpp_idx)) {
            if(v.clockwise())
                return add_best_clockwise(jpp_idx, t, cnt);
            else
                return add_best_unclockwise(jpp_idx, t, cnt);
        }
        return false;
    }


    inline bool add_best_clockwise(type::idx_t jpp_idx, const DateTime &t, size_t cnt) {
        if(t != DateTimeUtils ::inf) {
            const auto tmp_dt = t + jpp_idx_duration[jpp_idx];
            if((tmp_dt < best_now) || ((tmp_dt == best_now) && (count > cnt))) {
                best_now = tmp_dt;
                //best_now.departure = best_now.departure + jpp_idx_distance[rpid];
                best_now_jpp_idx = jpp_idx;
                count = cnt;
                return true;
            }
         }

        return false;
    }

    inline bool add_best_unclockwise(type::idx_t jpp_idx, const DateTime &t, size_t cnt) {
        if(t != DateTimeUtils::min) {
            const auto tmp_dt = t - jpp_idx_duration[jpp_idx];
            if((tmp_dt > best_now) || ((tmp_dt == best_now) && (count > cnt))) {
                best_now = tmp_dt;
                //best_now.departure = t.departure - jpp_idx_distance[rpid];
                best_now_jpp_idx = jpp_idx;
                count = cnt;
                return true;
            }
        }
        return false;
    }

    void reinit(const size_t nb_jpp_idx, const float max_walking_ = 0) {
        jpp_idx_duration.resize(nb_jpp_idx);
        memset32<u_int32_t>(&jpp_idx_duration[0], nb_jpp_idx, std::numeric_limits<u_int32_t>::max());
        best_now = DateTimeUtils::inf;
        best_now_jpp_idx = type::invalid_idx;
        count = std::numeric_limits<size_t>::max();
        max_walking = max_walking_;
    }

    void reinit(size_t nb_jpp_idx, const DateTime &borne, const float max_walking = 0) {
        reinit(nb_jpp_idx, max_walking);
        best_now = borne;
    }

    void reverse() {
        best_now = DateTimeUtils::min;
    }
};

inline
boarding_type get_type(size_t count, type::idx_t journey_pattern_point, const std::vector<std::vector<boarding_type> > &boarding_types, const navitia::type::Data &/*data*/){
    return boarding_types[count][journey_pattern_point];
}

const type::JourneyPatternPoint* get_boarding_jpp(size_t count, type::idx_t journey_pattern_point, const std::vector<std::vector<const type::JourneyPatternPoint *> >&boardings);
std::pair<const type::StopTime*, uint32_t> get_current_stidx_gap(size_t count, type::idx_t journey_pattern_point, const std::vector<label_vector_t> &labels, const std::vector<std::vector<boarding_type> >&boarding_types, const type::AccessibiliteParams & accessibilite_params/*const type::Properties &required_properties*/, bool clockwise, const navitia::type::Data &data);

}}
