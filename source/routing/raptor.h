#pragma once
#include "type/type.h"
#include "type/data.h"
#include "routing.h"
#include "utils/timer.h"
#include <unordered_map>
#include "boost/dynamic_bitset.hpp"
#include "routing/raptor_utils.h"
#include "routing/dataraptor.h"

namespace navitia { namespace routing { namespace raptor{

typedef std::pair<navitia::type::idx_t, int> pair_idx_int;
typedef std::vector<int> queue_t;
typedef std::vector<map_int_pint_t> map_retour_t;
typedef std::vector<pair_int> vector_pairint;
typedef std::pair<navitia::type::idx_t, type_retour> idx_retour;
typedef std::vector<idx_retour> vector_idxretour;
typedef std::pair<navitia::type::idx_t, double> idx_distance;
typedef std::vector<idx_distance> vec_idx_distance;


struct RAPTOR : public AbstractRouter
{
    const navitia::type::Data & data;

    //Données modifiées pendant le calcul
    map_retour_t retour;
    map_int_pint_t best;
    best_dest b_dest;
    unsigned int count;
    boost::dynamic_bitset<> marked_sp;
    boost::dynamic_bitset<> routes_valides;
    queue_t Q;

    //Constructeur
    RAPTOR(const navitia::type::Data &data) :  data(data), best(data.pt_data.stop_points.size()),
                                               marked_sp(data.pt_data.stop_points.size()),
                                               routes_valides(data.pt_data.routes.size()),
                                               Q(data.dataRaptor.routes.size()) {
        retour.assign(20, data.dataRaptor.retour_constant);
    }

    void init(std::vector<std::pair<type::idx_t, double> > departs, 
              std::vector<std::pair<type::idx_t, double> > destinations,
              const DateTime dep, DateTime borne, const bool clockwise, const bool reset);

    std::vector<Path> compute(idx_t departure_idx, idx_t destination_idx, int departure_hour,
                              int departure_day, bool clockwise = true);

    std::vector<Path> compute(idx_t departure_idx, idx_t destination_idx, int departure_hour,
                              int departure_day, DateTime borne, bool clockwise = true);

    std::vector<Path> makePathes(std::vector<std::pair<type::idx_t, double> > departs);

    std::vector<Path> makePathesreverse(std::vector<std::pair<type::idx_t, double> > departs);

    std::vector<Path> 
    compute_all(const std::vector<std::pair<type::idx_t, double> > &departs,
                const std::vector<std::pair<type::idx_t, double> > &destinations,
                const DateTime &dt_depart, const DateTime &borne = DateTime::inf);
    
    std::vector<Path> 
    compute_all(const std::vector<std::pair<type::idx_t, double> > &departs,
                const std::vector<std::pair<type::idx_t, double> > &destinations,
                std::vector<DateTime> dt_departs, const DateTime &borne);
    std::vector<Path> 
    compute_reverse_all(const std::vector<std::pair<type::idx_t, double> > &departs,
                        const std::vector<std::pair<type::idx_t, double> > &destinations,
                        std::vector<DateTime> dt_departs, const DateTime &borne); 
    std::vector<Path> 
    compute_reverse_all(const std::vector<std::pair<type::idx_t, double> > &departs,
                        const std::vector<std::pair<type::idx_t, double> > &destinations,
                        const DateTime &dt_depart, const DateTime &borne = DateTime::min);


    void set_routes_valides(const DateTime & dtDepart);

    void boucleRAPTOR();
    Path makePath(std::vector<std::pair<type::idx_t, double> > departs,
                  unsigned int destination_idx, unsigned int countb, bool reverse = false);
    void marcheapied();
    void make_queue();
    int  earliest_trip(const type::Route & route, const unsigned int order, const DateTime &dt) const;

    void boucleRAPTORreverse();

    template<typename Visitor>
    void raptor_loop(Visitor visitor);

    Path makePathreverse(std::vector<std::pair<type::idx_t, double> > departs,
                         unsigned int destination_idx, unsigned int countb);
    void marcheapiedreverse();
    void make_queuereverse();
    int tardiest_trip(const type::Route & route, const unsigned int order, const DateTime &dt) const;


};



}}}
