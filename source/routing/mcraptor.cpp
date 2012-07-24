#include "mcraptor.h"
namespace navitia { namespace routing { namespace raptor { namespace mcraptor {

//template<typename label_template, typename label_visitor>
//Path McRAPTOR<label_template, label_visitor>::compute_raptor(map_int_pint_t departs, map_int_pint_t destinations) {
//    return compute_raptor_all(departs, destinations).front();
//}

/// Implémentation de RAPTOR pour du multicritères
template<typename label_template, typename label_visitor>
std::vector<Path> McRAPTOR<label_template, label_visitor>::compute_raptor_all(std::unordered_map<int, label_template> departs, std::unordered_map<int, label_template> destinations) {

    bags.init(data.pt_data.stop_areas.size(), destinations, this);

    std::vector<unsigned int> marked_stop;
    unsigned int route, p;

    BOOST_FOREACH(auto depart, departs) {
        bags[std::make_pair(0, depart.first)].ajouter_label(depart.second);
        marked_stop.push_back(depart.first);
    }
    //Pour tous les k
    map_int_int_t Q;
    unsigned int count = 1;

    unsigned int count_marked_stop = 0;
    while(((Q.size() > 0) && (count < 5)) || count == 1) {
        //On intialise la queue
        Q = this->make_queue(marked_stop);
        marked_stop.clear();
        BOOST_FOREACH(map_int_int_t::value_type vq, Q) {
            route = vq.first;
            p = vq.second;
            //On initialise un bad vide Br
            Bag_route Br(this, route);
            //On traverse chaque route r en  partant de p
            for(unsigned int i = get_rp_order(route, p); i < data.pt_data.routes[route].route_point_list.size(); ++i) {
                unsigned int said = get_sa_rp(i, route);
                Br.update(i);

                if(bags[std::make_pair(count, said)].merge(Br)) {
                    marked_stop.push_back(said);
                    ++count_marked_stop;
                }

                if(bags.exists(std::make_pair(count-1, said))) {
                    Br.merge(bags[std::make_pair(count-1, said)], i);
                }
            }
        }

        BOOST_FOREACH(auto stop_area, marked_stop) {
            BOOST_FOREACH(auto stop_p, data.pt_data.stop_areas.at(stop_area).stop_point_list) {
                auto it_fp = foot_path.find(stop_p);
                if(it_fp != foot_path.end()) {
                    BOOST_FOREACH(auto connection_idx, (*it_fp).second) {
                        unsigned int saiddest = data.pt_data.stop_points.at(data.pt_data.connections[connection_idx].destination_stop_point_idx).stop_area_idx;
                        Bag bag_temp = bags[std::make_pair(count, stop_p)];
                        BOOST_FOREACH(label_template &lbl, bag_temp.labels) {
                            lbl.ajouterfootpath(data.pt_data.connections[connection_idx].duration);
                            marked_stop.push_back(saiddest);
                        }
                        bags[std::make_pair(count, saiddest)].merge(bag_temp);
                    }
                }
            }


        }
        ++count;
    }


    std::vector<Path> result;
    result.push_back(Path());

    std::cout << " fin calcule ! " << std::endl;
    //    bool stop = false;
    //    int countdebut = 0;

    for(unsigned int i=1; i<count; ++i) {
        BOOST_FOREACH(auto item, destinations) {
            if(!this->label_vistor.dominated_by(bags[std::make_pair(i,item.first)].best, bags.best_bags[item.first].best)) {
                BOOST_FOREACH(auto fpc, bags[std::make_pair(i,item.first)].labels) {
                    std::cout << "Label : " << fpc << " count(" << i << ")" << " destination : " << data.pt_data.stop_areas.at(item.first).name  << std::endl;
                }

                //                std::cout << "Arrivée à " << destinations[j] << " à " << bags[std::make_pair(i,destinations[j])].best.dt << " avec " << i << " correspondaces "<< std::endl;
            } else {
                std::cout << "Arrivée dominée  à " << item.first << " à " << bags[std::make_pair(i,item.first)].best.dt << " avec " << (i-1) << " correspondances "<< std::endl;
            }
        }
    }
    return result;
}


template<typename label_template, typename label_visitor>
bool McRAPTOR<label_template, label_visitor>::Bag::merge(McRAPTOR<label_template, label_visitor>::Bag_route &bagr) {
    bool test = false;

    BOOST_FOREACH(label_template lbl, bagr.labels) {
        test = test || ajouter_label(lbl);
    }

    if(test) {
        auto iter = this->labels.begin();
        auto iter_end = this->labels.end();

        auto new_end = std::remove_if(iter, iter_end, dominates(this->best, this->raptor->label_vistor));
        this->labels = list_label_t(this->labels.begin(), new_end);
    }

    return test;
}

template<typename label_template, typename label_visitor>
bool McRAPTOR<label_template, label_visitor>::Bag::merge(McRAPTOR<label_template, label_visitor>::Bag &bagr) {
    bool test = false;

    BOOST_FOREACH(label_template lbl, bagr.labels) {
        test = test || ajouter_label(lbl);
    }

    if(test) {
        auto iter = this->labels.begin();
        auto iter_end = this->labels.end();

        auto new_end = std::remove_if(iter, iter_end, dominates(this->best, this->raptor->label_vistor));
        this->labels = list_label_t(this->labels.begin(), new_end);
    }

    return test;
}


template<typename label_template, typename label_visitor>
bool McRAPTOR<label_template, label_visitor>::Bag_route::ajouter_label(label_template &lbl) {
    bool test = !this->raptor->label_vistor.dominated_by(lbl, this->best);

    if(test) {
        this->labels.push_back(lbl);
    }
    return test;
}

template<typename label_template, typename label_visitor>
bool McRAPTOR<label_template, label_visitor>::Best_Bag::ajouter_label(label_template &lbl) {
    bool test = !this->raptor->label_vistor.dominated_by(lbl, this->best);
    if(test) {
        if(said != -1)
            this->raptor->bags.ajouter_label_dest(lbl, said);
        this->labels.push_back(lbl);
        this->raptor->label_vistor.keepthebest(lbl, this->best);
    }
    return test;
}


template<typename label_template, typename label_visitor>
bool McRAPTOR<label_template, label_visitor>::Bag::ajouter_label(label_template &lbl) {
    bool test = false;


    if(!this->raptor->label_vistor.dominated_by(lbl, this->bp.best)/* && !this->raptor->label_vistor.dominated_by(lbl, this->raptor->bags.best_bag_dest.best)*/) {
        this->bp.ajouter_label(lbl);
        test = !this->raptor->label_vistor.dominated_by(lbl, this->best);

        if(test) {
            this->labels.push_back(lbl);
            this->raptor->label_vistor.keepthebest(lbl, this->best);
        }
    }
    return test;
}

template<typename label_template, typename label_visitor>
void McRAPTOR<label_template, label_visitor>::Bag_route::update(unsigned int order) {
    this->best = label_template();

    for(auto iter = this->labels.begin(); iter != this->labels.end(); ++iter) {
        if((*iter).vjid != -1) {
            this->raptor->label_vistor.update(order, *iter);
            this->raptor->label_vistor.keepthebest(this->best, *iter);
        }
    }
}

template<typename label_template, typename label_visitor>
bool McRAPTOR<label_template, label_visitor>::Bag_route::merge(McRAPTOR<label_template, label_visitor>::Bag &bag, unsigned int order) {
    bool test = false;
    unsigned int said = this->raptor->data.pt_data.stop_points.at(this->raptor->data.pt_data.route_points.at(this->raptor->data.pt_data.routes.at(route).route_point_list.at(order)).stop_point_idx).stop_area_idx;
    int etemp;
    bool pam;
    typedef label_template label;

    BOOST_FOREACH(auto lbl, bag.labels) {
        std::tie(etemp, pam) = this->raptor->earliest_trip(this->route, said, lbl.dt);
        if(etemp != -1) {
            label_template lbl_temp(lbl);
            lbl_temp.vjid = etemp;
            lbl_temp.stid = this->raptor->data.pt_data.vehicle_journeys.at(etemp).stop_time_list.at(order);
            lbl_temp.dt = DateTime(lbl.dt.date, this->raptor->data.pt_data.stop_times.at(lbl_temp.stid).arrival_time);
            if(pam)
                ++lbl_temp.dt.date;
            test = test || ajouter_label(lbl_temp);
        }
    }

    auto iter = this->labels.begin();
    auto iter_end = this->labels.end();
    auto new_end = std::remove_if(iter, iter_end, dominates(this->best, this->raptor->label_vistor));
    this->labels = list_label_t(this->labels.begin(), new_end);

    return test;
}
}}}}
