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
    std::vector<float> rpidx_distance;
    DateTime best_now;
    unsigned int best_now_rpid;
    size_t count;
    float max_walking;

    void add_destination(unsigned int rpid, const float dist_to_dest) {
        rpidx_distance[rpid] = dist_to_dest;
    }

    bool is_dest(unsigned int rpid) const {return rpidx_distance[rpid] != std::numeric_limits<float>::max();}

    template<typename Visitor>
    inline bool add_best(const Visitor & v, unsigned int rpid, const DateTime &t, size_t cnt) {
        if(v.clockwise())
            return add_best_clockwise(rpid, t, cnt);
        else
            return add_best_unclockwise(rpid, t, cnt);
    }

    inline bool add_best_clockwise(unsigned int rpid, const DateTime &t, size_t cnt) {
        if(rpidx_distance[rpid] != std::numeric_limits<float>::max()) {
            if((best_now == DateTimeUtils::inf) ||
               ((t != DateTimeUtils::inf) &&
                (((t + rpidx_distance[rpid]) < best_now) || (((t + rpidx_distance[rpid]) == best_now) && (count > cnt))))) {
                best_now = t + rpidx_distance[rpid];
                //best_now.departure = best_now.departure + rpidx_distance[rpid];
                best_now_rpid = rpid;
                count = cnt;
                return true;
            }
        }
        return false;
    }

    inline bool add_best_unclockwise(unsigned int rpid, const DateTime &t, size_t cnt) {
        if(rpidx_distance[rpid] != std::numeric_limits<float>::max()) {
            if((best_now <= max_walking) ||
               ((t != DateTimeUtils::min) &&
                (((t - rpidx_distance[rpid]) > best_now) || (((t - rpidx_distance[rpid]) == best_now) && (count > cnt))))) {
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
        rpidx_distance.resize(nb_rpid);
        memset32<float>(&rpidx_distance[0], nb_rpid, std::numeric_limits<float>::max());
        best_now = DateTimeUtils::inf;
        best_now_rpid = type::invalid_idx;
        count = std::numeric_limits<size_t>::max();
        max_walking = max_walking_;
    }

    void reinit(size_t nb_rpid, const DateTime &borne, const float max_walking = 0) {
        reinit(nb_rpid, max_walking);
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
