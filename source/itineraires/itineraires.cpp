#include "itineraires.h"

namespace itineraires {

bool etape::operator==(etape e2) {
    return (this->ligne == e2.ligne) & (this->descente == e2.descente);
}

bool etape::operator!=(etape e2) {
    return !((*this) == e2);
}

void parcours::ajouter_etape(idx_t ligne, idx_t depart) {
    this->etapes.push_front(etape(ligne, depart));
}

bool parcours::operator==(parcours i2) {
    if(this->etapes.size() != i2.etapes.size())
        return false;
    std::list<etape>::iterator it1, it2;
    for(it1 = this->etapes.begin(), it2 = i2.etapes.begin(); it1 != this->etapes.end(); ++it1, ++it2) {
        if((*it1) != (*it2))
            return false;
    }
    return true;
}

bool parcours::operator!=(parcours i2) {
    return !((*this) == i2);
}

void make_itineraires(network::vertex_t v1, network::vertex_t v2, std::vector<itineraire> &itineraires, map_parcours &parcours_list, network::NW &g, navitia::type::Data &data, std::vector<network::vertex_t> &predecessors){

    BOOST_FOREACH(network::edge_t e1, out_edges(v2, g)) {
        network::vertex_t spv = target(e1, g);
        BOOST_FOREACH(network::edge_t e2, out_edges(spv, g)) {
            network::vertex_t rpv = target(e2, g);
            BOOST_FOREACH(network::edge_t e3, out_edges(rpv, g)) {
                network::vertex_t tav = target(e3, g);
                if((network::get_n_type(tav, data) == network::TA) & (network::get_saidx(tav, data) == v2) & (predecessors[tav] != tav))  {

                    parcours p = parcours();
                    std::list<idx_t> stop_times = std::list<idx_t>();
                    network::edge_t etemp, eprec;
                    bool eexists;
                    idx_t sadescente = network::get_saidx(tav, data);
                    boost::tie(eprec, eexists) = edge(rpv, tav, g);
                    network::vertex_t v;
                    p.ajouter_etape(data.pt_data.routes.at(data.pt_data.vehicle_journeys.at(data.pt_data.stop_times.at(network::get_idx(tav, data)).vehicle_journey_idx).route_idx).line_idx, network::get_saidx(tav, data));
                    for(v=tav;  (network::get_saidx(v, data)!=v1) & (v!=predecessors[v]); v=predecessors[v]) {
                        boost::tie(etemp, eexists) = boost::edge(predecessors[v], v, g);
                        if(eexists) {
                            if(network::est_transport(g[etemp], data)& !network::est_transport(g[eprec], data)) {
                                //Descente
                                sadescente = network::get_saidx(v, data);
                                stop_times.push_front(network::get_idx(v, data));
                            }
                            if(!network::est_transport(g[etemp], data)& network::est_transport(g[eprec], data)) {
                                //Montée
                                p.ajouter_etape(data.pt_data.routes.at(data.pt_data.vehicle_journeys.at(data.pt_data.stop_times.at(network::get_idx(v, data)).vehicle_journey_idx).route_idx).line_idx, network::get_saidx(v, data));
                                stop_times.push_front(network::get_idx(v, data));
                            }
                            eprec = etemp;
                        }
                    }
                    if(v!=predecessors[v]) {
//                        std::cout  << "ajout d'un itineraire" << std::endl;
                        //Ajout de l'étape de montée
                        p.ajouter_etape(data.pt_data.routes.at(data.pt_data.vehicle_journeys.at(data.pt_data.stop_times.at(network::get_idx(v, data)).vehicle_journey_idx).route_idx).line_idx, v1);
                        stop_times.push_front(network::get_idx(v, data));

                        if(p.etapes.size() < 10) {
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
    }

    sort(itineraires.begin(), itineraires.end(), sort_itineraire(data));
}



}
