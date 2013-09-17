#pragma once
#include "routing/routing.h"
#include "type/data.h"


namespace navitia { namespace timetables {
typedef std::pair<DateTime, const type::StopTime*> datetime_stop_time;

std::string iso_string(const nt::Data & d, int date, int hour);


/**
 * @brief get_stop_times : Renvoie tous les departures partant de la liste des journey_pattern points
 * @param journey_pattern_points : Les journey_pattern points à partir desquels on veut les départs
 * @param dt : Datetime de départ
 * @param max_dt : Datetime maximale
 * @param nb_departures : Nombre maximal de départs
 * @param raptor : Sert pour les données
 * @return : Renvoie de paire de datetime, st.idx de départs. La liste est triée selon les datetimes.
 */
std::vector<datetime_stop_time> get_stop_times(const std::vector<type::idx_t> &journey_pattern_points, const DateTime &dt,
                                   const DateTime &max_dt, const size_t max_departures, const type::Data & data,
                                   const type::AccessibiliteParams & accessibilite_params = type::AccessibiliteParams()/*const bool wheelchair = false*/);



}}
