#pragma once
#include <unordered_map>
#include <limits>
#include "type/type.h"
#include "type/data.h"
#include "type/datetime.h"
#include "routing.h"
#include "utils/timer.h"
#include "boost/dynamic_bitset.hpp"
#include "dataraptor.h"
#include "best_stoptime.h"
#include "raptor_path.h"
#include "raptor_init.h"
#include "raptor_utils.h"

namespace navitia { namespace routing {

/** Worker Raptor : une instance par thread, les données sont modifiées par le calcul */
struct RAPTOR
{
    const navitia::type::Data & data;

    ///Contient les heures d'arrivées, de départ, ainsi que la façon dont on est arrivé à chaque journey_pattern point à chaque tour
    std::vector<label_vector_t> labels;
    std::vector<vector_idx> boardings;
    ///Contient les meilleures heures d'arrivées, de départ, ainsi que la façon dont on est arrivé à chaque journey_pattern point
    label_vector_t best_labels;
    ///Contient tous les points d'arrivée, et la meilleure façon dont on est arrivé à destination
    best_dest b_dest;
    ///Nombre de correspondances effectuées jusqu'à présent
    unsigned int count;
    ///Est-ce que le journey_pattern point est marqué ou non ?
    boost::dynamic_bitset<> marked_rp;
    ///Est-ce que le stop point est arrivé ou non ?
    boost::dynamic_bitset<> marked_sp;
    ///La journey_pattern est elle valide ?
    boost::dynamic_bitset<> journey_patterns_valides;
    ///L'ordre du premier j: public AbstractRouterourney_pattern point de la journey_pattern
    queue_t Q;

    //Constructeur
    RAPTOR(const navitia::type::Data &data) :  
        data(data), best_labels(data.pt_data.journey_pattern_points.size()), count(0),
        marked_rp(data.pt_data.journey_pattern_points.size()),
        marked_sp(data.pt_data.stop_points.size()),
        journey_patterns_valides(data.pt_data.journey_patterns.size()),
        Q(data.pt_data.journey_patterns.size()) {
            labels.assign(20, data.dataRaptor.labels_const);
    }



    void clear(const type::Data & data, bool clockwise, type::DateTime borne, int walking_duration);

    ///Initialise les structure retour et b_dest
    void clear_and_init(std::vector<Departure_Type> departs,
              std::vector<std::pair<type::idx_t, double> > destinations,
              navitia::type::DateTime borne, const bool clockwise,
              const float walking_speed, const int walking_distance,
              const type::Properties &properties = 0);


    ///Lance un calcul d'itinéraire entre deux stop areas avec aussi une borne
    std::vector<Path> 
    compute(idx_t departure_idx, idx_t destination_idx, int departure_hour,
            int departure_day, navitia::type::DateTime borne, bool clockwise = true,
            const type::Properties &required_properties = 0, uint32_t
            max_transfers=std::numeric_limits<uint32_t>::max());


    /** Calcul d'itinéraires dans le sens horaire à partir de plusieurs 
     *  stop points de départs, vers plusieurs stoppoints d'arrivée,
     *  à une heure donnée.
     */
    std::vector<Path> 
    compute_all(const std::vector<std::pair<type::idx_t, double> > &departs,
                const std::vector<std::pair<type::idx_t, double> > &destinations,
                const navitia::type::DateTime &dt_depart, const navitia::type::DateTime &borne = navitia::type::DateTime::inf,
                const uint32_t max_transfers=std::numeric_limits<int>::max(),
                const float walking_speed=1.38, const int walking_distance = 1000, const type::Properties &required_properties = 0,
                const std::vector<std::string> & forbidden = std::vector<std::string>(), bool clockwise = true);


    
    /** Calcul l'isochrone à partir de tous les points contenus dans departs,
     *  vers tous les autres points.
     *  Renvoie toutes les arrivées vers tous les stop points.
     */
    void
    isochrone(const std::vector<std::pair<type::idx_t, double> > &departs,
              const navitia::type::DateTime &dt_depart, const navitia::type::DateTime &borne = navitia::type::DateTime::min, uint32_t max_transfers = std::numeric_limits<uint32_t>::max(),
              float walking_speed=1.38, int walking_distance = 1000, const type::Properties &required_properties = 0,
              const std::vector<std::string>& forbidden = std::vector<std::string>(),
              bool clockwise = true);


    /// Désactive les journey_patterns qui n'ont pas de vj valides la veille, le jour, et le lendemain du calcul
    /// Gère également les lignes, modes, journey_patterns et VJ interdits
    void set_journey_patterns_valides(uint32_t date, const std::vector<std::string> & forbidden);

    ///Boucle principale, parcourt les journey_patterns,
    void boucleRAPTOR(const type::Properties &required_properties, bool clockwise,
                      bool global_pruning = true,
                      const uint32_t max_transfers=std::numeric_limits<uint32_t>::max());

    /// Fonction générique pour la marche à pied
    /// Il faut spécifier le visiteur selon le sens souhaité
    template<typename Visitor> void foot_path(const Visitor & v, const type::Properties &required_properties);

    ///Correspondances garanties et prolongements de service
    template<typename Visitor> void journey_pattern_path_connections(const Visitor &visitor/*, const type::Properties &required_properties*/);

    ///Trouve pour chaque journey_pattern, le premier journey_pattern point auquel on peut embarquer, se sert de marked_rp
    void make_queue();

    ///Boucle principale
    template<typename Visitor>
    void raptor_loop(Visitor visitor, const type::Properties &required_properties, bool global_pruning = true, uint32_t max_transfers=std::numeric_limits<uint32_t>::max());


    /// Retourne à quel tour on a trouvé la meilleure solution pour ce journey_patternpoint
    /// Retourne -1 s'il n'existe pas de meilleure solution
    int best_round(type::idx_t journey_pattern_point_idx);

    inline boarding_type get_type(size_t count, type::idx_t journey_pattern_point) const {
        return navitia::routing::get_type(count, journey_pattern_point, labels, boardings, data);
    }

    inline type::idx_t get_boarding_jpp(size_t count, type::idx_t journey_pattern_point) const {
        return navitia::routing::get_boarding_jpp(count, journey_pattern_point, labels, boardings, data);
    }

    inline std::pair<type::idx_t, uint32_t> get_current_stidx_gap(size_t count, type::idx_t journey_pattern_point, const type::Properties &required_properties, bool clockwise) const {
        return navitia::routing::get_current_stidx_gap(count, journey_pattern_point, labels, boardings, required_properties, clockwise, data);
    }

    ~RAPTOR() {}

};



}}
