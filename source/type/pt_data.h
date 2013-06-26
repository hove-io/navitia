#pragma once
#include "type.h"
#include "georef/georef.h"
#include "type/message.h"
#include "autocomplete/autocomplete.h"
#include "proximity_list/proximity_list.h"

#include <boost/serialization/map.hpp>

namespace navitia { namespace type {

// Maps pour external code
// À refléchir si un hash_map ne serait pas mieux ; pas forcément en lecture car hasher une chaîne c'est plus long que comparer
// En attendant on utilise std::map car on sait le sérialiser...
typedef std::map<std::string, idx_t> ExtCodeMap;

struct PT_Data : boost::noncopyable{
#define COLLECTION_AND_MAP(type_name, collection_name) std::vector<type_name*> collection_name; ExtCodeMap collection_name##_map;
    ITERATE_NAVITIA_PT_TYPES(COLLECTION_AND_MAP)

    std::vector<StopTime*> stop_times;
    std::vector<JourneyPatternPointConnection*> journey_pattern_point_connections;
    std::vector<StopPointConnection*> stop_point_connections;

    // First letter
    autocomplete::Autocomplete<idx_t> stop_area_autocomplete;
    autocomplete::Autocomplete<idx_t> stop_point_autocomplete;
    autocomplete::Autocomplete<idx_t> line_autocomplete;

    // Proximity list
    proximitylist::ProximityList<idx_t> stop_area_proximity_list;
    proximitylist::ProximityList<idx_t> stop_point_proximity_list;

    //Message
    MessageHolder message_holder;

    /** Fonction qui permet de sérialiser (aka binariser la structure de données
      *
      * Elle est appelée par boost et pas directement
      */
    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar
        #define SERIALIZE_ELEMENTS(type_name, collection_name) & collection_name & collection_name##_map
                ITERATE_NAVITIA_PT_TYPES(SERIALIZE_ELEMENTS)
                & stop_times
                // Les firstLetter
                & stop_area_autocomplete & stop_point_autocomplete
                // Les proximity list
                & stop_area_proximity_list & stop_point_proximity_list & line_autocomplete
                // Les types un peu spéciaux
                & stop_point_connections & journey_pattern_point_connections;
    }

    /** Initialise tous les indexes
      *
      * Les données doivent bien évidemment avoir été initialisés
      */
    void build_index();

    /** Construit l'indexe ExternelCode */
    void build_uri();

    /** Construit l'indexe Autocomplete */
    void build_autocomplete(const navitia::georef::GeoRef&);

    /** Construit l'indexe ProximityList */
    void build_proximity_list();

    /// tris les collections et affecte un idx a chaque élément
    void sort();

    /** Retrouve un élément par un attribut arbitraire de type chaine de caractères
      *
      * Le template a été surchargé pour gérer des const char* (string passée comme literal)
      */
    template<class RequestedType>
    std::vector<RequestedType*> find(std::string RequestedType::* attribute, const char * str){
        return find(attribute, std::string(str));
    }

    PT_Data& operator=(PT_Data&& other);

    /** Définis les idx des différents objets */
    void index();
};

}}
