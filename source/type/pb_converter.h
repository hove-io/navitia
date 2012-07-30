#pragma once
#include "data.h"
#include "type/type.pb.h"

namespace navitia{

/**
 * fonction générique pour convertir un objet navitia en un message protocol buffer
 *
 * @param idx idetypeifiatype de l'objet à convertir
 * @param data reférence vers l'objet Data de l'application
 * @param message l'objet protocol buffer a remplir
 * @param depth profondeur de remplissage
 *
 * @throw std::out_of_range si l'idx n'est pas valide
 * @throw std::bad_cast si le message PB n'est pas adapté
 */
template<type::Type_e type>
void fill_pb_object(type::idx_t idx, const type::Data &data, google::protobuf::Message* message, int max_depth = 0);

template<>
void fill_pb_object<type::eCity>(type::idx_t idx, const type::Data &data, google::protobuf::Message* message, int);

/**
 * spécialisation de fill_pb_object pour les StopArea
 *
 */
template<>
void fill_pb_object<type::eStopArea>(type::idx_t idx, const type::Data &data, google::protobuf::Message* message, int max_depth);

/**
 * spécialisation de fill_pb_object pour les StopPoint
 *
 */
template<>
void fill_pb_object<type::eStopPoint>(type::idx_t idx, const type::Data &data, google::protobuf::Message* message, int max_depth);
}//namespace navitia
