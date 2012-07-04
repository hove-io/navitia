#include "itineraires.h"

namespace itineraires {

bool etape::operator==(etape const & e2) const {
    return (this->mode == e2.mode) & (this->descente == e2.descente);
}

bool etape::operator!=(etape const & e2) const {
    return !((*this) == e2);
}

void etape::ajouter_ligne(idx_t &ligne) {
    if(lignes.count(ligne) == 0)
        lignes.insert(lignes.begin(), ligne);
}

void parcours::ajouter_etape(idx_t mode, idx_t descente, idx_t ligne) {
    vector_etapes::iterator iter = std::find(this->etapes.begin(), this->etapes.end(), etape(mode, descente, ligne));
    if(iter != etapes.end())
        (*iter).ajouter_ligne(ligne);
    else
        this->etapes.insert(this->etapes.begin(), etape(mode, descente, ligne));
}

void parcours::ajouter_ligne(idx_t mode, idx_t descente, idx_t ligne) {

    vector_etapes::iterator iter = std::find(this->etapes.begin(), this->etapes.end(), etape(mode, descente, ligne));
    if(iter != etapes.end())
        (*iter).ajouter_ligne(ligne);
    else
        this->ajouter_etape(mode, descente, ligne);
}

bool parcours::operator==(parcours i2) {
    if(this->etapes.size() != i2.etapes.size())
        return false;
    vector_etapes::iterator it1, it2;
    for(it1 = this->etapes.begin(), it2 = i2.etapes.begin(); it1 != this->etapes.end(); ++it1, ++it2) {
        if((*it1) != (*it2))
            return false;
    }
    return true;
}

bool parcours::operator!=(parcours i2) {
    return !((*this) == i2);
}



void make_itineraires(network::vertex_t v1, network::vertex_t v2, std::vector<itineraire> &itineraires, map_parcours &parcours_list, network::NW &g, navitia::type::Data &data, std::vector<network::vertex_t> &predecessors, network::map_tc_t &map_tc){

    BOOST_FOREACH(unsigned int sp, data.pt_data.stop_areas.at(v2).stop_point_list) {
        BOOST_FOREACH(unsigned int rpv, data.pt_data.stop_points.at(sp).route_point_list) {
            BOOST_FOREACH(unsigned int vj, data.pt_data.route_points.at(rpv).vehicle_journey_list) {
                unsigned int tav = network::get_ta_idx(data.pt_data.vehicle_journeys.at(vj).stop_time_list.at(data.pt_data.route_points.at(rpv).order), data);
                if((network::get_n_type(tav, data) == network::TA) && (network::get_saidx(tav, data, g, map_tc) == v2) && (predecessors[tav] != tav))  {
                    parcours p = parcours();
                    std::list<idx_t> stop_times = std::list<idx_t>();
                    network::edge_t etemp, eprec;
                    bool eexists;
                    idx_t sadescente = network::get_saidx(tav, data, g, map_tc);
                    boost::tie(eprec, eexists) = edge(rpv, tav, g);
                    network::vertex_t v;
                    p.ajouter_etape(data.pt_data.routes.at(data.pt_data.vehicle_journeys.at(data.pt_data.stop_times.at(network::get_idx(tav, data, map_tc)).vehicle_journey_idx).route_idx).mode_type_idx
                                    , network::get_saidx(tav, data, g, map_tc),
                                    data.pt_data.routes.at(data.pt_data.vehicle_journeys.at(data.pt_data.stop_times.at(network::get_idx(v, data, map_tc)).vehicle_journey_idx).route_idx).line_idx);
                    for(v=tav;  (network::get_saidx(v, data, g, map_tc)!=v1) & (v!=predecessors[v]); v=predecessors[v]) {
                        boost::tie(etemp, eexists) = boost::edge(predecessors[v], v, g);
                        if(eexists) {
                            if(network::est_transport(etemp, data, g)& !network::est_transport(eprec, data, g)) {
                                //Descente
                                sadescente = network::get_saidx(v, data, g, map_tc);
                                stop_times.push_front(network::get_idx(v, data, map_tc));
                            }
                            if(!network::est_transport(etemp, data, g)& network::est_transport(eprec, data, g)) {
                                //Montée
                                p.ajouter_etape(data.pt_data.routes.at(data.pt_data.vehicle_journeys.at(data.pt_data.stop_times.at(network::get_idx(v, data, map_tc)).vehicle_journey_idx).route_idx).mode_type_idx,
                                                network::get_saidx(v, data, g, map_tc),
                                                data.pt_data.routes.at(data.pt_data.vehicle_journeys.at(data.pt_data.stop_times.at(network::get_idx(v, data, map_tc)).vehicle_journey_idx).route_idx).line_idx);
                                stop_times.push_front(network::get_idx(v, data, map_tc));
                            }
                            eprec = etemp;
                        }
                    }
                    if(v!=predecessors[v]) {
                        //                        std::cout  << "ajout d'un itineraire" << std::endl;
                        //Ajout de l'étape de montée
                        p.ajouter_etape(data.pt_data.routes.at(data.pt_data.vehicle_journeys.at(data.pt_data.stop_times.at(network::get_idx(v, data, map_tc)).vehicle_journey_idx).route_idx).mode_type_idx,
                                        v1,
                                        data.pt_data.routes.at(data.pt_data.vehicle_journeys.at(data.pt_data.stop_times.at(network::get_idx(v, data, map_tc)).vehicle_journey_idx).route_idx).line_idx);
                        stop_times.push_front(network::get_idx(v, data, map_tc));

                        //On regarde si l'on connait un parcours identique
                        uint32_t pkey = parcours_list.size();
                        BOOST_FOREACH(map_parcours::value_type &pi, parcours_list) {
                            if(pi.second == p )
                                pkey = pi.first;
                        }
                        //Si non on l'ajoute à la liste des parcours
                        if(pkey == parcours_list.size())
                            parcours_list[pkey] = p;
                        //On ajoute l'itineraire à la liste des itineraires
                        itineraires.push_back(itineraire(pkey, stop_times));

                    }

                }
            }
        }
    }


    sort(itineraires.begin(), itineraires.end(), sort_itineraire(data));
}





}
