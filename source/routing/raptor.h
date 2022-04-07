/* Copyright © 2001-2022, Hove and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Hove (www.hove.com).
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
channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#pragma once

#include "type/data.h"
#include "type/datetime.h"
#include "type/rt_level.h"
#include "type/time_duration.h"
#include "type/line.h"
#include "type/route.h"
#include "type/commercial_mode.h"
#include "type/physical_mode.h"
#include "type/network.h"
#include "type/stop_area.h"

#include "raptor_solution_reader.h"
#include "routing.h"
#include "routing/journey.h"
#include "routing/labels.h"
#include "utils/idx_map.h"
#include "utils/timer.h"
#include "dataraptor.h"
#include "raptor_utils.h"

#include "dataraptor.h"
#include <unordered_map>
#include <queue>
#include <limits>

namespace navitia {
namespace routing {

DateTime limit_bound(const bool clockwise, const DateTime departure_datetime, const DateTime bound);

struct StartingPointSndPhase {
    SpIdx sp_idx;
    unsigned count;
    DateTime end_dt;
    unsigned walking_dur;
    bool has_priority;
};

/** Worker Raptor : une instance par thread, les données sont modifiées par le calcul */
struct RAPTOR {
    enum class NEXT_STOPTIME_TYPE {
        UNCACHED,
        CACHED,
    };

    typedef std::list<Journey> Journeys;

    const navitia::type::Data& data;

    std::shared_ptr<const CachedNextStopTime> cached_next_st;
    std::shared_ptr<const NextStopTime> uncached_next_st;

    std::shared_ptr<const NextStopTimeInterface> next_st;

    /// Contains the different labels used by raptor.
    /// Each element of index i in this vector represents the labels with i transfers
    std::vector<Labels> labels;
    std::vector<Labels> first_pass_labels;

    /// Contains the best arrival (or departure time) for each stoppoint
    Labels best_labels;

    /// Number of transfers done for the moment
    unsigned int count;
    /// Are the journey pattern valid
    boost::dynamic_bitset<> valid_journey_patterns;
    dataRAPTOR::JppsFromSp jpps_from_sp;
    /// Order of the first journey_pattern point of each journey_pattern
    IdxMap<JourneyPattern, int> Q;

    // set to store if the stop_point is valid
    boost::dynamic_bitset<> valid_stop_points;

    log4cplus::Logger raptor_logger;

