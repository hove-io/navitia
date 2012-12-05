#include "thermometer.h"
#include "ptreferential/ptreferential.h"
#include "time.h"
#include <boost/graph/graphviz.hpp> //DEBUG


namespace navitia { namespace timetables {


uint32_t /*Thermometer::*/get_lower_bound(std::vector<vector_size> &pre_computed_lb, type::idx_t max_sp) {


    vector_size tmp;
    tmp.insert(tmp.begin(), max_sp+1, 0);
    for(unsigned int i=0; i<pre_computed_lb.size(); ++i) {

        for(unsigned int j=0; j<pre_computed_lb[i].size(); ++j) {

            if(tmp[j] < pre_computed_lb[i][j])
                tmp[j] = pre_computed_lb[i][j];
        }
    }

    uint32_t result = 0;
    for(auto p : tmp)
        result += p;


    return result;
}

std::vector<vector_size> pre_compute_lower_bound(const std::vector<vector_idx> &routes, type::idx_t max_sp) {
    std::vector<vector_size> result;
    for(auto route : routes) {
        result.push_back(vector_size());
        result.back().insert(result.back().begin(), max_sp+1, 0);
        for(auto sp : route) {
            ++result.back()[sp];
        }
    }

    return result;
}

std::pair<vector_idx, bool> Thermometer::recc(std::vector<vector_idx> &routes, std::vector<vector_size> &pre_computed_lb, type::idx_t max_sp, const uint32_t lower_bound_, const uint32_t upper_bound_, int depth) {
    ++depth;
    ++debug_nb_branches;
    uint32_t lower_bound = lower_bound_, upper_bound = upper_bound_;
    std::vector<type::idx_t> result;


    std::vector<type::idx_t> possibilities = generate_possibilities(routes, pre_computed_lb);
    bool res_bool = possibilities.size() == 0;
    for(auto poss_spidx : possibilities) {
        if(debug_nb_branches > 10000 && (result.size() > 0))
            break;
        std::vector<uint32_t> to_retail = untail(routes, poss_spidx, pre_computed_lb);
        uint32_t u = upper_bound;
        if(u != std::numeric_limits<uint32_t>::max())
            --u;
        uint32_t l = get_lower_bound(pre_computed_lb, max_sp);

        if(l >= u){
            retail(routes, poss_spidx, to_retail, pre_computed_lb);
            upper_cut++;
        } else {
            std::pair<vector_idx, bool> tmp = recc(routes, pre_computed_lb, max_sp, l, u, depth);
            retail(routes, poss_spidx, to_retail, pre_computed_lb);
            if(tmp.second){
                tmp.first.push_back(poss_spidx);
                if(!res_bool|| (tmp.second && tmp.first.size() < upper_bound)) {
                    result = tmp.first;
                    res_bool = true;
                    upper_bound = result.size();
                }

                if(upper_bound == lower_bound) {
                    ++debug_nb_cuts;
                    break;
                }
            } else {
                upper_cut++;
            }
        }

    }

    return std::make_pair(result, res_bool);
}

uint32_t get_max_sp(std::vector<vector_idx> &routes) {

    uint32_t max_sp = std::numeric_limits<uint32_t>::min();

    for(auto route : routes) {
        for(auto i : route) {
            if(i > max_sp)
                max_sp = i;
        }
    }

    return max_sp;
}

void Thermometer::generate_thermometer(std::vector<vector_idx> &routes) {

    uint32_t max_sp = get_max_sp(routes);
    debug_nb_branches = 0; debug_nb_cuts = 0; upper_cut = 0;
    std::vector<vector_idx> req;
    for(auto v : routes) {
        if(req.empty())
            req.push_back(v);
        else {
            debug_nb_branches = 0; debug_nb_cuts = 0; upper_cut = 0;
            req.push_back(v);
            auto plb = pre_compute_lower_bound(req, max_sp);
            auto tp = recc(req, plb, max_sp).first;
            req.clear();
            req.push_back(tp);
        }
    }
    thermometer = req.back();
}


vector_idx Thermometer::get_thermometer(std::vector<vector_idx> routes) {
    generate_thermometer(routes);
    return thermometer;
}

vector_idx Thermometer::get_thermometer(std::string filter_) {
    if(filter_ != filter && filter_ != "") {
        filter = filter_;
        std::vector<vector_idx> routes;
        for(type::idx_t route_idx : ptref::make_query(navitia::type::Type_e::eRoute, filter, d)) {
            routes.push_back(vector_idx());
            for(type::idx_t rpidx : d.pt_data.routes[route_idx].route_point_list)
                routes.back().push_back(d.pt_data.route_points[rpidx].stop_point_idx);
        }

        generate_thermometer(routes);
    }
    return thermometer;
}


std::vector<uint32_t> Thermometer::match_route(const type::Route & route) {
    std::vector<type::idx_t> tmp;
    for(auto rpidx : route.route_point_list)
        tmp.push_back(d.pt_data.route_points[rpidx].stop_point_idx);
    
    return match_route(tmp);
}

std::vector<uint32_t> Thermometer::match_route(const vector_idx &stop_point_list) {
    std::vector<uint32_t> result;
    auto it = thermometer.begin();
    for(type::idx_t spidx : stop_point_list) {
        it = std::find(it, thermometer.end(),  spidx);
        if(it==thermometer.end())
            throw cant_match(spidx);
        else {
            result.push_back( distance(thermometer.begin(), it));
        }
        ++it;
    }
    return result;

}

vector_idx Thermometer::generate_possibilities(const std::vector<vector_idx> &routes, std::vector<vector_size> &pre_computed_lb) {

    vector_idx result;

    std::vector<uint32_t> tmp;
    std::unordered_map<type::idx_t, size_t> sp_count;
    for(uint32_t i=0; i<routes.size(); ++i) {
        if(routes[i].size() > 0) {bool is_optimal = true;
            for(uint32_t j=0; j<routes.size(); ++j) {
                if(i!=j)
                    is_optimal = is_optimal &&  pre_computed_lb[j][routes[i].back()] == 0;
            }
            if(is_optimal) {
                ++nb_opt;
                return {routes[i].back()};
            }

            tmp.push_back(i);
            if(sp_count.find(routes[i].back()) == sp_count.end())
                sp_count[routes[i].back()] = 1;
            else
                ++sp_count[routes[i].back()];
        }
    }

    std::sort(tmp.begin(), tmp.end(), [&](uint32_t r1, uint32_t r2) {
            return  (sp_count[routes[r1].back()]>sp_count[routes[r2].back()]) || (sp_count[routes[r1].back()]==sp_count[routes[r2].back()] && routes[r1].size() > routes[r2].size()) ;});

    for(auto i : tmp) {
        if(std::find(result.begin(), result.end(), routes[i].back()) == result.end())
            result.push_back(routes[i].back());
    }


    return result;
}

std::vector<uint32_t> Thermometer::untail(std::vector<vector_idx> &routes, type::idx_t spidx, std::vector<vector_size> &pre_computed_lb) {
    std::vector<uint32_t> result;
    if(spidx != type::invalid_idx) {
        for(unsigned int i=0; i<routes.size(); ++i) {
            auto & route = routes[i];
            if(route.size() > 0 && route.back() == spidx) {
                route.pop_back();
                result.push_back(i);
                if(pre_computed_lb[i][spidx] > 0)
                    --pre_computed_lb[i][spidx];
            }
        }
    }
    return result;
}

void Thermometer::retail(std::vector<vector_idx> &routes, type::idx_t spidx, const std::vector<uint32_t> &to_retail, std::vector<vector_size> &pre_computed_lb) {
    for(auto i : to_retail) {
        routes[i].push_back(spidx);
        ++pre_computed_lb[i][spidx];
    }
}

}}
