#pragma once
#include "type/type.h"
#include "type/data.h"
#include "routing.h"
#include "utils/timer.h"
#include <unordered_map>
#include "boost/dynamic_bitset.hpp"
#include <google/dense_hash_map>
namespace navitia { namespace routing { namespace raptor{

template<class T>
size_t hash_value(T t) {
    return static_cast<size_t>(t);
}

enum type_idx {
    vj,
    connection,
    uninitialized
};


struct type_retour {
    unsigned int stid;
    int said_emarquement;
    DateTime dt;
    int dist_to_dest;
    int dist_to_dep;
    type_idx type;


    type_retour(int stid, DateTime dt, int dist_to_dest) : stid(stid), said_emarquement(-1), dt(dt), dist_to_dest(dist_to_dest), dist_to_dep(0), type(vj) {}
    type_retour(int stid, DateTime dt, int dist_to_dest, int dist_to_dep) : stid(stid), said_emarquement(-1), dt(dt), dist_to_dest(dist_to_dest), dist_to_dep(dist_to_dep), type(vj) {}

    type_retour(int stid, DateTime dt) : stid(stid), said_emarquement(-1), dt(dt), dist_to_dest(0), dist_to_dep(0), type(vj){}
    type_retour(int stid, int said_emarquement,DateTime dt) : stid(stid), said_emarquement(said_emarquement), dt(dt), dist_to_dest(0), dist_to_dep(0), type(vj){}

    type_retour(int stid, int said_emarquement, DateTime dt, type_idx type) : stid(stid), said_emarquement(said_emarquement), dt(dt), dist_to_dest(0), dist_to_dep(0), type(type){}
    type_retour(unsigned int dist_to_dest) : stid(-1), said_emarquement(-1), dt(), dist_to_dest(dist_to_dest), dist_to_dep(0), type(vj){}
    type_retour() : stid(-1), said_emarquement(-1), dt(), dist_to_dest(0), dist_to_dep(0), type(uninitialized) {}

    bool operator<(type_retour r2) const {
        if(r2.dt == DateTime::inf)
            return true;
        else if(this->dt == DateTime::inf)
            return false;
        else
            return this->dt + this->dist_to_dest < r2.dt + dist_to_dest;
    }
    bool operator==(type_retour r2) const { return this->stid == r2.stid && this->dt == r2.dt; }
    bool operator!=(type_retour r2) const { return this->stid != r2.stid || this->dt != r2.dt;}
};

struct best_dest {
//    typedef std::pair<unsigned int, int> idx_dist;

    std::map<unsigned int, type_retour> map_date_time;
    type_retour best_now;
    unsigned int best_now_spid;

    void ajouter_destination(unsigned int spid, type_retour &t) { map_date_time[spid] = t;}

    bool ajouter_best(unsigned int spid, type_retour t) {
        if(map_date_time.find(spid) != map_date_time.end()) {
            map_date_time[spid] = t;
            if(t < best_now) {
                best_now = t;
                best_now_spid = spid;
            }
            return true;
        }
        return false;
    }

    void ajouter_best_reverse(unsigned int said, type_retour t) {
        if(map_date_time.find(said) != map_date_time.end()) {
            map_date_time[said] = t;
            if(best_now < t) {
                best_now = t;
                best_now_spid = said;
            }
        }
    }

    void reinit() {
        map_date_time.clear();
        best_now = type_retour();
        best_now_spid = 0;
    }

    void reverse() {
        best_now.dt = DateTime::min;
    }

};


typedef std::pair<int, int> pair_int;

typedef google::dense_hash_map<int, int> map_int_int_t;
typedef std::vector<type_retour> map_int_pint_t;
typedef std::pair<navitia::type::idx_t, int> pair_idx_int;
//typedef google::dense_hash_map<navitia::type::idx_t, int> queue_t;
typedef std::vector<int> queue_t;



//typedef std::unordered_map<unsigned int, map_int_pint_t> map_retour_t;
typedef std::vector<map_int_pint_t> map_retour_t;

//typedef std::pair<map_retour_t, map_best_t> pair_retour_t;








struct communRAPTOR : public AbstractRouter
{
    typedef std::vector<navitia::type::idx_t> vector_idx;
    typedef std::vector<pair_int> vector_pairint;
    typedef std::pair<navitia::type::idx_t, type_retour> idx_retour;
    typedef std::vector<idx_retour> vector_idxretour;
    struct compare_rp {
        unsigned int order;
        const navitia::type::Data &data;
        compare_rp(const navitia::type::Data &data) : order(0), data(data) {}

