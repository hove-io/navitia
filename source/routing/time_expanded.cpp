#include "time_expanded.h"


namespace navitia { namespace routing { namespace timeexpanded {


TimeExpanded::TimeExpanded(type::PT_Data & data) : data(data),
    graph(data.stop_areas.size() + data.stop_points.size() + data.route_points.size() + data.stop_times.size() * 2),
    astar_graph(data),
    map_tc(),
    map_td(),
    stop_area_offset(0),
    stop_point_offset(data.stop_areas.size()),
    route_point_offset(stop_point_offset + data.stop_points.size()),
    ta_offset(route_point_offset + data.route_points.size()),
    td_offset(ta_offset + data.stop_times.size()),
    tc_offset(td_offset + data.stop_times.size()) {}


void TimeExpanded::build_graph() {
    vertex_t tav, tdv, tcv, rpv;
    int min_corresp = 2 * 60;
    navitia::type::ValidityPattern vptemp;

    typedef std::set<vertex_t, sort_edges> set_vertices;
    typedef std::map<vertex_t, set_vertices> map_t;

    map_t map_stop_areas;

    //Creation des stop areas
        BOOST_FOREACH(navitia::type::StopArea sai, data.stop_areas) {
            map_stop_areas.insert(std::pair<vertex_t, set_vertices>(sai.idx, set_vertices(sort_edges(this))));
        }
    //Creation des stop points et creations lien SA->SP
    BOOST_FOREACH(navitia::type::StopPoint spi, data.stop_points) {
        add_edge(spi.stop_area_idx, get_sp_idx(spi.idx), EdgeDesc(0), graph);
    }

    //Création des liens SP->RP
    BOOST_FOREACH(navitia::type::RoutePoint rpi, data.route_points) {
        add_edge(get_sp_idx(rpi.stop_point_idx), get_rp_idx(rpi.idx), EdgeDesc(0), graph);
    }


    BOOST_FOREACH(navitia::type::VehicleJourney vj,  data.vehicle_journeys) {
        tdv = 0;
        unsigned int vp_idx = vj.validity_pattern_idx;
        BOOST_FOREACH(unsigned int stid, vj.stop_time_list) {
            rpv = get_rp_idx(data.stop_times.at(stid).route_point_idx);


            //ta
            tav = get_ta_idx(stid) ;
            if(tdv != 0) {
                if(is_passe_minuit(get_time(tdv), get_time(tav))) {
                    vptemp = *(decalage_pam(data.validity_patterns.at(vj.validity_pattern_idx)));
                    vp_idx = get_validity_pattern_idx(vptemp);
                }
                add_edge(tdv, tav, EdgeDesc(vp_idx, calc_diff(get_time(tdv), get_time(tav)), is_passe_minuit(get_time(tdv), get_time(tav))), graph);
            }

            //td
            tdv = get_td_idx(stid);

            //tc
            tcv = add_vertex(graph);
            map_tc.insert(std::pair<idx_t, idx_t>(tcv, tdv));
            map_stop_areas.find(data.stop_points.at(data.route_points.at(get_idx(rpv)).stop_point_idx).stop_area_idx)->second.insert(tcv);
            //edges


            if(is_passe_minuit(get_time(tav), get_time(tdv))) {
                vptemp = *decalage_pam(data.validity_patterns.at(vj.validity_pattern_idx));
                vp_idx = get_validity_pattern_idx(vptemp);
            }
            add_edge(tav, tdv, EdgeDesc(vp_idx, calc_diff(get_time(tav), get_time(tdv)),is_passe_minuit(get_time(tav), get_time(tdv))), graph);
            add_edge(tcv, tdv, EdgeDesc(0), graph);
        }
    }


    BOOST_FOREACH(map_t::value_type sa, map_stop_areas) {
        //On fabrique la chaine de tc
        vertex_t prec = num_vertices(graph) + 1;
        set_vertices::iterator itc, pam = sa.second.begin();
        itc = sa.second.begin();
        pam = sa.second.begin();
        BOOST_FOREACH(vertex_t tc, sa.second) {
            if((prec != (num_vertices(graph) + 1)) && !edge(prec, tc, graph).second && (prec != tc)) {
                add_edge(prec, tc, EdgeDesc(0, data.stop_times.at(get_idx(tc)).departure_time - data.stop_times.at(get_idx(prec)).departure_time, false), graph);
            }
            prec = tc;
            //Je relie le ta au premier tc possible
            while((itc != sa.second.end()) & ((data.stop_times.at(get_idx(tc)).arrival_time + min_corresp % 86400) > get_time(*itc) % 86400) )
                ++itc;
            if((itc != sa.second.end()) && ((data.stop_times.at(get_idx(tc)).arrival_time  + min_corresp% 86400) <= (get_time(*itc) % 86400))) {
                if(!edge(get_ta_idx(get_idx(tc)), *itc, graph).second)
                    add_edge(get_ta_idx(get_idx(tc)), *itc,
                             EdgeDesc(0, (data.stop_times.at(get_idx(tc)).arrival_time %86400) - data.stop_times.at(get_idx(tc)).departure_time,
                                      is_passe_minuit(get_time(get_ta_idx(get_idx(tc))), get_time(*itc))), graph);
            } else {
                if(!edge(get_ta_idx(get_idx(tc)), *itc, graph).second)
                    add_edge(get_ta_idx(get_idx(tc)), *sa.second.begin(),
                             EdgeDesc(0, (86400 - data.stop_times.at(get_idx(tc)).departure_time % 86400) + data.stop_times.at(get_idx(tc)).arrival_time %86400,
                                      is_passe_minuit(get_time(get_ta_idx(get_idx(tc))), get_time(*sa.second.begin()))), graph);
            }

//            //Si > 86400 je relie au premier tc possible avec le temps % 86400
//            if((data.stop_times.at(get_idx(tc, data, map_tc)).arrival_time + min_corresp) > 86400) {
//                while((pam != sa.second.end()) & ((data.stop_times.at(get_idx(tc, data, map_tc)).arrival_time%86400 + min_corresp )> get_time(*pam, data, g, map_tc)) )
//                    ++pam;
//                if(get_ta_idx(get_idx(tc, data, map_tc), data) == 33)
//                    std::cout << "T1 : " <<(pam != sa.second.end()) << " "  << (data.stop_times.at(get_idx(tc, data, map_tc)).arrival_time%86400) + min_corresp <<" " << get_time(*pam, data, g, map_tc) << std::endl  ;
//                if(pam != sa.second.end())
//                    if((pam != sa.second.end()) & ((data.stop_times.at(get_idx(tc, data, map_tc)).arrival_time%86400) + min_corresp <= get_time(*pam, data, g, map_tc)))
//                        add_edge(get_ta_idx(get_idx(tc, data, map_tc), data), *pam, EdgeDesc(0), g);
//            }
        }
        if(sa.second.size() > 1) {
            if(!edge(*(--sa.second.end()), *sa.second.begin(), graph).second) {
                add_edge(*(--sa.second.end()), *sa.second.begin(), EdgeDesc(0, true), graph);
            }
        }


        //On lie les RP a premier TC de la chaine
        BOOST_FOREACH(edge_t e1, out_edges(sa.first, graph)) {
            if(get_n_type(target(e1, graph)) == SP) {
                BOOST_FOREACH(edge_t e2, out_edges(target(e1, graph), graph)) {
                    if(get_n_type(target(e2, graph)) == RP) {
                        add_edge(target(e2, graph), *sa.second.begin(), EdgeDesc(0), graph);
                    }
                }
            }
        }

    }


    BOOST_FOREACH(auto pairtc, map_tc) {
        map_td.insert(std::pair<unsigned int, unsigned int>(pairtc.second, pairtc.first));
    }
}

idx_t TimeExpanded::get_idx(const vertex_t& v) const {
    if(v < stop_area_offset + data.stop_areas.size())
        return v;
    else if(v < stop_point_offset + data.stop_points.size())
        return v - stop_point_offset;
    else if(v < route_point_offset + data.route_points.size())
        return v - route_point_offset;
    else if(v < ta_offset + data.stop_times.size())
        return v - ta_offset;
    else if(v < td_offset + data.stop_times.size())
        return v - td_offset;
    else {
        return get_idx(map_tc.at(v));
    }
}

node_type TimeExpanded::get_n_type(const vertex_t& v) const {
    if(v < stop_area_offset + data.stop_areas.size())
        return SA;
    else if(v < stop_point_offset + data.stop_points.size())
        return SP;
    else if(v < route_point_offset + data.route_points.size())
        return RP;
    else if(v < ta_offset + data.stop_times.size())
        return TA;
    else if(v < td_offset + data.stop_times.size())
        return TD;
    else
        return TC;
}

int TimeExpanded::get_validity_pattern_idx(navitia::type::ValidityPattern vp) {
    BOOST_FOREACH(navitia::type::ValidityPattern vpi, data.validity_patterns) {
        unsigned int i=0;
        bool test = true;
        while((i<366) & test) {
            test = vpi.days[i] == vp.days[i];
            ++i;
        }

        if(test)
            return vpi.idx;
    }
    vp.idx = data.validity_patterns.size();

    data.validity_patterns.push_back(vp);


    return vp.idx;
}


navitia::type::ValidityPattern * TimeExpanded::decalage_pam(const navitia::type::ValidityPattern &vp) {
    navitia::type::ValidityPattern *vpr = new navitia::type::ValidityPattern(vp.beginning_date);

    for(unsigned int i=0; i < 366; i++) {
        vpr->days[i+1] = vp.days[i];
    }
    vpr->days[0] = vp.days[365];

    return vpr;
}


int32_t TimeExpanded::get_time(const vertex_t& v) const {
    switch(get_n_type(v)) {
    case TA:
        return data.stop_times.at(get_idx(v)).arrival_time;
        break;
    case TD:
        return data.stop_times.at(get_idx(v)).departure_time;
        break;
    case TC:
        return get_time(map_tc.at(v));
        break;
    default:
        return -1;
    }
}

bool TimeExpanded::is_passe_minuit(int32_t debut_t, int32_t fin_t) {
    return ((debut_t % 86400) > (fin_t % 86400)) & (debut_t != -1) & (fin_t!=-1);
}

 Path TimeExpanded::compute(idx_t departure_idx, idx_t destination_idx, int departure_hour, int departure_day) {
    Path result;
    return result;
 }


}}}
