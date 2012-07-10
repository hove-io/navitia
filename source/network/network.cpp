#include "network.h"


namespace network {

unsigned int get_sp_idx(unsigned int spid, navitia::type::Data &data) {
    return data.pt_data.stop_areas.size() +spid;
}

unsigned int get_rp_idx(unsigned int rpid, navitia::type::Data &data) {
    return data.pt_data.stop_areas.size() + data.pt_data.stop_points.size() + rpid;
}

unsigned int get_ta_idx(unsigned int stid, navitia::type::Data &data) {
    return data.pt_data.stop_areas.size() + data.pt_data.stop_points.size() + data.pt_data.route_points.size() + stid;
}

unsigned int get_td_idx(unsigned int stid, navitia::type::Data &data) {
    return data.pt_data.stop_areas.size() + data.pt_data.stop_points.size() + data.pt_data.route_points.size() + data.pt_data.stop_times.size() +  stid;
}


idx_t get_idx(unsigned int idx, navitia::type::Data &data,map_tc_t &map_tc) {
    if(idx < data.pt_data.stop_areas.size())
        return idx;
    else if(idx < (data.pt_data.stop_areas.size() + data.pt_data.stop_points.size()))
        return (idx - data.pt_data.stop_areas.size());
    else if(idx < (data.pt_data.stop_areas.size() + data.pt_data.stop_points.size() + data.pt_data.route_points.size()))
        return (idx - data.pt_data.stop_areas.size() - data.pt_data.stop_points.size());
    else if(idx < (data.pt_data.stop_areas.size() + data.pt_data.stop_points.size() + data.pt_data.route_points.size() + data.pt_data.stop_times.size()))
        return (idx - data.pt_data.stop_areas.size() - data.pt_data.stop_points.size() - data.pt_data.route_points.size());
    else if(idx < (data.pt_data.stop_areas.size() + data.pt_data.stop_points.size() + data.pt_data.route_points.size() + data.pt_data.stop_times.size()*2))
        return (idx - data.pt_data.stop_areas.size() - data.pt_data.stop_points.size() - data.pt_data.route_points.size() - data.pt_data.stop_times.size());
    else {
        return get_idx(map_tc[idx], data, map_tc);
    }
}

uint32_t get_saidx(unsigned int idx, navitia::type::Data &data, NW &g, map_tc_t &map_tc) {
    if(idx < data.pt_data.stop_areas.size())
        return idx;
    else if(idx < (data.pt_data.stop_areas.size() + data.pt_data.stop_points.size()))
        return data.pt_data.stop_points.at(idx - data.pt_data.stop_areas.size()).stop_area_idx;
    else if(idx < (data.pt_data.stop_areas.size() + data.pt_data.stop_points.size() + data.pt_data.route_points.size()))
        return data.pt_data.stop_points.at(data.pt_data.route_points.at(idx - data.pt_data.stop_areas.size() - data.pt_data.stop_points.size()).stop_point_idx).stop_area_idx;
    else if(idx < (data.pt_data.stop_areas.size() + data.pt_data.stop_points.size() + data.pt_data.route_points.size() + data.pt_data.stop_times.size()))
        return data.pt_data.stop_points.at(data.pt_data.route_points.at(data.pt_data.stop_times.at((idx - data.pt_data.stop_areas.size() - data.pt_data.stop_points.size() - data.pt_data.route_points.size())).route_point_idx).stop_point_idx).stop_area_idx;
    else if(idx < (data.pt_data.stop_areas.size() + data.pt_data.stop_points.size() + data.pt_data.route_points.size() + (data.pt_data.stop_times.size()) * 2))
        return data.pt_data.stop_points.at(data.pt_data.route_points.at(data.pt_data.stop_times.at((idx - data.pt_data.stop_areas.size() - data.pt_data.stop_points.size() - data.pt_data.route_points.size() - data.pt_data.stop_times.size())).route_point_idx).stop_point_idx).stop_area_idx;
    else
        return get_saidx(map_tc[idx], data, g, map_tc);
}

node_type get_n_type(unsigned int idx, navitia::type::Data &data) {
    if(idx < data.pt_data.stop_areas.size())
        return SA;
    else if(idx < (data.pt_data.stop_areas.size() + data.pt_data.stop_points.size()))
        return SP;
    else if(idx < (data.pt_data.stop_areas.size() + data.pt_data.stop_points.size() + data.pt_data.route_points.size()))
        return RP;
    else if(idx < (data.pt_data.stop_areas.size() + data.pt_data.stop_points.size() + data.pt_data.route_points.size() + data.pt_data.stop_times.size()))
        return TA;
    else if(idx < (data.pt_data.stop_areas.size() + data.pt_data.stop_points.size() + data.pt_data.route_points.size() + (data.pt_data.stop_times.size()) * 2))
        return TD;
    else
        return TC;
}

bool is_passe_minuit(uint32_t debut, uint32_t fin, navitia::type::Data &data, NW &g, map_tc_t &map_tc) {
    uint32_t debut_t = get_time(debut, data, g, map_tc), fin_t = get_time(fin, data, g, map_tc);
    return is_passe_minuit(debut_t, fin_t);
}

bool is_passe_minuit(int32_t debut_t, int32_t fin_t) {
    return ((debut_t % 86400) > (fin_t % 86400)) & (debut_t != -1) & (fin_t!=-1);
}

int32_t get_time(unsigned int idx, navitia::type::Data &data, NW &g, map_tc_t &map_tc) {
    switch(get_n_type(idx, data)) {
    case TA:
        return data.pt_data.stop_times.at(get_idx(idx, data, map_tc)).arrival_time;
        break;
    case TD:
        return data.pt_data.stop_times.at(get_idx(idx, data, map_tc)).departure_time;
        break;
    case TC:
        return get_time(map_tc.at(idx), data, g, map_tc);
        break;
    default:
        return -1;
    }
}

uint16_t calc_diff(uint16_t debut, uint16_t fin) {
  int diff = fin - debut;
  return (diff>0) ? diff : 86400 + diff;
}

int get_validity_pattern_idx(navitia::type::ValidityPattern vp, navitia::type::Data &data) {
    BOOST_FOREACH(navitia::type::ValidityPattern vpi, data.pt_data.validity_patterns) {

        unsigned int i=0;
        bool test = true;
        while((i<366) & test) {
            test = vpi.days[i] == vp.days[i];
            ++i;
        }

        if(test)
            return vpi.idx;
    }
    vp.idx = data.pt_data.validity_patterns.size();

    data.pt_data.validity_patterns.push_back(vp);


    return vp.idx;
}

navitia::type::ValidityPattern * decalage_pam(navitia::type::ValidityPattern &vp) {
    navitia::type::ValidityPattern *vpr = new navitia::type::ValidityPattern(vp.beginning_date);

    for(unsigned int i=0; i < 366; i++) {
        vpr->days[i+1] = vp.days[i];
    }
    vpr->days[0] = vp.days[365];

    return vpr;
}

void calculer_AR(navitia::type::Data &data, NW &g, map_routes_t &map_routes) {
    map_routes_t::iterator i_toadd;

    std::vector<navitia::type::Route>::iterator i_route1, i_route2;

    unsigned int nb_trouves = 0;

    for(i_route1 = data.pt_data.routes.begin(); i_route1 != data.pt_data.routes.end(); ++i_route1) {

        i_route2 = i_route1;

        for(++i_route2; i_route2 != data.pt_data.routes.end(); ++i_route2) {
            navitia::type::Route route1 = *i_route1, route2 = *i_route2;

            if(route1.route_point_list.size() == route2.route_point_list.size()) {
                bool test = true;
                for(unsigned int i=0; (i < route1.route_point_list.size()) & test; ++i) {
                    test = test &
                            (data.pt_data.stop_points.at(data.pt_data.route_points.at(route1.route_point_list[i]).stop_point_idx).stop_area_idx ==
                             data.pt_data.stop_points.at(data.pt_data.route_points.at(route2.route_point_list[route1.route_point_list.size() - i - 1]).stop_point_idx).stop_area_idx
                             );
                }

                if(test) {
                    nb_trouves ++;
                    i_toadd = map_routes.find(route1.idx);
                    i_toadd->second.push_back(route2.idx);


                    i_toadd = map_routes.find(route2.idx);

                    i_toadd->second.push_back(route1.idx);
                }
            }
        }
    }
    std::cout << "On a trouve " << nb_trouves << " A/R" << std::endl;
}


bool correspondance_valide(idx_t tav, idx_t tdv, bool pam, NW &g, navitia::type::Data &data, map_routes_t & map_routes, map_tc_t map_tc) {
    bool retour;
    int min_corresp = 5*60, max_corresp = 2*3600; //Temps minimal de correspondance en secondes
    retour = (data.pt_data.stop_times.at(get_idx(tdv, data, map_tc)).departure_time - data.pt_data.stop_times.at(get_idx(tdv, data, map_tc)).arrival_time + (pam*86400)) > min_corresp;

    retour = retour & ((data.pt_data.stop_times.at(get_idx(tdv, data, map_tc)).departure_time - data.pt_data.stop_times.at(get_idx(tdv, data, map_tc)).arrival_time + (pam*86400)) < max_corresp) ;

    list_idx l_route = map_routes[data.pt_data.route_points.at(data.pt_data.stop_times.at(get_idx(tdv, data, map_tc)).route_point_idx).route_idx];
    retour = retour & (std::find(l_route.begin(), l_route.end(), data.pt_data.route_points.at(data.pt_data.stop_times.at(get_idx(tdv, data, map_tc)).route_point_idx).route_idx) == l_route.end());

    return retour;
}


void charger_graph(navitia::type::Data &data, NW &g, map_tc_t &map_tc, map_tc_t &map_td) {
    vertex_t tav, tdv, tcv, rpv;
    bool bo;
    int min_corresp = 2 * 60;
    navitia::type::ValidityPattern vptemp;

    typedef std::set<vertex_t, sort_edges> set_vertices;
    typedef std::map<vertex_t, set_vertices> map_t;

    map_t map_stop_areas;

    //Creation des stop areas
        BOOST_FOREACH(navitia::type::StopArea sai, data.pt_data.stop_areas) {
    //        g[sai.idx] = VertexDesc(sai.idx);
            map_stop_areas.insert(std::pair<vertex_t, set_vertices>(sai.idx, set_vertices(sort_edges(g, data, map_tc))));
        }
    //Creation des stop points et creations lien SA->SP
    BOOST_FOREACH(navitia::type::StopPoint spi, data.pt_data.stop_points) {
        add_edge(spi.stop_area_idx, get_sp_idx(spi.idx, data), EdgeDesc(0), g);

    }

    //Création des liens SP->RP
    BOOST_FOREACH(navitia::type::RoutePoint rpi, data.pt_data.route_points) {
        add_edge(get_sp_idx(rpi.stop_point_idx, data), get_rp_idx(rpi.idx, data), EdgeDesc(0), g);
    }


    BOOST_FOREACH(navitia::type::VehicleJourney vj,  data.pt_data.vehicle_journeys) {
        tdv = 0;
        unsigned int vp_idx = vj.validity_pattern_idx;
        BOOST_FOREACH(unsigned int stid, vj.stop_time_list) {
            rpv = get_rp_idx(data.pt_data.stop_times.at(stid).route_point_idx, data);


            //ta
            tav = get_ta_idx(stid, data) ;
            if(tdv != 0) {
                if(is_passe_minuit(tdv, tav, data, g, map_tc)) {
                    vptemp = *(decalage_pam(data.pt_data.validity_patterns.at(vj.validity_pattern_idx)));
                    vp_idx = get_validity_pattern_idx(vptemp, data);
                }
                add_edge(tdv, tav, EdgeDesc(vp_idx, calc_diff(get_time(tdv, data, g, map_tc), get_time(tav, data, g, map_tc)), is_passe_minuit(tdv, tav, data, g, map_tc)), g);
            }

            //td
            tdv = get_td_idx(stid, data);

            //tc
            tcv = add_vertex(g);
            map_tc.insert(std::pair<idx_t, idx_t>(tcv, tdv));
            map_stop_areas.find(data.pt_data.stop_points.at(data.pt_data.route_points.at(get_idx(rpv, data, map_tc)).stop_point_idx).stop_area_idx)->second.insert(tcv);
            //edges


            if(is_passe_minuit(tav, tdv, data, g, map_tc)) {
                vptemp = *decalage_pam(data.pt_data.validity_patterns.at(vj.validity_pattern_idx));
                vp_idx = get_validity_pattern_idx(vptemp, data);
            }
            add_edge(tav, tdv, EdgeDesc(vp_idx, calc_diff(get_time(tav, data, g, map_tc), get_time(tdv, data, g, map_tc)),is_passe_minuit(tav, tdv, data, g, map_tc)), g);
            add_edge(tcv, tdv, EdgeDesc(0), g);
        }
    }

    BOOST_FOREACH(map_t::value_type sa, map_stop_areas) {
        //On fabrique la chaine de tc
        vertex_t prec = num_vertices(g) + 1;
        set_vertices::iterator itc, pam = sa.second.begin();
        itc = sa.second.begin();
        pam = sa.second.begin();
        BOOST_FOREACH(vertex_t tc, sa.second) {
            if((prec != (num_vertices(g) + 1)) && !edge(prec, tc, g).second && (prec != tc)) {
                add_edge(prec, tc, EdgeDesc(0, data.pt_data.stop_times.at(get_idx(tc, data, map_tc)).departure_time -  data.pt_data.stop_times.at(get_idx(prec, data, map_tc)).departure_time, false), g);
            }
            prec = tc;
            //Je relie le ta au premier tc possible
            while((itc != sa.second.end()) & ((data.pt_data.stop_times.at(get_idx(tc, data, map_tc)).arrival_time + min_corresp % 86400) > get_time(*itc, data, g, map_tc) % 86400) )
                ++itc;
            if((itc != sa.second.end()) && ((data.pt_data.stop_times.at(get_idx(tc, data, map_tc)).arrival_time  + min_corresp% 86400) <= (get_time(*itc, data, g, map_tc) % 86400))) {
                if(!edge(get_ta_idx(get_idx(tc, data, map_tc), data), *itc, g).second)
                    add_edge(get_ta_idx(get_idx(tc, data, map_tc), data), *itc,
                             EdgeDesc(-1, (data.pt_data.stop_times.at(get_idx(tc, data, map_tc)).arrival_time %86400) - data.pt_data.stop_times.at(get_idx(tc, data, map_tc)).departure_time,  is_passe_minuit(get_ta_idx(get_idx(tc, data, map_tc), data), *itc, data, g, map_tc)), g);
            } else {
                if(!edge(get_ta_idx(get_idx(tc, data, map_tc), data), *itc, g).second)
                    add_edge(get_ta_idx(get_idx(tc, data, map_tc), data), *sa.second.begin(),
                             EdgeDesc(-1, (86400 - data.pt_data.stop_times.at(get_idx(tc, data, map_tc)).departure_time % 86400) + data.pt_data.stop_times.at(get_idx(tc, data, map_tc)).arrival_time %86400, is_passe_minuit(get_ta_idx(get_idx(tc, data, map_tc), data),  *sa.second.begin(), data, g, map_tc)), g);
            }

//            //Si > 86400 je relie au premier tc possible avec le temps % 86400
//            if((data.pt_data.stop_times.at(get_idx(tc, data, map_tc)).arrival_time + min_corresp) > 86400) {
//                while((pam != sa.second.end()) & ((data.pt_data.stop_times.at(get_idx(tc, data, map_tc)).arrival_time%86400 + min_corresp )> get_time(*pam, data, g, map_tc)) )
//                    ++pam;
//                if(get_ta_idx(get_idx(tc, data, map_tc), data) == 33)
//                    std::cout << "T1 : " <<(pam != sa.second.end()) << " "  << (data.pt_data.stop_times.at(get_idx(tc, data, map_tc)).arrival_time%86400) + min_corresp <<" " << get_time(*pam, data, g, map_tc) << std::endl  ;
//                if(pam != sa.second.end())
//                    if((pam != sa.second.end()) & ((data.pt_data.stop_times.at(get_idx(tc, data, map_tc)).arrival_time%86400) + min_corresp <= get_time(*pam, data, g, map_tc)))
//                        add_edge(get_ta_idx(get_idx(tc, data, map_tc), data), *pam, EdgeDesc(0), g);
//            }
        }
        if(sa.second.size() > 1) {
            if(!edge(*(--sa.second.end()), *sa.second.begin(), g).second) {
                add_edge(*(--sa.second.end()), *sa.second.begin(), EdgeDesc(0, true), g);
            }
        }


        //On lie les RP a premier TC de la chaine
        BOOST_FOREACH(edge_t e1, out_edges(sa.first, g)) {
            if(get_n_type(target(e1, g), data) == SP) {
                BOOST_FOREACH(edge_t e2, out_edges(target(e1, g), g)) {
                    if(get_n_type(target(e2, g), data) == RP) {
                        add_edge(target(e2, g), *sa.second.begin(), EdgeDesc(0), g);
                    }
                }
            }
        }

    }


    BOOST_FOREACH(auto pairtc, map_tc) {
        map_td.insert(std::pair<unsigned int, unsigned int>(pairtc.second, pairtc.first));
    }



}


bool est_transport(edge_t e, navitia::type::Data &data, NW & g) {
    return (get_n_type(source(e, g), data) == TA && get_n_type(target(e, g), data) == TD) || (get_n_type(source(e, g), data) == TD || get_n_type(target(e, g), data) == TA);
}




bool etape::operator ==(network::etape e2) {
    return (this->ligne == e2.ligne) & (this->descente == e2.descente);
}

bool etape::operator!=(network::etape e2) {
    return !((*this) == e2);
}

void parcours::ajouter_etape(idx_t ligne, idx_t depart) {
    this->etapes.push_front(network::etape(ligne, depart));
}

bool parcours::operator==(parcours i2) {
    if(this->etapes.size() != i2.etapes.size())
        return false;
    std::list<network::etape>::iterator it1, it2;
    for(it1 = this->etapes.begin(), it2 = i2.etapes.begin(); it1 != this->etapes.end(); ++it1, ++it2) {
        if((*it1) != (*it2))
            return false;
    }
    return true;
}

bool parcours::operator!=(network::parcours i2) {
    return !((*this) == i2);
}

etiquette combine::operator()(etiquette debut, edge_t e) const{
    if(debut == etiquette::max()) {
        return etiquette::max();
    }
    else {
        etiquette retour;
        retour.temps = 0;
        retour.correspondances = 0;
        int32_t debut_temps = 0, fin_temps = 0;
        EdgeDesc ed = g[e];
        idx_t debut_idx = source(e, g), fin_idx = target(e, g);

        debut_temps = get_time(debut_idx, data, g, map_tc);
        fin_temps = get_time(fin_idx, data, g, map_tc);

        if((get_saidx(debut_idx, data, g, map_tc) == sa_depart) & (get_saidx(fin_idx, data, g, map_tc) == sa_depart)) {
            if(is_passe_minuit(debut_temps, fin_temps)) {
                retour.date_arrivee = debut.date_arrivee + 1;
            } else {
                retour.date_arrivee = debut.date_arrivee;
            }

            if((get_n_type(debut_idx, data) == TC) & (get_n_type(fin_idx, data) == TD)){
                if((debut.heure_arrivee % 86400) <= debut_temps) {
                    retour.heure_arrivee = fin_temps;
                    retour.temps = 0;
                    retour.correspondances = 0;
                    return retour;
                } else {
                    return etiquette::max();
                }
            } else {
                retour.heure_arrivee = debut.heure_arrivee;
                retour.temps = 0;
                retour.correspondances = 0;
                return retour;
            }
        } else {

            if(is_passe_minuit(debut_temps, fin_temps)) { //Passe minuit
                retour.date_arrivee = debut.date_arrivee + 1;
                if(est_transport(e, data, g))
                    retour.temps = debut.temps + 86400 - debut_temps%86400 + fin_temps%86400 ;
                else
                    retour.temps = debut.temps;
            } else {
                retour.date_arrivee = debut.date_arrivee;
                if(est_transport(e, data, g))
                    retour.temps = debut.temps + fin_temps%86400 - debut_temps%86400;
                else
                    retour.temps = debut.temps;
            }

            if((get_n_type(debut_idx, data) == TC) & (get_n_type(debut_idx, data) == TD) )
                retour.correspondances = debut.correspondances + 1;
            else
                retour.correspondances = debut.correspondances;

            if(data.pt_data.validity_patterns.at(ed.validity_pattern).check(retour.date_arrivee) || !est_transport(e, data, g)) {
                retour.heure_arrivee = fin_temps;
                return retour;
            } else {
                return etiquette::max();
            }
        }


    }
}

int count_combine = 0;

etiquette combine_simple::operator ()(etiquette debut, EdgeDesc ed) const {
    ++ count_combine;
//    std::cout << "Je suis dans le combine " << count_combine << std::endl;
    if(debut == etiquette::max()) {
        return etiquette::max();
    }
    else {
        etiquette retour;
//        EdgeDesc ed = g[e];

        if(!ed.is_pam)
            retour.date_arrivee = debut.date_arrivee;
        else
            retour.date_arrivee = debut.date_arrivee + 1;

        if(ed.validity_pattern == -1 || ed.temps == 0) {
            retour.heure_arrivee = debut.heure_arrivee;
            return retour;
        } else {
            if(data.pt_data.validity_patterns.at(ed.validity_pattern).check(retour.date_arrivee)) {
                retour.heure_arrivee  = debut.heure_arrivee + ed.temps;
                return retour;
            }
            else
                return etiquette::max();
        }

    }
}
idx_t trouver_premier_tc(idx_t saidx, int depart, navitia::type::Data & data, map_tc_t &map_td) {

    boost::posix_time::ptime start, end;
    start = boost::posix_time::microsec_clock::local_time();
    idx_t stidx = data.pt_data.stop_times.size();
    BOOST_FOREACH(idx_t spidx, data.pt_data.stop_areas.at(saidx).stop_point_list) {
        BOOST_FOREACH(idx_t rpidx, data.pt_data.stop_points.at(spidx).route_point_list) {
            BOOST_FOREACH(idx_t vjidx, data.pt_data.route_points.at(rpidx).vehicle_journey_list) {

                if(data.pt_data.stop_times.at(data.pt_data.vehicle_journeys.at(vjidx).stop_time_list.at(data.pt_data.route_points.at(rpidx).order)).departure_time >= depart) {
                    if(stidx == data.pt_data.stop_times.size()) {
                        stidx = data.pt_data.vehicle_journeys.at(vjidx).stop_time_list.at(data.pt_data.route_points.at(rpidx).order);
                    } else {
                        if(data.pt_data.stop_times.at(data.pt_data.vehicle_journeys.at(vjidx).stop_time_list.at(data.pt_data.route_points.at(rpidx).order)).departure_time <
                           data.pt_data.stop_times.at(stidx).departure_time) {
                            stidx = data.pt_data.vehicle_journeys.at(vjidx).stop_time_list.at(data.pt_data.route_points.at(rpidx).order);
                        }
                    }
                }
            }
        }
    }
    end = boost::posix_time::microsec_clock::local_time();
    std::cout << "t1 : " << (end-start).    total_milliseconds() << std::endl;
    if(stidx == data.pt_data.stop_times.size()) {
        stidx = data.pt_data.vehicle_journeys.at(data.pt_data.route_points.at(data.pt_data.stop_points.at(data.pt_data.stop_areas.at(saidx).stop_point_list.front()).route_point_list.front()).vehicle_journey_list.front()).stop_time_list.at(data.pt_data.route_points.at(data.pt_data.stop_points.at(data.pt_data.stop_areas.at(saidx).stop_point_list.front()).route_point_list.front()).order);
    }

    if(stidx != data.pt_data.stop_times.size()) {
        auto iter = map_td.find(get_td_idx(stidx,data));
        if(iter!= map_td.end())
            return (*iter).second;
        else
            return -1;
    }  else {
        return 1;
    }

}









}
