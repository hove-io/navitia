#pragma once
#include "raptor_utils.h"
#include "type/pt_data.h"
namespace navitia { namespace routing { namespace raptor {
typedef std::pair<int, int> pair_int;
typedef std::vector<type_retour> map_int_pint_t;
typedef std::map<navitia::type::idx_t, navitia::type::Connection> list_connections;
typedef std::vector<navitia::type::idx_t> vector_idx;
struct dataRAPTOR {

    struct Route_t {
        int nbTrips, nbStops;
        navitia::type::idx_t firstStopTime, idx;
        navitia::type::idx_t vp;
    };

    struct StopTime_t {
        uint32_t departure_time, arrival_time;
        navitia::type::idx_t idx;

        StopTime_t() : departure_time(std::numeric_limits<uint32_t>::max()), arrival_time(std::numeric_limits<uint32_t>::max()), idx(navitia::type::invalid_idx) {}
        StopTime_t(const navitia::type::StopTime & st) : departure_time(st.departure_time), arrival_time(st.arrival_time), idx(st.idx) {}
    };

    //Données statiques
    std::vector<navitia::type::Connection> foot_path;
    std::vector<pair_int> footpath_index;
    std::vector<Route_t> routes;
    std::vector<uint32_t> arrival_times;
    std::vector<uint32_t> departure_times;
    std::vector<type::idx_t> st_idx_forward;
    std::vector<type::idx_t> st_idx_backward;
    std::vector<pair_int> sp_indexrouteorder;
    std::vector<pair_int> sp_routeorder_const;
    std::vector<pair_int> sp_indexrouteorder_reverse;
    std::vector<pair_int> sp_routeorder_const_reverse;
    map_int_pint_t retour_constant;
    map_int_pint_t retour_constant_reverse;

    dataRAPTOR()  {}
    void load(const navitia::type::PT_Data &data);


    inline int get_stop_time_order(const Route_t & route, int orderVj, int order) const{
        return route.firstStopTime + (order * route.nbTrips) + orderVj;
    }
    inline uint32_t get_arrival_time(const Route_t & route, int orderVj, int order) const{
        if(orderVj == -1)
            return std::numeric_limits<uint32_t>::max();
        else
            return arrival_times[get_stop_time_order(route, orderVj, order)] % 86400;
    }
    inline uint32_t get_departure_time(const Route_t & route, int orderVj, int order) const{
        if(orderVj == -1)
            return std::numeric_limits<uint32_t>::max();
        else
            return departure_times[get_stop_time_order(route, orderVj, order)] % 86400;
    }
};

}}}

