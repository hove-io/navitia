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
        return get_saidx(get_idx(idx, data, map_tc), data, g, map_tc);
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
        return get_time(map_tc[idx], data, g, map_tc) + (5*60)*(get_n_type(map_tc[idx], data)==TA);
        break;
    default:
        return -1;
    }
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


void charger_graph(navitia::type::Data &data, NW &g, map_tc_t &map_tc) {
    vertex_t tav, tdv, tdp, rpv;
    bool bo;
    int min_corresp = 5 * 60;
    navitia::type::ValidityPattern vptemp;

    //Creation des stop areas
    //    BOOST_FOREACH(navitia::type::StopArea sai, data.pt_data.stop_areas) {
    //        g[sai.idx] = VertexDesc(sai.idx);
    //    }
    //Creation des stop points et creations lien SA->SP
    BOOST_FOREACH(navitia::type::StopPoint spi, data.pt_data.stop_points) {
        //        g[get_sp_idx(spi.idx, data)] = VertexDesc(spi.idx);
        add_edge(spi.stop_area_idx, get_sp_idx(spi.idx, data), EdgeDesc(0), g);
    }

    //Création des liens SP->RP
    BOOST_FOREACH(navitia::type::RoutePoint rpi, data.pt_data.route_points) {
        //        g[get_rp_idx(rpi.idx, data)] = VertexDesc(rpi.idx);
        add_edge(get_sp_idx(rpi.stop_point_idx, data), get_rp_idx(rpi.idx, data), EdgeDesc(0), g);
    }


    BOOST_FOREACH(navitia::type::VehicleJourney vj,  data.pt_data.vehicle_journeys) {
        tdv = 0;
        unsigned int vp_idx = vj.validity_pattern_idx;
        BOOST_FOREACH(unsigned int stid, vj.stop_time_list) {
            rpv = get_rp_idx(data.pt_data.stop_times.at(stid).route_point_idx, data);

            //Création des ta et td;
            //            g[get_ta_idx(stid, data)] = VertexDesc(stid);
            //            g[get_td_idx(stid, data)] = VertexDesc(stid);

            //ta
            tav = get_ta_idx(stid, data) ;
            if(tdv != 0) {
                if(is_passe_minuit(tdv, tav, data, g, map_tc)) {
                    vptemp = *(decalage_pam(data.pt_data.validity_patterns.at(vj.validity_pattern_idx)));
                    vp_idx = get_validity_pattern_idx(vptemp, data);
                }
                add_edge(tdv, tav, EdgeDesc(vp_idx), g);
            }

            //td
            tdv = get_td_idx(stid, data);
            //edges


            if(is_passe_minuit(tav, tdv, data, g, map_tc)) {
                vptemp = *decalage_pam(data.pt_data.validity_patterns.at(vj.validity_pattern_idx));
                vp_idx = get_validity_pattern_idx(vptemp, data);
            }
            add_edge(tav, tdv, EdgeDesc(vp_idx), g);

            add_edge(rpv, tav, EdgeDesc(0), g);
        }
    }

    typedef std::map<vertex_t, vertex_t> map_t;
    typedef std::map<vertex_t, bool> mapam_t;
    typedef std::pair<vertex_t, int32_t> pairTC_t;
    std::set<vertex_t, sort_edges> verticesTD = std::set<vertex_t, sort_edges>(sort_edges(g, data, map_tc));
    std::set<vertex_t, sort_edges> verticesTA = std::set<vertex_t, sort_edges>(sort_edges(g, data, map_tc));

    std::vector<pairTC_t> verticesTC = std::vector<pairTC_t>();

    //    map_routes_t map_AR = map_routes_t();
    //    for(unsigned int i=0; i<data.pt_data.routes.size();++i)
    //        map_AR.insert(std::pair<idx_t, list_idx>(i, list_idx()));

    //    calculer_AR(data, g, map_AR);

    //    std::cout <<"Taille map ar" << map_AR.size();
    for(unsigned int sav = 0; sav<data.pt_data.stop_areas.size();++sav) {
        //On itère sur les stop points
        verticesTA.clear();
        verticesTD.clear();
        verticesTC.clear();
        BOOST_FOREACH(edge_t e1, out_edges(sav, g)) {
            if(get_n_type(target(e1, g), data)== SP) {
                BOOST_FOREACH(edge_t e2, out_edges(target(e1, g), g)) {
                    if(get_n_type(target(e2, g), data) == RP) {
                        //On itère sur les ta
                        BOOST_FOREACH(edge_t e3, out_edges(target(e2, g), g)) {
                            if(get_n_type(target(e3, g), data) == TA) {
                                verticesTA.insert(target(e3, g));
                                //On itère sur les td
                                BOOST_FOREACH(edge_t e4, out_edges(target(e3, g), g)) {
                                    if(get_n_type(target(e4, g), data) == TD) {
                                        if(data.pt_data.stop_times.at(get_idx(target(e4, g), data, map_tc)).route_point_idx == data.pt_data.stop_times.at(get_idx(target(e3, g), data, map_tc)).route_point_idx) {
                                            verticesTD.insert(target(e4, g));
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        if(verticesTA.size() > 0) {
            std::vector<pairTC_t>::iterator itTC = verticesTC.begin();
            BOOST_FOREACH(vertex_t ta, verticesTA) {

                verticesTC.push_back(pairTC_t(add_vertex(g), data.pt_data.stop_times.at(get_idx(ta, data, map_tc)).arrival_time + min_corresp));
                add_edge(ta, verticesTC.back().first, EdgeDesc(0), g);
                map_tc.insert(std::pair<idx_t, idx_t>(verticesTC.back().first, ta));
            }

            itTC = verticesTC.begin();
            BOOST_FOREACH(vertex_t td, verticesTD) {
                while(((*itTC).second < data.pt_data.stop_times.at(get_idx(td, data, map_tc)).departure_time) & (itTC != verticesTC.end()))
                    ++itTC;
                if((*itTC).second != data.pt_data.stop_times.at(get_idx(td, data, map_tc)).departure_time) {
                    itTC = verticesTC.insert(itTC, pairTC_t(add_vertex(g), data.pt_data.stop_times.at(get_idx(td, data, map_tc)).departure_time));
                    map_tc.insert(std::pair<idx_t, idx_t>((*itTC).first, td));
                }
                add_edge((*itTC).first, td, EdgeDesc(0), g);

            }


            //On lie les TC entre eux
            itTC = verticesTC.begin();
            pairTC_t prec = pairTC_t(0, -1);


            BOOST_FOREACH(pairTC_t tc, verticesTC) {
                if((prec.second != -1) & !edge(prec.first, tc.first, g).second & (prec.first != tc.first)) {
                    add_edge(prec.first, tc.first, EdgeDesc(0), g);
                }
                prec = tc;
            }

            std::vector<pairTC_t>::iterator precTC= verticesTC.begin(), pamTC= verticesTC.begin();
            for(itTC = (++precTC); itTC!=verticesTC.end();++itTC) {
                //On rajoute le passe minuit
                if((*itTC).second > 86400) {
                    while((pamTC != verticesTC.end()) & ((*pamTC).second <= ((*itTC).second % 86400)))
                        ++pamTC;
                    if((*pamTC).second > ((*itTC).second % 86400)) {
                        if(!edge((*precTC).first, (*pamTC).first, g).second & ((*precTC).first != (*pamTC).first))
                            add_edge((*precTC).first, (*pamTC).first, EdgeDesc(0), g);
                    }
                }
                precTC = itTC;
            }
            if((*precTC).second > 86400) {
                while((pamTC != verticesTC.end()) & ((*pamTC).second <= ((*precTC).second % 86400)))
                    ++pamTC;
                if((*pamTC).second > ((*precTC).second % 86400)) {
                    if(!edge((*precTC).first, (*pamTC).first, g).second & ((*precTC).first != (*pamTC).first))
                        add_edge((*precTC).first, (*pamTC).first, EdgeDesc(0), g);
                }
            }

        }



    }


}


bool est_transport(edge_t e, navitia::type::Data &data, NW & g) {
    return (get_n_type(source(e, g), data) == TA || get_n_type(source(e, g), data) == TD) & (get_n_type(target(e, g), data) == TA || get_n_type(target(e, g), data) == TD);
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
        int32_t debut_temps = 0, fin_temps = 0;
        EdgeDesc ed = g[e];
        idx_t debut_idx = source(e, g), fin_idx = target(e, g);

        debut_temps = get_time(debut_idx, data, g, map_tc);
        fin_temps = get_time(fin_idx, data, g, map_tc);


        if(get_saidx(fin_idx, data, g, map_tc) == sa_depart) {
            if(get_n_type(debut_idx, data) == TC) {
                return etiquette::max();
            } else {
                if(get_n_type(debut_idx, data) == SA || get_n_type(debut_idx, data) == SP || get_n_type(debut_idx, data) == RP){
                    retour.date_arrivee = debut.date_arrivee;
                    retour.heure_arrivee = debut.heure_arrivee;
                    retour.temps = 0;
                    retour.correspondances = 0;
                    return retour;
                } else {
                    if(is_passe_minuit(debut_temps, fin_temps)) {
                        retour.date_arrivee = debut.date_arrivee + 1;
                    } else {
                        retour.date_arrivee = debut.date_arrivee;
                    }

                    if(data.pt_data.validity_patterns.at(ed.validity_pattern).check(retour.date_arrivee)
                            & ((debut.heure_arrivee % 86400) <= debut_temps)) {
                        retour.heure_arrivee = fin_temps;
                        retour.temps = 0;
                        retour.correspondances = 0;
                        return retour;
                    } else {
                        return etiquette::max();
                    }
                }
            }
        } else {

            //On verifie le validity pattern
            if(is_passe_minuit(debut_temps, fin_temps)) { //Passe minuit
                retour.date_arrivee = debut.date_arrivee + 1;
                retour.temps = debut.temps + 86400 - debut_temps%86400 + fin_temps%86400 ;
            } else {
                retour.date_arrivee = debut.date_arrivee;
                retour.temps = debut.temps + fin_temps%86400 - debut_temps%86400;
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









}
