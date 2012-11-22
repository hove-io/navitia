#pragma once
#include "routing/routing.h"
#include "type/data.h"


namespace navitia { namespace timetables {
typedef std::pair<routing::DateTime, const type::idx_t> dt_st;

struct comp_st {
    bool operator()(const dt_st st1, const dt_st st2) const {

        return st1.first < st2.first;
    }
};


std::string iso_string(const nt::Data & d, int date, int hour);

/**
 * @brief get_stop_times : Renvoie tous les departures partant de la liste des route points
 * @param route_points : Les route points à partir desquels on veut les départs
 * @param dt : Datetime de départ
 * @param max_dt : Datetime maximale
 * @param nb_departures : Nombre maximal de départs
 * @param raptor : Sert pour les données
 * @return : Renvoie de paire de datetime, st.idx de départs. La liste est triée selon les datetimes.
 */
std::vector<dt_st> get_stop_times(const std::vector<type::idx_t> &route_points, const routing::DateTime &dt,
                                   const routing::DateTime &max_dt, const int nb_departures, type::Data & data);

}}
