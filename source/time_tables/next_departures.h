#pragma once
#include "ptreferential/ptreferential.h"
#include "type/pb_converter.h"


namespace navitia { namespace timetables {


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


}}
