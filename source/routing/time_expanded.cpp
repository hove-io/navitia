#include "time_expanded.h"
#include "boost/timer.hpp"
#include "boost/lexical_cast.hpp"

namespace navitia { namespace routing { namespace timeexpanded {

class distance_heuristic : public boost::astar_heuristic<Graph, DateTime>
{
public:
    distance_heuristic(const TimeExpanded & te) : te(te){}

    // Estimation de l'heure d'arrivée. Cette heure doit est sous-estimée
    DateTime operator()(Vertex u){
        Vertex a = u;
        if(a > te.ta_offset)
            a = te.data.stop_times.at(te.get_idx(a)).route_point_idx;
        /*if(a > td.stop_area_offset)
          a -= td.stop_area_offset;
      else
          a = td.data.route_points[a].stop_point_idx;*/
        //BOOST_ASSERT(td.distance[u] == DateTime::infinity());
        DateTime result;

        result.hour = /*0;*/te.astar_graph.min_time.at(a);
        result.date = -1;
        return result;
    }
private:
    const TimeExpanded & te;
};

class astar_goal_visitor_te : public boost::default_astar_visitor
{
public:
    astar_goal_visitor_te(vertex_t goal, const navitia::routing::timeexpanded::TimeExpanded & te) : m_goal(goal), te(te) {}