    explicit RAPTOR(const navitia::type::Data& data)
        : data(data),
          uncached_next_st(std::make_shared<const NextStopTime>(data)),
          best_labels(data.pt_data->stop_points),
          count(0),
          valid_journey_patterns(data.dataRaptor->jp_container.nb_jps()),
          Q(data.dataRaptor->jp_container.get_jps_values()),
          valid_stop_points(data.pt_data->stop_points.size()),
          raptor_logger(log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("raptor"))) {
        labels.assign(10, data.dataRaptor->labels_const);
        first_pass_labels.assign(10, data.dataRaptor->labels_const);
    }

    void clear(const bool clockwise, const DateTime bound);

    /// Initialize starting points
    void init(const map_stop_point_duration& dep,
              const DateTime bound,
              const bool clockwise,
              const type::Properties& properties);

    // pt_data object getters by typed idx
    const type::StopPoint* get_sp(SpIdx idx) const { return data.pt_data->stop_points[idx.val]; }

    /// Lance un calcul d'itinéraire entre deux stop areas avec aussi une borne
    std::vector<Path> compute(const type::StopArea* departure,
                              const type::StopArea* destination,
                              int departure_hour,
                              int departure_day,
                              DateTime borne,
                              const nt::RTLevel rt_level,
                              const navitia::time_duration& arrival_transfer_penalty,
                              const navitia::time_duration& walking_transfer_penalty = 2_min,
                              bool clockwise = true,
                              const type::AccessibiliteParams& accessibilite_params = type::AccessibiliteParams(),
                              uint32_t max_transfers = std::numeric_limits<uint32_t>::max(),
                              const std::vector<std::string>& forbidden_uri = {},
                              const boost::optional<navitia::time_duration>& direct_path_dur = boost::none,
                              const boost::optional<boost::posix_time::ptime>& current_datetime = boost::none);

    /** Calcul d'itinéraires multiples dans le sens horaire à partir de plusieurs
     * stop points de départs, vers plusieurs stoppoints d'arrivée,
     * à une heure donnée.
     */
    std::vector<Path> compute_all(const map_stop_point_duration& departures,
                                  const map_stop_point_duration& destinations,
                                  const DateTime& departure_datetime,
                                  const nt::RTLevel rt_level = type::RTLevel::Base,
                                  const navitia::time_duration& arrival_transfer_penalty = 2_min,
                                  const navitia::time_duration& walking_transfer_penalty = 2_min,
                                  const DateTime& bound = DateTimeUtils::inf,
                                  const uint32_t max_transfers = 10,
                                  const type::AccessibiliteParams& accessibilite_params = type::AccessibiliteParams(),
                                  const std::vector<std::string>& forbidden_uri = std::vector<std::string>(),
                                  const std::vector<std::string>& allowed_ids = std::vector<std::string>(),
                                  bool clockwise = true,
                                  const boost::optional<navitia::time_duration>& direct_path_dur = boost::none,
                                  const size_t max_extra_second_pass = 0,
                                  const boost::optional<boost::posix_time::ptime>& current_datetime = boost::none);

    Journeys compute_all_journeys(const map_stop_point_duration& departures,
                                  const map_stop_point_duration& destinations,
                                  const DateTime& departure_datetime,
                                  const nt::RTLevel rt_level,
                                  const navitia::time_duration& arrival_transfer_penalty,
                                  const navitia::time_duration& walking_transfer_penalty,
                                  const DateTime& bound = DateTimeUtils::inf,
                                  const uint32_t max_transfers = 10,
                                  const type::AccessibiliteParams& accessibilite_params = type::AccessibiliteParams(),
                                  bool clockwise = true,
                                  const boost::optional<navitia::time_duration>& direct_path_dur = boost::none,
                                  const size_t max_extra_second_pass = 0,
                                  const boost::optional<boost::posix_time::ptime>& current_datetime = boost::none);

    template <class T>
    std::vector<Path> from_journeys_to_path(const T& journeys) const {
        std::vector<Path> result;
        for (const auto& journey : journeys) {
            if (journey.sections.empty()) {
                continue;
            }
            result.push_back(make_path(journey, data));
        }
        return result;
    }

    /** Calcul l'isochrone à partir de tous les points contenus dans departs,
     *  vers tous les autres points.
     *  Renvoie toutes les arrivées vers tous les stop points.
     */
    void isochrone(const map_stop_point_duration& departures,
                   const DateTime& departure_datetime,
                   const DateTime& b = DateTimeUtils::min,
                   const uint32_t max_transfers = 10,
                   const type::AccessibiliteParams& accessibilite_params = type::AccessibiliteParams(),
                   const std::vector<std::string>& forbidden = std::vector<std::string>(),
                   const std::vector<std::string>& allowed = std::vector<std::string>(),
                   const bool clockwise = true,
                   const nt::RTLevel rt_level = nt::RTLevel::Base);

    /// Désactive les journey_patterns qui n'ont pas de vj valides la veille, le jour, et le lendemain du calcul
    /// Gère également les lignes, modes, journey_patterns et VJ interdits
    void set_valid_jp_and_jpp(uint32_t date,
                              const type::AccessibiliteParams&,
                              const std::vector<std::string>& forbidden,
                              const std::vector<std::string>& allowed,
                              const nt::RTLevel rt_level);

    /// Boucle principale, parcourt les journey_patterns,
    void boucleRAPTOR(const type::AccessibiliteParams& accessibilite_params,
                      const bool clockwise,
                      const nt::RTLevel rt_level,
                      const uint32_t max_transfers);

    /// Apply foot pathes to labels
    /// Return true if it improves at least one label, false otherwise
    template <typename Visitor>
    bool foot_path(const Visitor& v);

    /// Returns true if we improve at least one label, false otherwise
    template <typename Visitor>
    bool apply_vj_extension(const Visitor& v,
                            const nt::RTLevel rt_level,
                            const type::VehicleJourney* vj,
                            const uint16_t l_zone,
                            DateTime workingDate,
                            DateTime working_walking_duration,
                            SpIdx boarding_stop_point);

    /// Main loop
    template <typename Visitor>
    void raptor_loop(Visitor visitor,
                     const type::AccessibiliteParams& accessibilite_params,
                     const nt::RTLevel rt_level,
                     uint32_t max_transfers = std::numeric_limits<uint32_t>::max());

    /// Return the round that has found the best solution for this stop point
    /// Return -1 if no solution found
    int best_round(SpIdx sp_idx);

    /// First raptor loop
    /// externalized for testing purposes
    void first_raptor_loop(const map_stop_point_duration& departures,
                           const DateTime& departure_datetime,
                           const nt::RTLevel rt_level,
                           const DateTime& bound_limit,
                           const uint32_t max_transfers,
                           const type::AccessibiliteParams& accessibilite_params,
                           const bool clockwise,
                           const boost::optional<boost::posix_time::ptime>& current_datetime = boost::none);

    ~RAPTOR() = default;

    std::string print_all_labels();
    std::string print_starting_points_snd_phase(std::vector<StartingPointSndPhase>& starting_points);

private:
    NEXT_STOPTIME_TYPE choose_next_stop_time_type(
        const DateTime& departure_datetime,
        const boost::optional<boost::posix_time::ptime>& current_datetime) const;
    void set_next_stop_time(const DateTime& departure_datetime,
                            const nt::RTLevel rt_level,
                            const DateTime& bound_limit,
                            const type::AccessibiliteParams& accessibilite_params,
                            const bool clockwise,
                            const NEXT_STOPTIME_TYPE next_st_type);
};

}  // namespace routing
}  // namespace navitia
