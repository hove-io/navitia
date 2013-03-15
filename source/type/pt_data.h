#pragma once
#include "type.h"
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
#define COLLECTION_AND_MAP(type_name, collection_name) std::vector<type_name> collection_name; ExtCodeMap collection_name##_map;
    ITERATE_NAVITIA_PT_TYPES(COLLECTION_AND_MAP)
    std::vector<StopTime> stop_times;
    std::vector<JourneyPatternPointConnection> journey_pattern_point_connections;
    std::vector<std::vector<Connection> > stop_point_connections;

    // First letter
    autocomplete::Autocomplete<idx_t> stop_area_autocomplete;
    autocomplete::Autocomplete<idx_t> city_autocomplete;
    autocomplete::Autocomplete<idx_t> stop_point_autocomplete;

    // Proximity list
    proximitylist::ProximityList<idx_t> stop_area_proximity_list;
    proximitylist::ProximityList<idx_t> stop_point_proximity_list;
    proximitylist::ProximityList<idx_t> city_proximity_list;

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
                & stop_area_autocomplete & city_autocomplete & stop_point_autocomplete
                // Les proximity list
                & stop_area_proximity_list & stop_point_proximity_list & city_proximity_list
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
    void build_autocomplete(const std::map<std::string, std::string> & map_alias);

    /** Construit l'indexe ProximityList */
    void build_proximity_list();

    /** Retrouve un élément par un attribut arbitraire de type chaine de caractères
      *
      * Le template a été surchargé pour gérer des const char* (string passée comme literal)
      */
    template<class RequestedType>
    std::vector<RequestedType*> find(std::string RequestedType::* attribute, const char * str){
        return find(attribute, std::string(str));
    }


    /// Prefixe le type à l'uri
    template<typename T>
    void normalize_extcode(std::map<std::string, idx_t> & map){
        //std::string prefix = static_data::get()->captionByType(T::type);
        for(auto & element : this->get_data<T>()){
            //element.uri = prefix + ":" + element.uri;
            map[element.uri] = element.idx;
        }
    }


    /** Étant donné une liste d'indexes pointant vers source,
      * retourne une liste d'indexes pointant vers target
      */
    std::vector<idx_t> get_target_by_source(Type_e source, Type_e target, std::vector<idx_t> source_idx) const;

    /** Étant donné un index pointant vers source,
      * retourne une liste d'indexes pointant vers target
      */
    std::vector<idx_t> get_target_by_one_source(Type_e source, Type_e target, idx_t source_idx) const ;

    /** Retourne la structure de données associée au type */
    template<typename T>  std::vector<T> & get_data();
    template<typename T>  std::vector<T> const & get_data() const;

    /** Retourne tous les indices d'un type donné
      *
      * Concrètement, on a un tableau avec des éléments allant de 0 à (n-1) où n est le nombre d'éléments
      */
    std::vector<idx_t> get_all_index(Type_e type) const;


    /** Construit une nouvelle structure de correspondance */
    void build_connections();

    /** Retourne la correspondance entre deux journey_pattern point
     *
     * Cela peut varier pour les correspondances garanties ou les prolongements de service
     * Dans le cas normal, il s'agit juste du temps de sécurité
    */
    Connection journey_pattern_point_connection(idx_t, idx_t){
        Connection result;
        result.duration = 120;
        result.connection_type = eStopPointConnection;
        return result;
    }

    PT_Data& operator=(PT_Data&& other);

};

}}
