#include "thermometer.h"
#include "ptreferential/ptreferential.h"
#include "time.h"
#include <boost/graph/graphviz.hpp> //DEBUG


namespace navitia { namespace timetables {


uint32_t /*Thermometer::*/get_lower_bound(const std::vector<vector_idx> &routes) {
    std::unordered_map<type::idx_t, uint32_t> min_spid_occurences;


    for(auto route : routes) {
        std::unordered_map<type::idx_t, uint32_t> to_count;
        for(auto i : route) {
            if(to_count.find(i) == to_count.end())
                to_count.insert(std::make_pair(i,1));
            else
                ++to_count[i];
        }

        for(auto p : to_count) {
            if(min_spid_occurences.find(p.first) == min_spid_occurences.end())
                min_spid_occurences.insert(std::make_pair(p.first, p.second));
            else if(p.second > min_spid_occurences[p.first])
                min_spid_occurences[p.first] = p.second;
        }
    }

    uint32_t result = 0;

    for(auto p : min_spid_occurences)
        result += p.second;
    return result;
}

std::pair<vector_idx, bool> Thermometer::recc(std::vector<vector_idx> &routes, const uint32_t upper_bound_, int depth) {
    ++depth;
    ++debug_nb_branches;
    uint32_t upper_bound = upper_bound_;
    std::vector<type::idx_t> result;
    unsigned int lower_bound = get_lower_bound(routes);

    if(lower_bound >= upper_bound){
        return std::make_pair(vector_idx(), false);
   }


    std::vector<type::idx_t> possibilities = generate_possibilities(routes);
    bool res_bool = possibilities.size() == 0;
    for(auto poss_spidx : possibilities) {
        std::vector<uint32_t> to_retail = untail(routes, poss_spidx);
        uint32_t u = upper_bound;
        if(u != std::numeric_limits<uint32_t>::max())
            --u;
        std::pair<vector_idx, bool> tmp = recc(routes, u, depth);
        retail(routes, poss_spidx, to_retail);
        if(tmp.second){
            tmp.first.push_back(poss_spidx);
            if(u == std::numeric_limits<uint32_t>::max()|| upper_bound > tmp.first.size()) {
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

    if(result.size() == 0 && res_bool)
        std::cout << depth <<  " " << std::flush;

    return std::make_pair(result, res_bool);
}

void Thermometer::generate_thermometer(std::vector<vector_idx> routes) {
    debug_nb_branches = 0; debug_nb_cuts = 0; upper_cut = 0, nb_opt = 0;
    if(routes.size() > 1) {
        std::vector<vector_idx> tmp;
        vector_idx answer;
        for(auto r : routes) {
            if(tmp.empty())
                tmp.push_back(r);
            else {
                tmp.push_back(r);
                for(unsigned int i=0; i < tmp.size(); ++i) {
                    std::cout << "Route " << i << " : ";
                    for(auto sp : tmp[i])
                        std::cout << sp << " ";
                    std::cout << std::endl;
                }
                answer = recc(tmp, type::invalid_idx).first;
                tmp.clear();
                tmp.push_back(answer);
            }
        }

        thermometer = answer/*recc(routes, type::invalid_idx).first*/;
    } else {
        thermometer = routes.back();
    }


    std::cout << "Nb branch : " << debug_nb_branches << " nb cuts: " << debug_nb_cuts<< " nb upper cuts " << upper_cut << " nb optimal : " << nb_opt << std::endl;
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

vector_idx Thermometer::generate_possibilities(const std::vector<vector_idx> &routes) {


    vector_idx result;
    std::vector<uint32_t> tmp;
    std::unordered_map<type::idx_t, size_t> sp_count;

    for(uint32_t i=0; i<routes.size(); ++i) {
        if(routes[i].size() > 0) {
            bool is_optimal = true;
            for(uint32_t j=0; j<routes.size(); ++j) {
                if(i!=j)
                    is_optimal = is_optimal && std::find(routes[j].begin(), routes[j].end(), routes[i].back()) == routes[j].end();
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
        return  (sp_count[routes[r1].back()]<sp_count[routes[r2].back()]) || (sp_count[routes[r1].back()]==sp_count[routes[r2].back()] && routes[r1].size() > routes[r2].size()) ;});

    for(auto i : tmp) {
        if(std::find(result.begin(), result.end(), routes[i].back()) == result.end())
            result.push_back(routes[i].back());
    }



    return result;
}

std::vector<uint32_t> Thermometer::untail(std::vector<vector_idx> &routes, type::idx_t spidx) {
    std::vector<uint32_t> result;
    if(spidx != type::invalid_idx) {
        for(unsigned int i=0; i<routes.size(); ++i) {
            auto & route = routes[i];
            if(route.size() > 0 && route.back() == spidx) {
                route.pop_back();
                result.push_back(i);
            }
        }
    }
    return result;
}

void Thermometer::retail(std::vector<vector_idx> &routes, type::idx_t spidx, const std::vector<uint32_t> &to_retail) {
    for(auto i : to_retail) {
        routes[i].push_back(spidx);
    }
}

}}