    template<class Graph>
    void examine_vertex(vertex_t u, const Graph& ) const {
        if(te.get_saidx(u) == m_goal)
            throw found_goal(u);
    }
private:
    vertex_t m_goal;
    const navitia::routing::timeexpanded::TimeExpanded & te;
};


class sort_edges {
private:
    TimeExpanded & te;
public:
    sort_edges (TimeExpanded & te) : te(te) { }
    bool operator() (const vertex_t& v1, const vertex_t& v2) const {

        int32_t t1 = te.get_time(v1) % 86400,
                t2 = te.get_time(v2) % 86400;
        return (t1 < t2) || ((t1 == t2) & (te.get_idx(v1) < te.get_idx(v2)));
    }

};




TimeExpanded::TimeExpanded(const type::PT_Data & data) : data(data),
    graph(data.stop_areas.size() + data.stop_points.size() + data.route_points.size() + data.stop_times.size() * 3),
    astar_graph(data),
    predecessors(boost::num_vertices(graph)),
    distances(boost::num_vertices(graph)),
    astar_dist(boost::num_vertices(graph)),
    stop_area_offset(0),
    stop_point_offset(data.stop_areas.size()),
    route_point_offset(stop_point_offset + data.stop_points.size()),
    ta_offset(route_point_offset + data.route_points.size()),
    td_offset(ta_offset + data.stop_times.size()),
    tc_offset(td_offset + data.stop_times.size())
{}


void TimeExpanded::build_graph() {
    vertex_t tav, tdv, tcv, rpv;
    int min_corresp = 2 * 60;
    navitia::type::ValidityPattern vptemp;

    typedef std::set<vertex_t, sort_edges> set_vertices;
    typedef std::map<vertex_t, set_vertices> map_t;

    map_t map_stop_areas;

    //Creation des stop areas
    BOOST_FOREACH(navitia::type::StopArea sai, data.stop_areas) {
        map_stop_areas.insert(std::pair<vertex_t, set_vertices>(sai.idx, set_vertices(sort_edges(*this))));
    }
    //Creation des stop points et creations lien SA->SP
    BOOST_FOREACH(navitia::type::StopPoint spi, data.stop_points) {
        add_edge(spi.stop_area_idx, get_sp_idx(spi.idx), EdgeDesc(-1), graph);
    }

    //Création des liens SP->RP
    BOOST_FOREACH(navitia::type::RoutePoint rpi, data.route_points) {

        add_edge(get_sp_idx(rpi.stop_point_idx), get_rp_idx(rpi.idx), EdgeDesc(-1), graph);
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
            tcv = get_tc_idx(stid);
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
                add_edge(prec, tc, EdgeDesc(-1, calc_diff(get_time(prec)%86400, get_time(tc)%86400), false), graph);
            }
            prec = tc;
            //Je relie le ta au premier tc possible
            while(itc != sa.second.end()) {
                if(((data.stop_times.at(get_idx(tc)).arrival_time + min_corresp) % 86400) > get_time(*itc) % 86400) {
                    ++itc;
                }
                else {
                    break;
                }

            }
            if((itc != sa.second.end()) && (((data.stop_times.at(get_idx(tc)).arrival_time  + min_corresp)% 86400) <= (get_time(*itc) % 86400))) {
                if(!edge(get_ta_idx(get_idx(tc)), *itc, graph).second) {
                    add_edge(get_ta_idx(get_idx(tc)), *itc,
                             EdgeDesc(-1, calc_diff(get_time(get_ta_idx(get_idx(tc))), get_time(*itc)),
                                      is_passe_minuit(get_time(get_ta_idx(get_idx(tc))), get_time(*itc))), graph);
                }
            } else {
                if(!edge(get_ta_idx(get_idx(tc)), *itc, graph).second) {

                    add_edge(get_ta_idx(get_idx(tc)), *sa.second.begin(),
                             EdgeDesc(-1, calc_diff(get_time(get_ta_idx(get_idx(tc))), get_time(*sa.second.begin())),
                                      is_passe_minuit(get_time(get_ta_idx(get_idx(tc))), get_time(*sa.second.begin()))), graph);
                }
            }


        }
        if(sa.second.size() > 1) {
            if(!edge(*(--sa.second.end()), *sa.second.begin(), graph).second) {

                add_edge(*(--sa.second.end()), *sa.second.begin(), EdgeDesc(0, calc_diff(get_time(*(--sa.second.end())), get_time(*sa.second.begin())),true), graph);
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



    BOOST_ASSERT(boost::num_vertices(graph) == data.stop_areas.size() + data.stop_points.size() + data.route_points.size() + data.stop_times.size() * 3);

}

void TimeExpanded::build_heuristic(uint destination) {
    astar_graph.build_heuristic(destination);
}

idx_t TimeExpanded::get_idx(const vertex_t& v) const {
    if(v < stop_point_offset)
        return v;
    else if(v < route_point_offset)
        return v - stop_point_offset;
    else if(v < ta_offset)
        return v - route_point_offset;
    else if(v < td_offset)
        return v - ta_offset;
    else if(v < tc_offset)
        return v - td_offset;
    else {
        return v - tc_offset;
    }
}

uint32_t TimeExpanded::get_saidx(const vertex_t& v) const {
    if(v < stop_area_offset + data.stop_areas.size())
        return v;
    else if(v < stop_point_offset + data.stop_points.size())
        return data.stop_points.at(v - stop_point_offset).stop_area_idx;
    else if(v < route_point_offset + data.route_points.size())
        return data.stop_points.at(data.route_points.at(v - route_point_offset).stop_point_idx).stop_area_idx;
    else if(v < ta_offset + data.stop_times.size())
        return data.stop_points.at(data.route_points.at(data.stop_times.at(v - ta_offset).route_point_idx).stop_point_idx).stop_area_idx;
    else if(v < td_offset + data.stop_times.size())
        return data.stop_points.at(data.route_points.at(data.stop_times.at(v - td_offset).route_point_idx).stop_point_idx).stop_area_idx;
    else
        return data.stop_points.at(data.route_points.at(data.stop_times.at(v - tc_offset).route_point_idx).stop_point_idx).stop_area_idx;
}

node_type TimeExpanded::get_n_type(const vertex_t& v) const {
    if(v < stop_point_offset)
        return SA;
    else if(v < route_point_offset)
        return SP;
    else if(v < ta_offset)
        return RP;
    else if(v < td_offset)
        return TA;
    else if(v < tc_offset)
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

    BOOST_FOREACH(navitia::type::ValidityPattern vpi, validityPatterns) {
        unsigned int i=0;
        bool test = true;
        while((i<366) & test) {
            test = vpi.days[i] == vp.days[i];
            ++i;
        }

        if(test)
            return vpi.idx;
    }
    vp.idx = data.validity_patterns.size() + validityPatterns.size();

    validityPatterns.push_back(vp);


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


int32_t TimeExpanded::get_time(const vertex_t v) const {
    switch(get_n_type(v)) {
    case TA:
        return data.stop_times.at(get_idx(v)).arrival_time;
        break;
    case TD:
        return data.stop_times.at(get_idx(v)).departure_time;
        break;
    case TC:
        return data.stop_times.at(get_idx(v)).departure_time;
        break;
    default:
        return -1;
    }
}

bool TimeExpanded::is_passe_minuit(int32_t debut_t, int32_t fin_t) const {
    return ((debut_t % 86400) > (fin_t % 86400)) & (debut_t != -1) & (fin_t!=-1);
}

Path TimeExpanded::compute(idx_t departure_idx, idx_t destination_idx, int departure_hour, int departure_day) {
    Path result;

    departure_idx = trouver_premier_tc(departure_idx, departure_hour);

    DateTime etdebut;
    etdebut.date = departure_day;
    etdebut.hour = get_time(departure_idx);

    unsigned int arrival = destination_idx;

    try {
        boost::dijkstra_shortest_paths(this->graph, departure_idx,
                                       boost::predecessor_map(&predecessors[0])
                                       .weight_map(boost::get(boost::edge_bundle, graph)/*identity*/)
                                       .distance_map(&distances[0])
                                       .distance_combine(combine_simple(data, validityPatterns))
                                       .distance_zero(etdebut)
                                       .distance_inf(DateTime::infinity())
                                       .distance_compare(edge_less())
                                       .visitor(dijkstra_goal_visitor(destination_idx, *this))
                                       );

    } catch(found_goal fg) { arrival = fg.v; }

    int count = 1;
    BOOST_FOREACH(auto v, boost::vertices(this->graph)){
        if(predecessors[v] != v)
            count++;
    }

    result.percent_visited = 100 * count / boost::num_vertices(graph);


    result.duration = distances[arrival] - distances[departure_idx];

    while(predecessors[arrival] != arrival){
        if(arrival < tc_offset && arrival >= ta_offset){
            PathItem item(data.stop_areas.at(data.stop_points.at(data.route_points.at(data.stop_times.at(get_idx(arrival)).route_point_idx).stop_point_idx).stop_area_idx).name+"("+boost::lexical_cast<std::string>(get_saidx(arrival))+")",
                          distances[arrival].hour, distances[arrival].date, data.lines.at(data.routes.at(data.vehicle_journeys.at(data.stop_times.at(get_idx(arrival)).vehicle_journey_idx).route_idx).line_idx).name);
            result.items.push_back(item);
        }
        if(get_n_type(arrival) == TC && get_n_type(predecessors[arrival]) == TA)
            ++result.nb_changes;
        arrival = predecessors[arrival];
    }
    std::reverse(result.items.begin(), result.items.end());

    return result;
}


std::vector<routing::PathItem> TimeExpanded::compute_astar(idx_t departure_idx, idx_t destination_idx, int departure_hour, int departure_day){
    DateTime etdebut;
    etdebut.date = departure_day;
    etdebut.hour = departure_hour;

    std::cout << "Etiquette debut : " << etdebut.date << "  " << etdebut.hour << std::endl;

    vertex_t departure = departure_idx + stop_area_offset;
    vertex_t arrival = destination_idx + stop_area_offset;

    departure = trouver_premier_tc(departure_idx, departure_hour);



    try{
        boost::astar_search(this->graph, departure, distance_heuristic(*this),
                            boost::distance_map(&distances[0])
                            .predecessor_map(&predecessors[0])
                            .weight_map(boost::get(boost::edge_bundle, graph))
                            .distance_combine(combine_simple(data, validityPatterns))
                            .distance_zero(etdebut)
                            .distance_inf(DateTime::infinity())
                            .distance_compare(edge_less())
                            .visitor(astar_goal_visitor_te(arrival, *this))
                            .rank_map(&astar_dist[0])
                            );
    } catch(navitia::routing::timeexpanded::found_goal fg){  destination_idx = fg.v; }

    std::vector<routing::PathItem> result;

    int count = 1;
    BOOST_FOREACH(auto v, boost::vertices(this->graph)){
        if(predecessors[v] != v)
            count++;
    }

    //     while(preds[arrival] != arrival){
    //         if(arrival < data.route_points.size()){
    //             const type::StopPoint & sp = data.stop_points[data.route_points[arrival].stop_point_idx];
    //             PathItem item;
    //             item.stop_point_name = sp.name;
    //             item.day = distance[arrival].date;
    //             item.time = distance[arrival].hour;
    //             result.push_back(item);
    //         }
    //         arrival = preds[arrival];
    //     }
    //     std::reverse(result.begin(), result.end());
    return result;
}


DateTime combine_simple::operator ()(DateTime debut, EdgeDesc ed) const {
    if(debut == DateTime::infinity()) {
        return DateTime::infinity();
    }
    else {
        DateTime retour;
        if(ed.is_pam) {
            retour.date = debut.date + 1;
            retour.hour = (debut.hour +  ed.temps)%86400;
        }
        else {
            retour.date = debut.date;
            retour.hour = debut.hour + ed.temps;
        }

        if(ed.temps == 0 || ed.validity_pattern == -1 ) {
            return retour;
        } else {


            type::ValidityPattern vp;
            if(ed.validity_pattern < int(data.validity_patterns.size()))
                vp = data.validity_patterns.at(ed.validity_pattern);
            else
                vp = validityPatterns.at(ed.validity_pattern - data.validity_patterns.size());
            if(vp.check(retour.date)) {
                return retour;
            }
            else
                return DateTime::infinity();
        }

    }
}


int TimeExpanded::trouver_premier_tc(idx_t saidx, int depart) {

    boost::posix_time::ptime start, end;
    start = boost::posix_time::microsec_clock::local_time();
    idx_t stidx = data.stop_times.size();
    BOOST_FOREACH(idx_t spidx, data.stop_areas.at(saidx).stop_point_list) {
        BOOST_FOREACH(idx_t rpidx, data.stop_points.at(spidx).route_point_list) {
            BOOST_FOREACH(idx_t vjidx, data.route_points.at(rpidx).vehicle_journey_list) {

                if(data.stop_times.at(data.vehicle_journeys.at(vjidx).stop_time_list.at(data.route_points.at(rpidx).order)).departure_time >= depart) {
                    if(stidx == data.stop_times.size()) {
                        stidx = data.vehicle_journeys.at(vjidx).stop_time_list.at(data.route_points.at(rpidx).order);
                    } else {
                        if(data.stop_times.at(data.vehicle_journeys.at(vjidx).stop_time_list.at(data.route_points.at(rpidx).order)).departure_time <
                                data.stop_times.at(stidx).departure_time) {
                            stidx = data.vehicle_journeys.at(vjidx).stop_time_list.at(data.route_points.at(rpidx).order);
                        }
                    }
                }
            }
        }
    }
    end = boost::posix_time::microsec_clock::local_time();
    if(stidx == data.stop_times.size()) {
        stidx = data.vehicle_journeys.at(data.route_points.at(data.stop_points.at(data.stop_areas.at(saidx).stop_point_list.front()).route_point_list.front()).vehicle_journey_list.front()).stop_time_list.at(data.route_points.at(data.stop_points.at(data.stop_areas.at(saidx).stop_point_list.front()).route_point_list.front()).order);
    }

    if(stidx != data.stop_times.size()) {
        return get_tc_idx(stidx);
    }  else {
        return -1;
    }

}


}}}
