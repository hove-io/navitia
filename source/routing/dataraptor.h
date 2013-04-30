#pragma once
#include "type/pt_data.h"
#include "type/datetime.h"

#include <boost/foreach.hpp>
namespace navitia { namespace routing {

/** Données statiques qui ne sont pas modifiées pendant le calcul */
struct dataRAPTOR {


    typedef std::pair<int, int> pair_int;
    typedef std::vector<type::DateTime> label_vector_t;
    typedef std::map<navitia::type::idx_t, navitia::type::Connection> list_connections;
    typedef std::vector<navitia::type::idx_t> vector_idx;



    std::vector<navitia::type::Connection> foot_path_forward;
    std::vector<pair_int> footpath_index_forward;
    std::vector<navitia::type::Connection> foot_path_backward;
    std::vector<pair_int> footpath_index_backward;
    std::multimap<navitia::type::idx_t, navitia::type::Connection> footpath_rp_forward;
    std::multimap<navitia::type::idx_t, navitia::type::Connection> footpath_rp_backward;
    std::vector<uint32_t> arrival_times;
    std::vector<uint32_t> departure_times;
    std::vector<uint32_t> start_times_frequencies;
    std::vector<uint32_t> end_times_frequencies;
    std::vector<std::bitset<366>> validity_patterns;
    std::vector<type::idx_t> st_idx_forward;
    std::vector<type::idx_t> st_idx_backward;
    std::vector<type::idx_t> vp_idx_forward;
    std::vector<type::idx_t> vp_idx_backward;
    std::vector<size_t> first_stop_time;
    std::vector<size_t> nb_trips;
    std::vector<size_t> first_frequency;
    std::vector<size_t> nb_frequencies;
    label_vector_t labels_const;
    label_vector_t labels_const_reverse;
    vector_idx boardings_const;

    dataRAPTOR()  {}
    void load(const navitia::type::PT_Data &data);

    const std::multimap<navitia::type::idx_t, navitia::type::Connection> & footpath_rp(bool forward) const {
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

    inline type::idx_t get_connection_idx(type::idx_t jpp_idx_origin, type::idx_t jpp_idx_destination, bool clockwise,const navitia::type::PT_Data &data)  const {
        BOOST_FOREACH(auto conn, footpath_rp(clockwise).equal_range(jpp_idx_origin)) {
            if(conn.second.destination_idx == jpp_idx_destination)
                return conn.second.idx;
        }
        
        const std::vector<type::Connection> &foot_path = clockwise ? foot_path_forward : foot_path_backward;
        const std::vector<pair_int> &footpath_index = clockwise ? footpath_index_forward : footpath_index_backward;
        type::idx_t origin_stop_point_idx = data.journey_pattern_points[jpp_idx_origin].stop_point_idx,
                    destination_stop_point_idx = data.journey_pattern_points[jpp_idx_destination].stop_point_idx;
        for(int i = footpath_index[origin_stop_point_idx].first; i < footpath_index[origin_stop_point_idx].second; ++i) {
            auto &conn = foot_path[i];
            if(conn.destination_idx == destination_stop_point_idx) {
                return conn.idx;
            }
        }
        return type::invalid_idx;
    }
};

}}

