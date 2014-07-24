/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.
  
This file is part of Navitia,
    the software to build cool stuff with public transport.
 
Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
Help us simplify mobility and open public transport:
    a non ending quest to the responsive locomotion way of traveling!
  
LICENCE: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
   
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.
   
You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
  
Stay tuned using
twitter @navitia 
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#pragma once
#include <unordered_map>
#include <queue>
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
#include "raptor_solutions.h"
#include "raptor_utils.h"
#include "type/time_duration.h"

namespace navitia { namespace routing {

/** Worker Raptor : une instance par thread, les données sont modifiées par le calcul */
struct RAPTOR
{
    const navitia::type::Data & data;

    ///Contient les heures d'arrivées, de départ, ainsi que la façon dont on est arrivé à chaque journey_pattern point à chaque tour
    std::vector<label_vector_t> labels;
    ///Contient les meilleures heures d'arrivées, de départ, ainsi que la façon dont on est arrivé à chaque journey_pattern point
    std::vector<DateTime> best_labels;
    ///Contient tous les points d'arrivée, et la meilleure façon dont on est arrivé à destination
    best_dest b_dest;
    ///Nombre de correspondances effectuées jusqu'à présent
    unsigned int count;
    /// The best jpp reached for every stop point during a round of raptor algorithm
    /// Used by footpath()
    std::vector<type::idx_t> best_jpp_by_sp;
    ///La journey_pattern est elle valide ?
    boost::dynamic_bitset<> valid_journey_patterns;
    boost::dynamic_bitset<> valid_journey_pattern_points;
    ///L'ordre du premier j: public AbstractRouterourney_pattern point de la journey_pattern
    queue_t Q;

    //Constructeur
    RAPTOR(const navitia::type::Data &data) :
        data(data), best_labels(data.pt_data->journey_pattern_points.size()), count(0),
        best_jpp_by_sp(data.pt_data->stop_points.size()),
        valid_journey_patterns(data.pt_data->journey_patterns.size()),
        valid_journey_pattern_points(data.pt_data->journey_pattern_points.size()),
        Q(data.pt_data->journey_patterns.size()) {
            labels.assign(10, data.dataRaptor->labels_const);
    }



    void clear(bool clockwise, DateTime bound);

    ///Initialise les structure retour et b_dest
    void init(Solutions departures,
              std::vector<std::pair<type::idx_t, navitia::time_duration> > destinations,
              navitia::DateTime bound, const bool clockwise,
              const type::Properties &properties = 0);


    ///Lance un calcul d'itinéraire entre deux stop areas avec aussi une borne
    std::vector<Path>
    compute(const type::StopArea* departure, const type::StopArea* destination,
            int departure_hour, int departure_day, DateTime bound, bool disruption_active,
            bool allow_odt,
            bool clockwise = true,
            /*const type::Properties &required_properties = 0*/
            const type::AccessibiliteParams & accessibilite_params = type::AccessibiliteParams(), uint32_t
            max_transfers=std::numeric_limits<uint32_t>::max(),
            const std::vector<std::string>& forbidden_uris = {});


    /** Calcul d'itinéraires dans le sens horaire à partir de plusieurs 
     *  stop points de départs, vers plusieurs stoppoints d'arrivée,
     *  à une heure donnée.
     */
    std::vector<Path>
    compute_all(const std::vector<std::pair<type::idx_t, navitia::time_duration>> &departs,
                const std::vector<std::pair<type::idx_t, navitia::time_duration>> &destinations,
                const DateTime &departure_datetime, bool disruption_active, bool allow_odt,
                const DateTime &bound=DateTimeUtils::inf,
                const uint32_t max_transfers=std::numeric_limits<int>::max(),
                const type::AccessibiliteParams & accessibilite_params = type::AccessibiliteParams(),
                const std::vector<std::string> & forbidden = std::vector<std::string>(), bool clockwise=true);


    
    /** Calcul l'isochrone à partir de tous les points contenus dans departs,
     *  vers tous les autres points.
     *  Renvoie toutes les arrivées vers tous les stop points.
     */
    void
    isochrone(const std::vector<std::pair<type::idx_t, navitia::time_duration>> &departures_,
              const DateTime &departure_datetime, const DateTime &bound = DateTimeUtils::min,
              uint32_t max_transfers = std::numeric_limits<uint32_t>::max(),
              const type::AccessibiliteParams & accessibilite_params = type::AccessibiliteParams(),
              const std::vector<std::string>& forbidden = std::vector<std::string>(),
              bool clockwise = true, bool disruption_active = false, bool allow_odt = true);


    /// Désactive les journey_patterns qui n'ont pas de vj valides la veille, le jour, et le lendemain du calcul
    /// Gère également les lignes, modes, journey_patterns et VJ interdits
    void set_valid_jp_and_jpp(uint32_t date, const std::vector<std::string> & forbidden,
                                      bool disruption_active,
                                      bool allow_odt);

    ///Boucle principale, parcourt les journey_patterns,
    void boucleRAPTOR(const type::AccessibiliteParams & accessibilite_params, bool clockwise, bool disruption_active,
                      bool global_pruning = true,
                      const uint32_t max_transfers=std::numeric_limits<uint32_t>::max());

    /// Fonction générique pour la marche à pied
    /// Il faut spécifier le visiteur selon le sens souhaité
    template<typename Visitor> void foot_path(const Visitor & v);

    template<typename Visitor>
    void apply_vj_extension(const Visitor& v, const bool global_pruning,
                            const type::VehicleJourney* prev_vj, type::idx_t boarding_jpp_idx,
                            DateTime workingDt, const uint16_t l_zone,
                            const bool disruption_active);

    void make_queue();

    ///Boucle principale
    template<typename Visitor>
    void raptor_loop(Visitor visitor, const type::AccessibiliteParams & accessibilite_params, bool disruption_active, bool global_pruning = true, uint32_t max_transfers=std::numeric_limits<uint32_t>::max());


    /// Retourne à quel tour on a trouvé la meilleure solution pour ce journey_patternpoint
    /// Retourne -1 s'il n'existe pas de meilleure solution
    int best_round(type::idx_t journey_pattern_point_idx);

    inline boarding_type get_type(size_t count, type::idx_t jpp_idx) const {
        return labels[count][jpp_idx].type;
    }

    inline type::idx_t get_boarding_jpp(size_t count, type::idx_t jpp_idx) const {
        return labels[count][jpp_idx].boarding_jpp;
    }

    template<typename Visitor>
    inline
    void mark_all_jpp_of_sp(const type::StopPoint* stop_point, const DateTime dt, const type::idx_t boarding_jpp,
                            label_vector_t& working_labels, Visitor visitor) {
        for(auto jpp : stop_point->journey_pattern_point_list) {
            type::idx_t jpp_idx = jpp->idx;
            if(jpp_idx != boarding_jpp && visitor.comp(dt, best_labels[jpp_idx])) {
               working_labels[jpp_idx].dt = dt;
               working_labels[jpp_idx].boarding_jpp = boarding_jpp;
               working_labels[jpp_idx].type = boarding_type::connection;
               best_labels[jpp_idx] = dt;
               if(visitor.comp(jpp->order, Q[jpp->journey_pattern->idx])) {
                   Q[jpp->journey_pattern->idx] = jpp->order;
               }
            }
        }
    }

    ~RAPTOR() {}
};

}}
