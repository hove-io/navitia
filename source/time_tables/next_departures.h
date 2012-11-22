#pragma once
#include "ptreferential/ptreferential.h"
#include "routing/raptor.h"

namespace navitia { namespace timetables {

typedef std::pair<routing::DateTime, type::idx_t> dt_st;
struct comp_st {
    bool operator()(const dt_st st1, const dt_st st2) const {

        return st1.first < st2.first;
    }
};

std::string iso_string(const nt::Data & d, int date, int hour);

/**
 * @brief next_departures : Tous les départs compris entre date_time et max_datetime ( dans la limite de nb_departures) correspondant à la requete. Sert principalement à générer la liste des route
 * points, à gérer les erreurs, et à générer les réponses à protocole buffer.
 * @param request : Requete au format ptref
 * @param str_dt : datetime au format iso à partir de laquelle on veut tous les départs
 * @param str_max_dt : datetime maximale au format iso
 * @param nb_departures : Nombre maximum de départ
 * @param data
 * @param raptor
 * @return : Réponse au format protocole buffer
 */
pbnavitia::Response next_departures(const std::string &request, const std::string &str_dt, const std::string &str_max_dt,
                                    const int nb_departures, const int depth, type::Data & data);

/**
 * @brief next_departures : Renvoie tous les departures partant de la liste des route points
 * @param route_points : Les route points à partir desquels on veut les départs
 * @param dt : Datetime de départ
 * @param max_dt : Datetime maximale
 * @param nb_departures : Nombre maximal de départs
 * @param raptor : Sert pour les données
 * @return : Renvoie de paire de datetime, st.idx de départs. La liste est triée selon les datetimes.
 */
std::vector<dt_st> next_departures(const std::vector<type::idx_t> &route_points, const routing::DateTime &dt,
                                   const routing::DateTime &max_dt, const int nb_departures, type::Data & data);
}}
