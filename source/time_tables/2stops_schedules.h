#pragma once
#include "ptreferential/ptreferential.h"
#include "routing/routing.h"
#include "type/pb_converter.h"
#include "get_stop_times.h"
#include <unordered_map>

namespace navitia { namespace timetables {

typedef std::pair<datetime_stop_time, datetime_stop_time> pair_dt_st;

/**
 * @brief get_arrival_order : Fait un matching entre les journey_pattern points de departure_journey_patternpoint et ceux obtenus via arrival_filter.
 *                            Le matching est effectué si les deux journey_pattern_points appartiennent à la même journey_pattern, et que l'ordre du journey_pattern_point de départ est inférieur à celui d'arrivée.
 * @param departure_journey_patternpoint : Les journey_pattern_points de départs
 * @param arrival_filter : Les journey_pattern points d'arrivée
 * @param data : Un objet data
 * @return : Renvoie l'idx des journey_patterns points de départ et l'ordre des journey_pattern points de d'arrivée
 */
std::unordered_map<type::idx_t, uint32_t> get_arrival_order(const std::vector<type::idx_t> &departure_journey_patternpoint, const std::string &arrival_filter, type::Data & data);

/**
 * @brief departure_board : Renvoie un vecteur de paires de DateTime/StopTime, correspondant au matching effectué par departure_board
 * @param departure_filter : Le filtre pour les journey_pattern_points de départ
 * @param arrival_filter : Le filtre pour les journey_pattern_points d'arrivée
 * @param datetime : L'heure à partir de laquelle on veut les horaires
 * @param max_datetime : L'heure maximale pour les journey_pattern_points
 * @param nb_departures : Le nombre maximale de départ
 * @param data : Un objet data
 * @param raptor : Un objet raptor
 * @return : Un vecteur de paires de DateTime/StopTime
 */
std::vector<pair_dt_st> stops_schedule(const std::string &departure_filter, const std::string &arrival_filter,
                                       const std::vector<std::string>& forbidden_uris,
                                       const DateTime &datetime, const DateTime &max_datetime,
                                       type::Data & data, bool without_disrupt);

/**
 * @brief departure_board : Renvoie un flux protocole, fait appel à la fonction departure_board renvoyant les paires DateTime/StopTime.
 * @param departure_filter : Le filtre pour les journey_pattern_points de départ
 * @param arrival_filter : Le filtre pour les journey_pattern_points d'arrivée
 * @param str_dt : Une chaine de caractère décrivant en format iso l'heure à partir de laquelle on veut les horaires.
 * @param str_max_dt : Une chaine de caractère décrivant en format iso l'heure maximale pour les horaires.
 * @param nb_departures : Le nombre maximale de paire d'horaires que l'on veut.
 * @param depth : La profondeur maximale que l'on veut pour les objets protobuff stopTime.
 * @param data : Un objet data.
 * @param raptor : Un objet raptor.
 * @return : Un object protobuff de type DEPARTURE_BOARD
 */
pbnavitia::Response stops_schedule(const std::string &departure_filter, const std::string &arrival_filter,
                                   const std::vector<std::string>& forbidden_uris,
                                   const std::string &str_dt, uint32_t duration, uint32_t depth, type::Data & data, bool without_disrupt);

}}
