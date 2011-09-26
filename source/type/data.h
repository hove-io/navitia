#pragma once
#include "type.h"

namespace navitia { namespace type {

class Data{
public:
    int nb_threads; /// Nombre de threads. IMPORTANT ! Sans cette variable, ça ne compile pas
    bool loaded;
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
    public:

    Data() : nb_threads(8), loaded(false){}
    /** Fonction qui permet de sérialiser (aka binariser la structure de données
      *
      * Elle est appelée par boost et pas directement
      */
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & validity_patterns & lines & stop_points & stop_areas & stop_times & routes
            & vehicle_journeys & route_points ;
    }

    /** Initialise tous les indexes
      *
      * Les données doivent bien évidemment avoir été initialisés
      */
    void build_index();

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

    /** Sauvegarde la structure de fichier au format texte
      *
      * Le format est plus portable que la version binaire
      */
    void save(const std::string & filename);

    /** Charge la structure de données du fichier au format texte */
    void load(const std::string & filename);
    void load_flz(const std::string & filename);
    void save_flz(const std::string & filename);

    /** Sauvegarde la structure de fichier au format binaire
      *
      * Attention à la portabilité
      */
    void save_bin(const std::string & filename);

    /** Charge la structure de données depuis un fichier au format binaire */
    void load_bin(const std::string & filename);

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

    std::vector<idx_t> get_all_index(Type_e type);

};

} } //namespace navitia::type
