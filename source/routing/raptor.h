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
#include "raptor_utils.h"
#include "type/time_duration.h"

namespace navitia { namespace routing {

/*
 * Use to save routing test to launch stay_in
 */
struct RoutingState {
    const type::VehicleJourney* vj = nullptr;
    JppIdx boarding_jpp_idx = JppIdx();
    uint16_t l_zone = std::numeric_limits<uint16_t>::max();
    DateTime workingDate = DateTimeUtils::inf;

    RoutingState(const type::VehicleJourney* vj, JppIdx boarding_jpp_idx, uint16_t l_zone, DateTime workingDate) :
        vj(vj), boarding_jpp_idx(boarding_jpp_idx), l_zone(l_zone), workingDate(workingDate) {}
};

/** Worker Raptor : une instance par thread, les données sont modifiées par le calcul */
struct RAPTOR
{
    typedef std::pair<SpIdx, navitia::time_duration> stop_point_duration;
    typedef std::vector<stop_point_duration> vec_stop_point_duration;

    const navitia::type::Data& data;

    NextStopTime next_st;

    /// Contains the different labels used by raptor.
    /// Each element of index i in this vector represents the labels with i transfers
    std::vector<Labels> labels;
    std::vector<Labels> first_pass_labels;
    ///Contains the best arrival (or departure time) for each stoppoint
    IdxMap<type::StopPoint, DateTime> best_labels_pts;
    IdxMap<type::StopPoint, DateTime> best_labels_transfers;

    /// Number of transfers done for the moment
    unsigned int count;
    /// Are the journey pattern valid
    boost::dynamic_bitset<> valid_journey_patterns;
    dataRAPTOR::JppsFromSp jpps_from_sp;
    /// Order of the first journey_pattern point of each journey_pattern
    IdxMap<type::JourneyPattern, int> Q;

    // set to store if the stop_point is valid
    boost::dynamic_bitset<> valid_stop_points;

    explicit RAPTOR(const navitia::type::Data& data) :
        data(data),
        next_st(data),
        best_labels_pts(data.pt_data->stop_points),
        best_labels_transfers(data.pt_data->stop_points),
        count(0),
        valid_journey_patterns(data.pt_data->journey_patterns.size()),
        Q(data.pt_data->journey_patterns),
        valid_stop_points(data.pt_data->stop_points.size())
    {
        labels.assign(10, data.dataRaptor->labels_const);
        first_pass_labels.assign(10, data.dataRaptor->labels_const);
    }

    void clear(bool clockwise, DateTime bound);

    ///Initialize starting points
    void init(const vec_stop_point_duration& dep,
              const DateTime bound,
              const bool clockwise,
              const type::Properties& properties);

    // pt_data object getters by typed idx
    const type::JourneyPattern* get_jp(JpIdx idx) const {
        return data.pt_data->journey_patterns[idx.val];
    }
    const type::JourneyPatternPoint* get_jpp(JppIdx jpp_idx) const {
        return data.pt_data->journey_pattern_points[jpp_idx.val];
    }

    const type::StopPoint* get_sp(SpIdx idx) const {
        return data.pt_data->stop_points[idx.val];
    }

    ///Lance un calcul d'itinéraire entre deux stop areas avec aussi une borne
    std::vector<Path>
    compute(const type::StopArea* departure,
            const type::StopArea* destination,
            int departure_hour,
            int departure_day,
            DateTime bound,
            bool disruption_active,
            bool clockwise = true,
            const type::AccessibiliteParams& accessibilite_params = type::AccessibiliteParams(),
            uint32_t max_transfers = std::numeric_limits<uint32_t>::max(),
            const std::vector<std::string>& forbidden_uris = {},
            const boost::optional<navitia::time_duration>& direct_path_dur = boost::none);


    /** Calcul d'itinéraires multiples dans le sens horaire à partir de plusieurs
    * stop points de départs, vers plusieurs stoppoints d'arrivée,
    * à une heure donnée.
    */
    std::vector<Path>
    compute_all(const vec_stop_point_duration& departs,
                const vec_stop_point_duration& destinations,
                const DateTime& departure_datetime,
                bool disruption_active,
                const DateTime& bound = DateTimeUtils::inf,
                const uint32_t max_transfers = 10,
                const type::AccessibiliteParams& accessibilite_params = type::AccessibiliteParams(),
                const std::vector<std::string>& forbidden = std::vector<std::string>(),
                bool clockwise = true,
                const boost::optional<navitia::time_duration>& direct_path_dur = boost::none);


    /** Calcul d'itinéraires multiples dans le sens horaire à partir de plusieurs
     * stop points de départs, vers plusieurs stoppoints d'arrivée,
     * à une heure donnée.
     */
    std::vector<std::pair<type::EntryPoint, std::vector<Path>>>
    compute_nm_all(const std::vector<std::pair<type::EntryPoint, vec_stop_point_duration>>& departures,
                   const std::vector<std::pair<type::EntryPoint, vec_stop_point_duration>>& arrivals,
                   const DateTime& departure_datetime,
                   bool disruption_active, 
                   const DateTime& bound,
                   const uint32_t max_transfers,
                   const type::AccessibiliteParams& accessibilite_params,
                   const std::vector<std::string>& forbidden_uri,
                   bool clockwise);


    /** Calcul l'isochrone à partir de tous les points contenus dans departs,
     *  vers tous les autres points.
     *  Renvoie toutes les arrivées vers tous les stop points.
     */
    void
    isochrone(const vec_stop_point_duration& departures_,
              const DateTime& departure_datetime,
              const DateTime& bound = DateTimeUtils::min,
              uint32_t max_transfers = 10,
              const type::AccessibiliteParams& accessibilite_params = type::AccessibiliteParams(),
              const std::vector<std::string>& forbidden = std::vector<std::string>(),
              bool clockwise = true,
              bool disruption_active = false);


    /// Désactive les journey_patterns qui n'ont pas de vj valides la veille, le jour, et le lendemain du calcul
    /// Gère également les lignes, modes, journey_patterns et VJ interdits
    void set_valid_jp_and_jpp(uint32_t date,
                              const type::AccessibiliteParams& accessibilite_params,
                              const std::vector<std::string>& forbidden,
                              bool disruption_active);

    ///Boucle principale, parcourt les journey_patterns,
    void boucleRAPTOR(const type::AccessibiliteParams& accessibilite_params,
                      bool clockwise,
                      bool disruption_active,
                      const uint32_t max_transfers);

    /// Apply foot pathes to labels
    /// Return true if it improves at least one label, false otherwise
    template<typename Visitor> bool foot_path(const Visitor& v);

    /// Returns true if we improve at least one label, false otherwise
    template<typename Visitor>
    bool apply_vj_extension(const Visitor& v,
                            const bool disruption_active,
                            const RoutingState& state);

    ///Main loop
    template<typename Visitor>
    void raptor_loop(Visitor visitor,
                     const type::AccessibiliteParams& accessibilite_params,
                     bool disruption_active,
                     uint32_t max_transfers=std::numeric_limits<uint32_t>::max());

    /// Return the round that has found the best solution for this stop point
    /// Return -1 if no solution found
    int best_round(SpIdx sp_idx);

    /// First raptor loop
    /// externalized for testing purposes
    void first_raptor_loop(const vec_stop_point_duration& dep,
                           const DateTime& departure_datetime,
                           bool disruption_active,
                           const DateTime& bound,
                           const uint32_t max_transfers,
                           const type::AccessibiliteParams& accessibilite_params,
                           const std::vector<std::string>& forbidden_uri,
                           bool clockwise);

    ~RAPTOR() {}
};


}}
