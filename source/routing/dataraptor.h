#pragma once
#include "type/pt_data.h"
#include "type/datetime.h"
#include "routing/raptor_utils.h"

#include <boost/foreach.hpp>
#include <boost/dynamic_bitset.hpp>
namespace navitia { namespace routing {

/** Données statiques qui ne sont pas modifiées pendant le calcul */
struct dataRAPTOR {

    typedef std::pair<int, int> pair_int;
    typedef std::vector<navitia::type::idx_t> vector_idx;

    std::vector<const navitia::type::StopPointConnection*> foot_path_forward;
    std::vector<pair_int> footpath_index_forward;
    std::vector<const navitia::type::StopPointConnection*> foot_path_backward;
    std::vector<pair_int> footpath_index_backward;
    std::multimap<navitia::type::idx_t,const navitia::type::JourneyPatternPointConnection*> footpath_rp_forward;
    std::multimap<navitia::type::idx_t,const navitia::type::JourneyPatternPointConnection*> footpath_rp_backward;
    std::vector<uint32_t> arrival_times;
    std::vector<uint32_t> departure_times;
    std::vector<uint32_t> start_times_frequencies;
    std::vector<uint32_t> end_times_frequencies;
    std::vector<type::StopTime*> st_idx_forward;// Nom a changer ce ne sont plus des idx mais des pointeurs
    std::vector<type::StopTime*> st_idx_backward;
    std::vector<size_t> first_stop_time;
    std::vector<size_t> nb_trips;
    label_vector_t labels_const;
    label_vector_t labels_const_reverse;
    vector_idx boardings_const;
    std::vector<boost::dynamic_bitset<> > jp_validity_patterns;
    std::vector<boost::dynamic_bitset<> > jp_adapted_validity_pattern;


    dataRAPTOR()  {}
    void load(const navitia::type::PT_Data &data);

    const std::multimap<navitia::type::idx_t, const navitia::type::JourneyPatternPointConnection*> & footpath_rp(bool forward) const {
        if(forward)
            return footpath_rp_forward;
        else
            return footpath_rp_backward;
    }

    inline int get_stop_time_order(const type::JourneyPattern & journey_pattern, int orderVj, int order) const{
        return first_stop_time[journey_pattern.idx] + (order * nb_trips[journey_pattern.idx]) + orderVj;
    }

    inline uint32_t get_arrival_time(const type::JourneyPattern & journey_pattern, int orderVj, int order) const{
        if(orderVj < 0)
            return std::numeric_limits<uint32_t>::max();
        else
            return arrival_times[get_stop_time_order(journey_pattern, orderVj, order)];
    }

    inline uint32_t get_departure_time(const type::JourneyPattern & journey_pattern, int orderVj, int order) const{
        if(orderVj < 0)
            return std::numeric_limits<uint32_t>::max();
        else
            return departure_times[get_stop_time_order(journey_pattern, orderVj, order)];
    }

    inline type::idx_t get_route_connection_idx(type::idx_t jpp_idx_origin, type::idx_t jpp_idx_destination, bool clockwise,const navitia::type::PT_Data &/*data*/)  const {
        BOOST_FOREACH(auto conn, footpath_rp(clockwise).equal_range(jpp_idx_origin)) {
            if(conn.second->destination->idx == jpp_idx_destination)
                return conn.second->idx;
        }
        return type::invalid_idx;
    }

    inline type::idx_t get_stop_point_connection_idx(type::idx_t stop_point_idx_origin, type::idx_t stop_point_idx_destination, bool clockwise,const navitia::type::PT_Data &/*data*/)  const {

        const std::vector<const type::StopPointConnection*> &foot_path = clockwise ? foot_path_forward : foot_path_backward;
        const std::vector<pair_int> &footpath_index = clockwise ? footpath_index_forward : footpath_index_backward;
        for(int i = footpath_index[stop_point_idx_origin].first; i < footpath_index[stop_point_idx_origin].second; ++i) {
            auto conn = foot_path[i];
            if(conn->destination->idx == stop_point_idx_destination) {
                return conn->idx;
            }
        }
        return type::invalid_idx;
    }
};

}}