        bool operator ()(unsigned int vj1, int time) {
            return (data.pt_data.stop_times[data.pt_data.vehicle_journeys[vj1].stop_time_list[order]].departure_time %86400) < time;
        }
    };


    struct Route_t {
        int nbTrips, nbStops;
        navitia::type::idx_t firstStopTime, idx;
        navitia::type::ValidityPattern vp;
    };

    struct StopTime_t {
        uint32_t departure_time, arrival_time;
        navitia::type::idx_t idx;

        StopTime_t() : departure_time(std::numeric_limits<uint32_t>::max()), arrival_time(std::numeric_limits<uint32_t>::max()), idx(navitia::type::invalid_idx) {}
        StopTime_t(navitia::type::StopTime & st) : departure_time(st.departure_time), arrival_time(st.arrival_time), idx(st.idx) {}
    };

    struct Connection_t {
        navitia::type::idx_t departure_sp, destination_sp, connection_idx;
        int duration;

        Connection_t(navitia::type::idx_t departure_sp, navitia::type::idx_t destination_sp, navitia::type::idx_t connection_idx, int duration) :
            departure_sp(departure_sp), destination_sp(destination_sp), connection_idx(connection_idx), duration(duration) {}

        Connection_t(navitia::type::idx_t departure_sp, navitia::type::idx_t destination_sp, int duration) :
            departure_sp(departure_sp), destination_sp(destination_sp), connection_idx(navitia::type::invalid_idx), duration(duration) {}
    };

    typedef std::vector<Connection_t> list_connections;



    struct compare_rp_reverse {
        const int order;
        const navitia::type::Data &data;
        compare_rp_reverse(const int order, const navitia::type::Data &data) : order(order), data(data) {}

        bool operator ()(unsigned int vj1, int time) {
            return (data.pt_data.stop_times.at(data.pt_data.vehicle_journeys.at(vj1).stop_time_list.at(order)).arrival_time %86400) > time;
        }
    };

    navitia::type::Data &data;
    compare_rp cp;
//    google::dense_hash_map<unsigned int, list_connections> foot_path;
    std::vector<Connection_t> foot_path;
    std::vector<Route_t> routes;
    std::vector<StopTime_t> stopTimes;
    std::vector<pair_int> footpath_index;
    std::vector<pair_int> sp_indexrouteorder;
    std::vector<pair_int> sp_routeorder;
//    std::vector<vector_pairint> sp_routeorder;
    communRAPTOR(navitia::type::Data &data);

    virtual Path compute_raptor(vector_idxretour departs, vector_idxretour destinations) = 0;
    virtual Path compute_raptor_reverse(vector_idxretour departs, vector_idxretour destinations) = 0;
    virtual Path compute_raptor_rabattement(vector_idxretour departs, vector_idxretour destinations) = 0;
    Path compute(idx_t departure_idx, idx_t destination_idx, int departure_hour, int departure_day, senscompute sens);
    Path compute_reverse(idx_t departure_idx, idx_t destination_idx, int departure_hour, int departure_day);
    Path compute_rabattement(idx_t departure_idx, idx_t destination_idx, int departure_hour, int departure_day);
    Path compute(const type::GeographicalCoord & departure, double radius, idx_t destination_idx, int departure_hour, int departure_day);
    Path compute(const type::GeographicalCoord & departure, double radius_depart, const type::GeographicalCoord & destination, double radius_destination
                 , int departure_hour, int departure_day);

    void trouverGeo(const type::GeographicalCoord & departure, double radius_depart, const type::GeographicalCoord & destination, double radius_destination,
                    int departure_hour, int departure_day, map_int_pint_t &departs, map_int_pint_t &destinations);

    void trouverGeo(const type::GeographicalCoord & departure, double radius_depart, const type::GeographicalCoord & destination, double radius_destination,
                    int departure_hour, int departure_day, vector_idxretour &departs, vector_idxretour &destinations);


