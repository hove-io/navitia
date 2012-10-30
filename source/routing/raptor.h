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

    ///Données modifiées pendant le calcul
    ///Contient les heures d'arrivées, de départ, ainsi que la façon dont on est arrivé à chaque route point à chaque tour
    map_retour_t retour;
    ///Contient les meilleures heures d'arrivées, de départ, ainsi que la façon dont on est arrivé à chaque route point
    map_int_pint_t best;
    ///Contient tous les points d'arrivée, et la meilleure façon dont on est arrivé à destination
    best_dest b_dest;
    ///Nombre de correspondances effectuées jusqu'à présent
    unsigned int count;
    ///Est-ce que le route point est marqué ou non ?
    boost::dynamic_bitset<> marked_rp;
    ///Est-ce que le stop point est arrivé ou non ?
    boost::dynamic_bitset<> marked_sp;
    ///La route est elle valide ?
    boost::dynamic_bitset<> routes_valides;
    ///L'ordre du premier route point de la route
    queue_t Q;

    //Constructeur
    RAPTOR(const navitia::type::Data &data) :  data(data), best(data.pt_data.route_points.size()),
                                               marked_rp(data.pt_data.route_points.size()),
                                               marked_sp(data.pt_data.stop_points.size()),
                                               routes_valides(data.pt_data.routes.size()),
                                               Q(data.dataRaptor.routes.size()) {
        retour.assign(20, data.dataRaptor.retour_constant);
    }

    ///Initialise les structure retour et b_dest
    void init(std::vector<std::pair<type::idx_t, double> > departs,
              std::vector<std::pair<type::idx_t, double> > destinations,
              const DateTime &dep, DateTime borne, const bool clockwise, const bool reset, const bool dep_dest = false);

    ///Cherche le temps de départ du calcul
    DateTime get_temps_depart(const DateTime &dt_depart, const std::vector<std::pair<type::idx_t, double> > &departs);

    ///Lance un calcul d'itinéraire entre deux stop areas
    std::vector<Path> compute(idx_t departure_idx, idx_t destination_idx, int departure_hour,
                              int departure_day, bool clockwise = true);
    ///Lance un calcul d'itinéraire entre deux stop areas avec aussi une borne
    std::vector<Path> compute(idx_t departure_idx, idx_t destination_idx, int departure_hour,
                              int departure_day, DateTime borne, bool clockwise = true);
    ///Construit tous chemins trouvés
    std::vector<Path> makePathes(std::vector<std::pair<type::idx_t, double> > departs, std::vector<std::pair<type::idx_t, double> > destinations);
    ///Construit tous les chemins trouvés, lorsque le calcul est lancé dans le sens inverse
    std::vector<Path> makePathesreverse(std::vector<std::pair<type::idx_t, double> > departs, std::vector<std::pair<type::idx_t, double> > destinations);

    ///Calcul d'itinéraires dans le sens horaire à partir de plusieurs stop points de départs, vers plusieurs stoppoints d'arrivée, à une heure donnée
    std::vector<Path> 
    compute_all(const std::vector<std::pair<type::idx_t, double> > &departs,
                const std::vector<std::pair<type::idx_t, double> > &destinations,
                const DateTime &dt_depart, const DateTime &borne = DateTime::inf,
                const std::multimap<std::string, std::string> & forbidden = std::multimap<std::string, std::string>());
    ///Calcul d'itinéraires dans le sens horaire à partir de plusieurs stop points de départs, vers plusieurs stoppoints d'arrivée, à partir d'une collection horaires
    std::vector<Path> 
    compute_all(const std::vector<std::pair<type::idx_t, double> > &departs,
                const std::vector<std::pair<type::idx_t, double> > &destinations,
                std::vector<DateTime> dt_departs, const DateTime &borne);
    ///Calcul d'itinéraires dans le sens horaire inversé à partir de plusieurs stop points de départs, vers plusieurs stoppoints d'arrivée, à une heure donnée
    std::vector<Path> 
    compute_reverse_all(const std::vector<std::pair<type::idx_t, double> > &departs,
                        const std::vector<std::pair<type::idx_t, double> > &destinations,
                        std::vector<DateTime> dt_departs, const DateTime &borne); 
    ///Calcul d'itinéraires dans le sens horaire inversé à partir de plusieurs stop points de départs, vers plusieurs stoppoints d'arrivée, à partir d'une collection horaires
    std::vector<Path> 
    compute_reverse_all(const std::vector<std::pair<type::idx_t, double> > &departs,
                        const std::vector<std::pair<type::idx_t, double> > &destinations,
                        const DateTime &dt_depart, const DateTime &borne = DateTime::min,
                        const std::multimap<std::string, std::string> & forbidden = std::multimap<std::string, std::string>());

    /// Désactive les routes qui n'ont pas de vj valides la veille, le jour, et le lendemain du calcul
    /// Gère également les lignes, modes, routes et VJ interdits
    void set_routes_valides(uint32_t date, const std::multimap<std::string, std::string> & forbidden);

    ///Boucle principale, parcourt les routes,
    void boucleRAPTOR();
    ///Construit un chemin
    Path makePath(std::vector<std::pair<type::idx_t, double> > departs,
                  unsigned int destination_idx, unsigned int countb, bool reverse = false);
    ///Marche à pied à l'interieur d'un stop point et entre deux stop points
    void marcheapied();
    ///Correspondances garanties et prolongements de service
    void route_path_connections_forward(); 
    ///Trouve pour chaque route, le premier route point auquel on peut embarquer, se sert de marked_rp
    void make_queue();
    ///Cherche le premier trip partant apres dt sur la route au route point order
    int  earliest_trip(const type::Route & route, const unsigned int order, const DateTime &dt) const;

    ///Route parcourant dans le sens anti-horaire
    void boucleRAPTORreverse(bool global_pruning = true);

    ///Boucle principale
    template<typename Visitor>
    void raptor_loop(Visitor visitor, bool global_pruning = true);

    ///Construit un chemin, utilisé lorsque l'algorithme a été fait en sens anti-horaire
    Path makePathreverse(std::vector<std::pair<type::idx_t, double> > departs,
                         unsigned int destination_idx, unsigned int countb);
     ///Marche à pied à l'interieur d'un stop point et entre deux stop points
    void marcheapiedreverse();
    ///Correspondances garanties et prolongements de service
    void route_path_connections_backward(); 
    ///Trouve pour chaque route, le premier route point auquel on peut embarquer, se sert de marked_rp
    void make_queuereverse();
    ///Cherche le premier trip partant avant dt sur la route au route point order
    int tardiest_trip(const type::Route & route, const unsigned int order, const DateTime &dt) const;

    ~RAPTOR() {}

};



}}}
