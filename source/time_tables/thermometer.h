#pragma once
#include "type/data.h"
#include "boost/functional/hash.hpp"
namespace navitia { namespace timetables {

typedef std::vector<type::idx_t> vector_idx;
typedef std::vector<size_t> vector_size;

struct Thermometer {
    type::Data& d;

    Thermometer(type::Data &d) : d(d), filter("") {}

    vector_idx get_thermometer(std::string filter = "");
    vector_idx get_thermometer(std::vector<vector_idx> routes);
    std::vector<uint32_t> match_route(const type::Route & route);
    std::vector<uint32_t> match_route(const vector_idx &route);

    void set_thermometer(const vector_idx &thermometer_) {//Pour debug
        thermometer = thermometer_;
    }

    struct cant_match {
        type::idx_t rp_idx;
        cant_match() : rp_idx(type::invalid_idx) {}
        cant_match(type::idx_t rp_idx) : rp_idx(rp_idx) {}
    };

    struct upper {};

private :
    vector_idx thermometer;
    std::string filter;
    int debug_nb_branches, debug_nb_cuts, upper_cut, nb_opt;

    void generate_thermometer(std::vector<vector_idx> &routes);
    std::vector<uint32_t> untail(std::vector<vector_idx> &routes, type::idx_t spidx, std::vector<vector_size> &pre_computed_lb);
    void retail(std::vector<vector_idx> &routes, type::idx_t spidx, const std::vector<uint32_t> &to_retail, std::vector<vector_size> &pre_computed_lb);
    vector_idx generate_possibilities(const std::vector<vector_idx> &routes, std::vector<vector_size> &pre_computed_lb);
    std::pair<vector_idx, bool> recc(std::vector<vector_idx> &routes, std::vector<vector_size> &pre_computed_lb, type::idx_t max_sp, const uint32_t lower_bound_ = std::numeric_limits<uint32_t>::min(), const uint32_t upper_bound_ = std::numeric_limits<uint32_t>::max(), int depth = 0);
};
uint32_t get_lower_bound(std::vector<vector_size> &pre_computed_lb);



}}