    int earliest_trip(const Route_t &route, unsigned int order, DateTime dt, int orderVj);
    int tardiest_trip(const Route_t &route, unsigned int order, DateTime dt, int orderVj);

    inline int get_stop_time_idx(const Route_t & route, int orderVj, int order) {
        return route.firstStopTime + (orderVj * route.nbStops) + order;
    }

    inline int get_sa_rp(unsigned int order, int route) {
        return data.pt_data.stop_points[data.pt_data.route_points[data.pt_data.routes[routes[route].idx].route_point_list[order]].stop_point_idx].stop_area_idx;
    }

};

struct departureNotFound {};
struct destinationNotFound {};
struct RAPTOR : public communRAPTOR {
    map_int_pint_t retour_constant;
    map_int_pint_t retour_constant_reverse;
    RAPTOR(navitia::type::Data &data) : communRAPTOR(data), retour_constant(data.pt_data.stop_points.size()), retour_constant_reverse(data.pt_data.stop_points.size()){
        BOOST_FOREACH(auto r, retour_constant_reverse) {
            r.dt = DateTime::inf;
        }
    }

    Path compute_raptor(vector_idxretour departs, vector_idxretour destinations);
    Path compute_raptor_reverse(vector_idxretour departs, vector_idxretour destinations);
    Path compute_raptor_rabattement(vector_idxretour departs, vector_idxretour destinations);


    Path makeBestPath(map_retour_t &retour, map_int_pint_t &best, vector_idxretour departs, unsigned int destination_idx, unsigned int count);
    Path makeBestPathreverse(map_retour_t &retour, map_int_pint_t &best, vector_idxretour departs, unsigned int destination_idx, unsigned int count);
    std::vector<Path> makePathes(map_retour_t &retour, map_int_pint_t &best, vector_idxretour departs, best_dest &b_dest, unsigned int count);
    std::vector<Path> compute_all(const type::GeographicalCoord & , double , const type::GeographicalCoord & , double
                               , int , int );
    std::vector<Path> compute_all(vector_idxretour departs, vector_idxretour destinations);
    std::vector<Path> compute_all(navitia::type::EntryPoint departure, navitia::type::EntryPoint destination, int departure_hour, int departure_day);

    void boucleRAPTOR(std::vector<unsigned int> &marked_stop, map_retour_t &retour, map_int_pint_t &best, best_dest &b_dest, unsigned int & count);
    Path makePath(map_retour_t &retour, map_int_pint_t &best, vector_idxretour departs, unsigned int destination_idx, unsigned int countb);
    void marcheapied(boost::dynamic_bitset<> &marked_stop, map_retour_t &retour, map_int_pint_t &best, best_dest &b_dest, unsigned int count);
    void setRoutesValides(boost::dynamic_bitset<> & routesValides, std::vector<unsigned int> &marked_stop, map_retour_t &retour);
    inline uint32_t get_temps_depart(const Route_t & route, int orderVj, int order) {
        if(orderVj == -1)
            return std::numeric_limits<uint32_t>::max();
        else
            return stopTimes[get_stop_time_idx(route, orderVj, order)].departure_time % 86400;
    }
    void make_queue(boost::dynamic_bitset<> &stops, boost::dynamic_bitset<> & routesValides, queue_t &Q);

    void boucleRAPTORreverse(std::vector<unsigned int> &marked_stop, map_retour_t &retour, map_int_pint_t &best, best_dest &b_dest, unsigned int & count);
    Path makePathreverse(map_retour_t &retour, map_int_pint_t &best, vector_idxretour departs, unsigned int destination_idx, unsigned int countb);
    void marcheapiedreverse(boost::dynamic_bitset<> & marked_stop, map_retour_t &retour, map_int_pint_t &best, best_dest &b_dest, unsigned int count);
    void setRoutesValidesreverse(boost::dynamic_bitset<> &routesValides, std::vector<unsigned int> &marked_stop, map_retour_t &retour);
    inline uint32_t get_temps_departreverse(const Route_t & route, int orderVj, int order) {
        if(orderVj == -1)
            return std::numeric_limits<uint32_t>::min();
        else
            return stopTimes[get_stop_time_idx(route, orderVj, order)].departure_time % 86400;
    }
    void make_queuereverse(boost::dynamic_bitset<> &stops, boost::dynamic_bitset<> & routesValides, queue_t &Q);
};



}}}
