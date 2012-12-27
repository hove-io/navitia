#include "thermometer.h"
#include "ptreferential/ptreferential.h"
#include "time.h"
#include <boost/graph/graphviz.hpp> //DEBUG


namespace navitia { namespace timetables {


uint32_t /*Thermometer::*/get_lower_bound(std::vector<vector_size> &pre_computed_lb, type::idx_t max_sp) {
    uint32_t result = 0;
    for(unsigned int sp = 0; sp < (max_sp +1); ++sp) {
        result += std::max(pre_computed_lb[0][sp], pre_computed_lb[1][sp]);
    }
    return result;
}

std::vector<vector_size> pre_compute_lower_bound(const std::vector<vector_idx> &routes, type::idx_t max_sp) {
    std::vector<vector_size> lower_bounds;
    for(auto route : routes) {
        lower_bounds.push_back(vector_size());
        lower_bounds.back().insert(lower_bounds.back().begin(), max_sp+1, 0);
        for(auto sp : route) {
            ++lower_bounds.back()[sp];
        }
    }
    return lower_bounds;
}

std::pair<vector_idx, bool> Thermometer::recc(std::vector<vector_idx> &routes, std::vector<vector_size> &pre_computed_lb, const uint32_t lower_bound_, type::idx_t max_sp, const uint32_t upper_bound_, int depth) {
    ++depth;
    ++debug_nb_branches;
    uint32_t lower_bound = lower_bound_, upper_bound = upper_bound_;
    std::vector<type::idx_t> result;


    std::vector<type::idx_t> possibilities = generate_possibilities(routes, pre_computed_lb);
    bool res_bool = possibilities.size() == 0;
    for(auto poss_spidx : possibilities) {
        if(debug_nb_branches > 5000 && (result.size() > 0))
            break;
        int temp1 = std::max(pre_computed_lb[0][poss_spidx], pre_computed_lb[1][poss_spidx]);
        std::vector<uint32_t> to_retail = untail(routes, poss_spidx, pre_computed_lb);
        lower_bound = lower_bound - temp1 + std::max(pre_computed_lb[0][poss_spidx], pre_computed_lb[1][poss_spidx]);

        uint32_t u = upper_bound;
        if(u != std::numeric_limits<uint32_t>::max())
            --u;

        if(lower_bound >= u) {
            int temp = std::max(pre_computed_lb[0][poss_spidx], pre_computed_lb[1][poss_spidx]);
            retail(routes, poss_spidx, to_retail, pre_computed_lb);
            lower_bound = lower_bound - temp + std::max(pre_computed_lb[0][poss_spidx], pre_computed_lb[1][poss_spidx]);
            upper_cut++;
        } else {
            std::pair<vector_idx, bool> tmp = recc(routes, pre_computed_lb, lower_bound, max_sp, u, depth);
            int temp = std::max(pre_computed_lb[0][poss_spidx], pre_computed_lb[1][poss_spidx]);
            retail(routes, poss_spidx, to_retail, pre_computed_lb);
            lower_bound = lower_bound - temp + std::max(pre_computed_lb[0][poss_spidx], pre_computed_lb[1][poss_spidx]);
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
    if(routes.size() > 1) {
        for(auto v : routes) {
            if(req.empty())
                req.push_back(v);
            else {
                debug_nb_branches = 0; debug_nb_cuts = 0; upper_cut = 0;
                req.push_back(v);
                auto plb = pre_compute_lower_bound(req, max_sp);
                uint32_t lowerbound = get_lower_bound(plb, max_sp);
                auto tp = recc(req, plb, lowerbound, max_sp).first;
                req.clear();

                req.push_back(tp);
            }
        }
        thermometer = req.back();
    } else if(routes.size() == 1){
        thermometer = routes.back();
    }

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

    //C'est qu'il n'y a pas de possibilités possibles
    if(routes[0].size() == 0 && routes[1].size() == 0)
        return {};


    //Si la route une est vide, ou bien si le dernier de la route n'est pas présent dans la route 0, on renvoie la tete de la route 1
    if((routes[0].size()==0) ||(routes[1].size() > 0 && pre_computed_lb[0][routes[1].back()] == 0) ) {
        ++nb_opt;
        return {routes[1].back()};
    } else if((routes[1].size()==0)||(routes[0].size() > 0 && pre_computed_lb[1][routes[0].back()] == 0)) { //Même chose mais avec la route 0
        ++nb_opt;
        return {routes[0].back()};
    }

    vector_idx result;

    std::vector<uint32_t> tmp;
    std::unordered_map<type::idx_t, size_t> sp_count;


    auto count1 = std::count(routes[0].begin(), routes[0].end(), routes[1].back());
    auto count2 = std::count(routes[1].begin(), routes[1].end(), routes[0].back());

    if(count1 > count2) {
        return {routes[0].back(), routes[1].back()};
    } else if(count1 < count2) {
        return {routes[1].back(), routes[0].back()};
    } else {
        if(routes[0].size() < routes[1].size())
            return {routes[0].back(), routes[1].back()};
        else
            return {routes[1].back(), routes[0].back()};
    }
    return {routes[0].back(), routes[1].back()};
}

std::vector<uint32_t> Thermometer::untail(std::vector<vector_idx> &routes, type::idx_t spidx, std::vector<vector_size> &pre_computed_lb) {
    std::vector<uint32_t> result;
    if(spidx != type::invalid_idx) {
        if((routes[0].size() > 0) && (routes[0].back() == spidx)) {
            routes[0].pop_back();
            result.push_back(0);
            --pre_computed_lb[0][spidx];
        }
        if((routes[1].size() > 0) && (routes[1].back() == spidx)) {
            routes[1].pop_back();
            result.push_back(1);
            --pre_computed_lb[1][spidx];
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
