#pragma once
#include "type.h"
#include "autocomplete/autocomplete.h"
#include "proximity_list/proximity_list.h"

#include <boost/serialization/map.hpp>

namespace navitia { namespace type {

struct PT_Data : boost::noncopyable{
    std::vector<ValidityPattern> validity_patterns;
    std::vector<Line> lines;
    std::vector<Route> routes;
    std::vector<VehicleJourney> vehicle_journeys;
    std::vector<StopPoint> stop_points;
    std::vector<StopArea> stop_areas;
    std::vector<StopTime> stop_times;

    std::vector<Network> networks;
    std::vector<PhysicalMode> physical_modes;
    std::vector<CommercialMode> commercial_modes;
    std::vector<City> cities;
    std::vector<Connection> connections;
    std::vector<RoutePointConnection> route_point_connections;
    std::vector<RoutePoint> route_points;

    std::vector<District> districts;
    std::vector<Department> departments;
    std::vector<Company> companies;
    std::vector<Vehicle> vehicles;
    std::vector<Country> countries;

    std::vector<std::vector<Connection> > stop_point_connections;

    // First letter
    autocomplete::Autocomplete<idx_t> stop_area_autocomplete;
    autocomplete::Autocomplete<idx_t> city_autocomplete;
    autocomplete::Autocomplete<idx_t> stop_point_autocomplete;

    // Proximity list
    proximitylist::ProximityList<idx_t> stop_area_proximity_list;
    proximitylist::ProximityList<idx_t> stop_point_proximity_list;
    proximitylist::ProximityList<idx_t> city_proximity_list;

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
    ExtCodeMap physical_mode_map;
    ExtCodeMap commercial_mode_map;
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
                & vehicle_journeys & route_points & commercial_modes & physical_modes & cities & networks
                // Les firstLetter
                & stop_area_autocomplete & city_autocomplete & stop_point_autocomplete
                // Les map d'externalcode
                & line_map & route_map & vehicle_journey_map & stop_area_map & stop_point_map
                & network_map & physical_mode_map & commercial_mode_map & city_map & district_map & department_map
                & company_map & country_map
                // Les proximity list
                & stop_area_proximity_list & stop_point_proximity_list & city_proximity_list
                & connections & stop_point_connections & route_point_connections;
    }

    /** Initialise tous les indexes
      *
      * Les données doivent bien évidemment avoir été initialisés
      */
    void build_index();

    /** Construit l'indexe ExternelCode */
    void build_uri();

    /** Construit l'indexe Autocomplete */
    void build_autocomplete();

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
        std::string prefix = static_data::get()->captionByType(T::type);
        for(auto & element : this->get_data<T>()){
            element.uri = prefix + ":" + element.uri;
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

    /** Retourne la correspondance entre deux route point
     *
     * Cela peut varier pour les correspondances garanties ou les prolongements de service
     * Dans le cas normal, il s'agit juste du temps de sécurité
    */
    Connection route_point_connection(idx_t, idx_t){
        Connection result;
        result.duration = 120;
        result.connection_type = eStopPointConnection;
        return result;
    }

    PT_Data& operator=(PT_Data&& other);

    int connection_duration(idx_t origin_route_point, idx_t destination_route_point){
        const RoutePoint & origin = route_points[origin_route_point];
        const RoutePoint & destination = route_points[destination_route_point];
        if(origin.stop_point_idx == destination.stop_point_idx)
            return 120;
        else
            return 180;
    }
};

}}
