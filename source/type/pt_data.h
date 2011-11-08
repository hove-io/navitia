#include "type.h"
#include "first_letter/first_letter.h"
#include "proximity_list/proximity_list.h"

#include <boost/serialization/map.hpp>

namespace navitia { namespace type {

struct PT_Data{
    std::vector<ValidityPattern> validity_patterns;
    std::vector<Line> lines;
    std::vector<Route> routes;
    std::vector<VehicleJourney> vehicle_journeys;
    std::vector<StopPoint> stop_points;
    std::vector<StopArea> stop_areas;
    std::vector<StopTime> stop_times;

    std::vector<Network> networks;
    std::vector<Mode> modes;
    std::vector<ModeType> mode_types;
    std::vector<City> cities;
    std::vector<Connection> connections;
    std::vector<RoutePoint> route_points;

    std::vector<District> districts;
    std::vector<Department> departments;
    std::vector<Company> companies;
    std::vector<Vehicle> vehicles;
    std::vector<Country> countries;


    // First letter
    FirstLetter<idx_t> stop_area_first_letter;
    FirstLetter<idx_t> city_first_letter;

    // Proximity list
    ProximityList<idx_t> stop_area_proximity_list;
    ProximityList<idx_t> stop_point_proximity_list;
    ProximityList<idx_t> city_proximity_list;

    // Maps pour external code
    // À refléchir si un hash_map ne serait pas mieux ; pas forcément en lecture car hasher une chaîne c'est plus long que comparer
    // En attendant on utilise std::map car on sait le sérialiser...
    typedef std::map<std::string, idx_t> ExtCodeMap;
    ExtCodeMap line_map;
    ExtCodeMap route_map;
    ExtCodeMap vehicle_journey_map;
    ExtCodeMap stop_area_map;
    ExtCodeMap stop_point_map;
    ExtCodeMap network_map;
    ExtCodeMap mode_map;
    ExtCodeMap mode_type_map;
    ExtCodeMap city_map;
    ExtCodeMap district_map;
    ExtCodeMap department_map;
    ExtCodeMap company_map;
    ExtCodeMap country_map;

    /** Fonction qui permet de sérialiser (aka binariser la structure de données
      *
      * Elle est appelée par boost et pas directement
      */
    template<class Archive> void serialize(Archive & ar, const unsigned int) {        
        ar
                // Les listes de données
                & validity_patterns & lines & stop_points & stop_areas & stop_times & routes
                & vehicle_journeys & route_points
                // Les firstLetter
                & stop_area_first_letter & city_first_letter
                // Les map d'externalcode
                & line_map & route_map & vehicle_journey_map & stop_area_map & stop_point_map
                & network_map & mode_map & mode_type_map & city_map & district_map & department_map
                & company_map & country_map
                // Les proximity list
                & stop_area_proximity_list & stop_point_proximity_list & city_proximity_list;
    }

    /** Initialise tous les indexes
      *
      * Les données doivent bien évidemment avoir été initialisés
      */
    void build_index();

    /** Construit l'indexe ExternelCode */
    void build_external_code();

    /** Construit l'indexe FirstLetter */
    void build_first_letter();

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

    /** Retourne le conteneur associé au type
      *
      * Cette fonction est surtout utilisée en interne
      */
    template<class Type> std::vector<Type> & get();


    /** Étant donné une liste d'indexes pointant vers source,
      * retourne une liste d'indexes pointant vers target
      */
    std::vector<idx_t> get_target_by_source(Type_e source, Type_e target, std::vector<idx_t> source_idx);

    /** Étant donné un index pointant vers source,
      * retourne une liste d'indexes pointant vers target
      */
    std::vector<idx_t> get_target_by_one_source(Type_e source, Type_e target, idx_t source_idx);

    /** Retourne la structure de données associée au type */
    template<Type_e E>  std::vector<typename boost::mpl::at<enum_type_map, boost::mpl::int_<E> >::type> & get_data();

    /** Retourne tous les indices d'un type donné */
    template<Type_e E> std::vector<idx_t> get_all_index() {
        size_t size = get_data<E>().size();
        std::vector<idx_t> indexes(size);
        for(size_t i=0; i < size; i++)
            indexes[i] = i;
        return indexes;
    }

    /** Retourne tous les indices d'un type donné
      *
      * Concrètement, on a un tableau avec des éléments allant de 0 à (n-1) où n est le nombre d'éléments
      */
    std::vector<idx_t> get_all_index(Type_e type);
};

}}
