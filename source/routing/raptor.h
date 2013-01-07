#pragma once
#include "type/type.h"
#include "type/data.h"
#include "routing.h"
#include "utils/timer.h"
#include <unordered_map>
#include "boost/dynamic_bitset.hpp"
#include "routing/raptor_utils.h"
#include "routing/dataraptor.h"
#include "best_trip.h"
#include "raptor_init.h"

namespace navitia { namespace routing { namespace raptor{

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
                                               Q(data.pt_data.routes.size()) {
        retour.assign(20, data.dataRaptor.retour_constant);
    }

    ///Initialise les structure retour et b_dest
    void clear_and_init(std::vector<init::Departure_Type> departs,
              std::vector<std::pair<type::idx_t, double> > destinations, DateTime borne, const bool clockwise, const bool clear, const float walking_speed);

    ///Lance un calcul d'itinéraire entre deux stop areas
    std::vector<Path> compute(idx_t departure_idx, idx_t destination_idx, int departure_hour,
                              int departure_day, bool clockwise = true, const bool wheelchair = false);
    ///Lance un calcul d'itinéraire entre deux stop areas avec aussi une borne
    std::vector<Path> compute(idx_t departure_idx, idx_t destination_idx, int departure_hour,
                              int departure_day, DateTime borne, bool clockwise = true, const bool wheelchair = false);
    ///Construit tous chemins trouvés
    std::vector<Path> makePathes(std::vector<std::pair<type::idx_t, double> > destinations, DateTime dt, const float walking_speed);
    ///Construit tous les chemins trouvés, lorsque le calcul est lancé dans le sens inverse
    std::vector<Path> makePathesreverse(std::vector<std::pair<type::idx_t, double> > destinations, DateTime dt, const float walking_speed);

    ///Calcul d'itinéraires dans le sens horaire à partir de plusieurs stop points de départs, vers plusieurs stoppoints d'arrivée, à une heure donnée
    std::vector<Path> 
    compute_all(const std::vector<std::pair<type::idx_t, double> > &departs,
                const std::vector<std::pair<type::idx_t, double> > &destinations,
                const DateTime &dt_depart, const DateTime &borne = DateTime::inf, const float walking_speed=1.38, const bool wheelchair = false,
                const std::multimap<std::string, std::string> & forbidden = std::multimap<std::string, std::string>());
    ///Calcul d'itinéraires dans le sens horaire à partir de plusieurs stop points de départs, vers plusieurs stoppoints d'arrivée, à partir d'une collection horaires
    std::vector<Path> 
    compute_all(const std::vector<std::pair<type::idx_t, double> > &departs,
                const std::vector<std::pair<type::idx_t, double> > &destinations,
                std::vector<DateTime> dt_departs, const DateTime &borne, const float walking_speed=1.38, const bool wheelchair = false);
    ///Calcul d'itinéraires dans le sens horaire inversé à partir de plusieurs stop points de départs, vers plusieurs stoppoints d'arrivée, à une heure donnée
    std::vector<Path> 
    compute_reverse_all(const std::vector<std::pair<type::idx_t, double> > &departs,
                        const std::vector<std::pair<type::idx_t, double> > &destinations,
                        std::vector<DateTime> dt_departs, const DateTime &borne, const float walking_speed=1.38, const bool wheelchair = false);
    ///Calcul d'itinéraires dans le sens horaire inversé à partir de plusieurs stop points de départs, vers plusieurs stoppoints d'arrivée, à partir d'une collection horaires
    std::vector<Path> 
    compute_reverse_all(const std::vector<std::pair<type::idx_t, double> > &departs,
                        const std::vector<std::pair<type::idx_t, double> > &destinations,
                        const DateTime &dt_depart, const DateTime &borne = DateTime::min, const float walking_speed=1.38, const bool wheelchair = false,
                        const std::multimap<std::string, std::string> & forbidden = std::multimap<std::string, std::string>());

    /// Désactive les routes qui n'ont pas de vj valides la veille, le jour, et le lendemain du calcul
    /// Gère également les lignes, modes, routes et VJ interdits
    void set_routes_valides(uint32_t date, const std::multimap<std::string, std::string> & forbidden);

    ///Boucle principale, parcourt les routes,
    void boucleRAPTOR(const bool wheelchair, bool global_pruning = true);
    ///Construit un chemin
    Path makePath(type::idx_t destination_idx, unsigned int countb, bool reverse = false);

    /// Fonction générique pour la marche à pied
    /// Il faut spécifier le visiteur selon le sens souhaité
    template<typename Visitor> void foot_path(const Visitor & v, const bool wheelchair);
    ///Marche à pied à l'interieur d'un stop point et entre deux stop points
    void marcheapied(const bool wheelchair);
    ///Correspondances garanties et prolongements de service
    void route_path_connections_forward(const bool wheelchair);
    ///Trouve pour chaque route, le premier route point auquel on peut embarquer, se sert de marked_rp
    void make_queue();

    ///Route parcourant dans le sens anti-horaire
    void boucleRAPTORreverse(const bool wheelchair, bool global_pruning = true);

    ///Boucle principale
    template<typename Visitor>
    void raptor_loop(Visitor visitor, const bool wheelchair = false, bool global_pruning = true);

    ///Construit un chemin, utilisé lorsque l'algorithme a été fait en sens anti-horaire
    Path makePathreverse(unsigned int destination_idx, unsigned int countb);
     ///Marche à pied à l'interieur d'un stop point et entre deux stop points
    void marcheapiedreverse(const bool wheelchair);
    ///Correspondances garanties et prolongements de service
    void route_path_connections_backward(const bool wheelchair);
    ///Trouve pour chaque route, le premier route point auquel on peut embarquer, se sert de marked_rp
    void make_queuereverse();

    ~RAPTOR() {}

};



}}}
