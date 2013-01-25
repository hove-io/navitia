#pragma once
#include "ptreferential/ptreferential.h"
#include "routing/routing.h"
#include "type/pb_converter.h"
#include "get_stop_times.h"
#include <unordered_map>

namespace navitia { namespace timetables {

typedef std::pair<dt_st, dt_st> pair_dt_st;

/**
 * @brief get_arrival_order : Fait un matching entre les route points de departure_routepoint et ceux obtenus via arrival_filter.
 *                            Le matching est effectué si les deux route_points appartiennent à la même route, et que l'ordre du route_point de départ est inférieur à celui d'arrivée.
 * @param departure_routepoint : Les route_points de départs
 * @param arrival_filter : Les route points d'arrivée
 * @param data : Un objet data
 * @return : Renvoie l'idx des routes points de départ et l'ordre des route points de d'arrivée
 */
std::unordered_map<type::idx_t, uint32_t> get_arrival_order(const std::vector<type::idx_t> &departure_routepoint, const std::string &arrival_filter, type::Data & data);

/**
 * @brief departure_board : Renvoie un vecteur de paires de DateTime/StopTime, correspondant au matching effectué par departure_board
 * @param departure_filter : Le filtre pour les route_points de départ
 * @param arrival_filter : Le filtre pour les route_points d'arrivée
 * @param datetime : L'heure à partir de laquelle on veut les horaires
 * @param max_datetime : L'heure maximale pour les route_points
 * @param nb_departures : Le nombre maximale de départ
 * @param data : Un objet data
 * @param raptor : Un objet raptor
 * @return : Un vecteur de paires de DateTime/StopTime
 */
std::vector<pair_dt_st> stops_schedule(const std::string &departure_filter, const std::string &arrival_filter,
                                        const routing::DateTime &datetime, const routing::DateTime &max_datetime, const int nb_departures,
                                        type::Data & data);

/**
 * @brief departure_board : Renvoie un flux protocole, fait appel à la fonction departure_board renvoyant les paires DateTime/StopTime.
 * @param departure_filter : Le filtre pour les route_points de départ
 * @param arrival_filter : Le filtre pour les route_points d'arrivée
 * @param str_dt : Une chaine de caractère décrivant en format iso l'heure à partir de laquelle on veut les horaires.
 * @param str_max_dt : Une chaine de caractère décrivant en format iso l'heure maximale pour les horaires.
 * @param nb_departures : Le nombre maximale de paire d'horaires que l'on veut.
 * @param depth : La profondeur maximale que l'on veut pour les objets protobuff stopTime.
 * @param data : Un objet data.
 * @param raptor : Un objet raptor.
 * @return : Un object protobuff de type DEPARTURE_BOARD
 */
pbnavitia::Response stops_schedule(const std::string &departure_filter, const std::string &arrival_filter,
                                    const std::string &str_dt, uint32_t duration,
                                    uint32_t nb_stoptimes, uint32_t depth, type::Data & data);

}}
