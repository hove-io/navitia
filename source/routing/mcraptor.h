#pragma once
#include "raptor.h"
#include "boost/variant.hpp"
namespace navitia { namespace routing { namespace raptor { namespace mcraptor {

struct label_parent {
    int vjid;
    int stid;
    DateTime dt;

    label_parent() : vjid(-1), stid(-1) {}
    label_parent(const label_parent & lbl) : vjid(lbl.vjid), stid(lbl.stid), dt(lbl.dt) {}

};

template<typename label_template, typename label_visitor_t>
struct McRAPTOR : navitia::routing::raptor::communRAPTOR {

    label_visitor_t label_vistor;

    McRAPTOR(navitia::type::Data &data, label_visitor_t label_vistor) : communRAPTOR(data), label_vistor(label_vistor) {}

    struct dominates {
        label_template &best;
        label_visitor_t &label_vistor;
        dominates(label_template &best, label_visitor_t &label_vistor) : best(best), label_vistor(label_vistor) {}
        bool operator()(const label_template &lbl) {
            return label_vistor.dominated_by(lbl, best);
        }
    };




    typedef std::unordered_map<int, label_template> map_label_t;
    typedef std::vector<label_template> list_label_t;
    class Parent_Bag {
    public:
        list_label_t labels;
        label_template best;
        McRAPTOR * raptor;

        Parent_Bag() : labels(), best() {}
        Parent_Bag(const Parent_Bag& parent): labels(), best(), raptor(parent.raptor) {
            BOOST_FOREACH(auto label, parent.labels) {
                labels.push_back(label_template(label));
            }
        }

        Parent_Bag(McRAPTOR * raptor) :labels(),  best(), raptor(raptor) {}
        bool dominated_by(label_template lbl) {
            return label_vistor.dominated_by(best, lbl);
        }

        label_template get_best(int crit) {return best[crit];}

        label_template& operator[](const int i) {
            return labels[i] = label_template();
        }

        virtual bool ajouter_label(label_template lbl) = 0;
    };

    //Forward declaration
    class Bag_route;

    class Best_Bag : public Parent_Bag {
    public:
        int said;
        Best_Bag() {}
        Best_Bag(const Best_Bag & bb) : Parent_Bag(bb), said(bb.said) {}
        Best_Bag(McRAPTOR * raptor, unsigned int said) : Parent_Bag(raptor), said(said) {}
        bool ajouter_label(label_template lbl);

    };


    class Bag : public Parent_Bag {
    public :
        Best_Bag &bp;

        Bag(const Bag& bag) : Parent_Bag(bag), bp(bag.bp) { }
        Bag(McRAPTOR * raptor, Best_Bag &bp) : Parent_Bag(raptor), bp(bp) { }



        bool ajouter_label(label_template lbl);
        bool merge(Bag_route &bagr);
        bool merge(Bag &bagr);
    };

    class Bag_route : public Parent_Bag {
    public:
        unsigned int route;

        Bag_route(McRAPTOR * raptor) : Parent_Bag(raptor)  {}
        Bag_route(McRAPTOR * raptor, unsigned int route) : Parent_Bag(raptor), route(route) {}

        bool ajouter_label(label_template lbl) ;
        void update(unsigned int order) ;
        bool merge(Bag &bagr, unsigned int order) ;
    };


    struct bags_t {
        std::vector<unsigned int> destinations;
        McRAPTOR * raptor;
        boost::unordered_map<std::pair<unsigned int, unsigned int>, Bag> bags;
        boost::unordered_map<int, Best_Bag>best_bags;
        Best_Bag best_bag_dest;


        bags_t() : destinations() {}

        bags_t(unsigned int p, std::vector<unsigned int>destinations, McRAPTOR * raptor) : destinations(destinations), raptor(raptor) {
            for(unsigned int i = 0; i < p; ++i) {
                best_bags.insert(std::make_pair(i, Best_Bag(raptor, i)));
            }
            best_bag_dest.said = -1;
            best_bag_dest.raptor = raptor;
        }

        void init(unsigned int p, std::vector<unsigned int>destinations_, McRAPTOR * raptor_) {
            destinations = destinations_;
            raptor = raptor_;
            for(unsigned int i = 0; i < p; ++i) {
                best_bags.insert(std::make_pair(i, Best_Bag(raptor, i)));
            }
            best_bag_dest.said = -1;
            best_bag_dest.raptor = raptor;
        }

        bool exists(std::pair<unsigned int, unsigned int> index) {
            return bags.find(index) != bags.end();
        }

        Bag &operator[](std::pair<unsigned int, unsigned int> index) {
            auto iter = bags.find(index);
            if(iter == bags.end()) {
                return (*bags.insert(std::make_pair(index, Bag(raptor, best_bags[index.second]))).first).second;
            } else {
                return (*iter).second;
            }
        }
        void ajouter_label_dest(label_template lbl, unsigned int destination) {
            if(std::count(destinations.begin(), destinations.end(), destination) > 0) {
                best_bag_dest.ajouter_label(lbl);
            }
        }

    };
    bags_t bags;

    typedef std::unordered_map<int, label_template> map_int_T;
    Path compute_raptor(map_int_pint_t departs_, map_int_pint_t destinations_) {
        map_int_T departs;
        BOOST_FOREACH(auto depart, departs_) {
            departs[depart.first] = label_template(depart.second);
        }
        std::vector<unsigned int> destinations;

        BOOST_FOREACH(auto destination, destinations_) {
            destinations.push_back(destination.first);
        }
        return compute_raptor(departs, destinations);
    }

    std::vector<Path> compute_raptor_all(map_int_T departs, std::vector<unsigned int> destinations);
    Path compute_raptor(map_int_T departs, std::vector<unsigned int> destinations);


};
}}}}

