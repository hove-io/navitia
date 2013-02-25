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
#include "raptor_path.h"

namespace navitia { namespace routing { namespace raptor {

/** Worker Raptor : une instance par thread, les données sont modifiées par le calcul */
struct RAPTOR : public AbstractRouter
{
    const navitia::type::Data & data;

    ///Contient les heures d'arrivées, de départ, ainsi que la façon dont on est arrivé à chaque route point à chaque tour
    map_labels_t labels;
    ///Contient les meilleures heures d'arrivées, de départ, ainsi que la façon dont on est arrivé à chaque route point
    label_vector_t best_labels;
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
    RAPTOR(const navitia::type::Data &data) :  
        data(data), best_labels(data.pt_data.route_points.size()),
        marked_rp(data.pt_data.route_points.size()),
        marked_sp(data.pt_data.stop_points.size()),
        routes_valides(data.pt_data.routes.size()),
        Q(data.pt_data.routes.size()) {
            labels.assign(20, data.dataRaptor.labels_const);
    }

    ///Initialise les structure retour et b_dest
    void clear_and_init(std::vector<init::Departure_Type> departs,
              std::vector<std::pair<type::idx_t, double> > destinations,
              navitia::type::DateTime borne, const bool clockwise, const bool clear,
              const float walking_speed, const int walking_distance);

    ///Lance un calcul d'itinéraire entre deux stop areas
    std::vector<Path> 
    compute(idx_t departure_idx, idx_t destination_idx, int departure_hour,
            int departure_day, bool clockwise = true,
            const bool wheelchair = false);
    ///Lance un calcul d'itinéraire entre deux stop areas avec aussi une borne
    std::vector<Path> 
    compute(idx_t departure_idx, idx_t destination_idx, int departure_hour,
            int departure_day, navitia::type::DateTime borne, bool clockwise = true,
            const bool wheelchair = false);

    template<typename Visitor>
    std::vector<Path> compute_all(const std::vector<std::pair<type::idx_t, double> > &departs,
                                  const std::vector<std::pair<type::idx_t, double> > &destinations,
                                  const navitia::type::DateTime &dt_depart, const navitia::type::DateTime &borne,
                                  const float walking_speed, const int walking_distance, const bool wheelchair,
                                  const std::multimap<std::string, std::string> & forbidden,
                                  Visitor vis);

    /** Calcul d'itinéraires dans le sens horaire à partir de plusieurs 
     *  stop points de départs, vers plusieurs stoppoints d'arrivée,
     *  à une heure donnée.
     */
    std::vector<Path> 
    compute_all(const std::vector<std::pair<type::idx_t, double> > &departs,
                const std::vector<std::pair<type::idx_t, double> > &destinations,
                const navitia::type::DateTime &dt_depart, const navitia::type::DateTime &borne = navitia::type::DateTime::inf,
                const float walking_speed=1.38, const int walking_distance = 1000, const bool wheelchair = false,
                const std::multimap<std::string, std::string> & forbidden = std::multimap<std::string, std::string>());

    /** Calcul d'itinéraires dans le sens horaire à partir de plusieurs 
     *  stop points de départs, vers plusieurs stoppoints d'arrivée, 
     *  à partir d'une collection horaires.
     */
    std::vector<Path> 
    compute_all(const std::vector<std::pair<type::idx_t, double> > &departs,
                const std::vector<std::pair<type::idx_t, double> > &destinations,
                std::vector<navitia::type::DateTime> dt_departs, const navitia::type::DateTime &borne, 
                const float walking_speed=1.38, const int walking_distance = 1000, const bool wheelchair = false);

    /** Calcul d'itinéraires dans le sens horaire inversé à partir de plusieurs
     *  stop points de départs, vers plusieurs stoppoints d'arrivée,
     *  à une heure donnée.
     */
    std::vector<Path> 
    compute_reverse_all(const std::vector<std::pair<type::idx_t, double> > &departs,
                        const std::vector<std::pair<type::idx_t, double> > &destinations,
                        std::vector<navitia::type::DateTime> dt_departs, const navitia::type::DateTime &borne,
                        float walking_speed=1.38, int walking_distance = 1000,
                        bool wheelchair = false);

    /** Calcul d'itinéraires dans le sens horaire inversé à partir de plusieurs
     *  stop points de départs, vers plusieurs stoppoints d'arrivée,
     *  à partir d'une collection horaires.
     */
    std::vector<Path> 
    compute_reverse_all(const std::vector<std::pair<type::idx_t, double> > &departs,
                        const std::vector<std::pair<type::idx_t, double> > &destinations,
                        const navitia::type::DateTime &dt_depart, const navitia::type::DateTime &borne = navitia::type::DateTime::min, float walking_speed=1.38,
                        int walking_distance = 1000, bool wheelchair = false,
                        const std::multimap<std::string, std::string> & forbidden = std::multimap<std::string, std::string>());
    
    /** Calcul l'isochrone à partir de tous les points contenus dans departs,
     *  vers tous les autres points.
     *  Renvoie toutes les arrivées vers tous les stop points.
     */
    void
    isochrone(const std::vector<std::pair<type::idx_t, double> > &departs,
              const navitia::type::DateTime &dt_depart, const navitia::type::DateTime &borne = navitia::type::DateTime::min,
              float walking_speed=1.38, int walking_distance = 1000, bool wheelchair = false,
              const std::multimap<std::string, std::string> & forbidden = std::multimap<std::string, std::string>(),
              bool clockwise = true);


    /// Désactive les routes qui n'ont pas de vj valides la veille, le jour, et le lendemain du calcul
    /// Gère également les lignes, modes, routes et VJ interdits
    void set_routes_valides(uint32_t date, const std::multimap<std::string, std::string> & forbidden);

    ///Boucle principale, parcourt les routes,
    void boucleRAPTOR(const bool wheelchair, bool global_pruning = true);
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

     ///Marche à pied à l'interieur d'un stop point et entre deux stop points
    void marcheapiedreverse(const bool wheelchair);
    ///Correspondances garanties et prolongements de service
    void route_path_connections_backward(const bool wheelchair);
    ///Trouve pour chaque route, le premier route point auquel on peut embarquer, se sert de marked_rp
    void make_queuereverse();

    /// Retourne à quel tour on a trouvé la meilleure solution pour ce routepoint
    /// Retourne -1 s'il n'existe pas de meilleure solution
    int best_round(type::idx_t route_point_idx);

    ~RAPTOR() {}

};



}}}
