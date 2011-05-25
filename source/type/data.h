#pragma once
#include "type.h"
#include "indexes.h"
#include "fusion_vector_serialize.h"
#include "boost/serialization/array.hpp"

namespace navitia { namespace type {
class Data{
public:
    std::vector<ValidityPattern> validity_patterns;
    std::vector<Line> lines;
    std::vector<Route> routes;
    std::vector<VehicleJourney> vehicle_journeys;
    std::vector<StopPoint> stop_points;
    std::vector<StopArea> stop_areas;
    std::vector<StopTime> stop_times;

    std::vector<Network> networks;

    public:
    /** Fonction qui permet de sérialiser (aka binariser la structure de données
      *
      * Elle est appelée par boost et pas directement
      */
    template<class Archive> void serialize(Archive & ar, const unsigned int ) {
        ar & validity_patterns & lines & stop_points & stop_areas & stop_times & routes
            & vehicle_journeys ;
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

    /** Retrouve un élément par un attribut arbitraire
      *
      * exemple : find<StopPoint>.find(&StopPoint::name, "Gare du nord")
      */
    template<class RequestedType, class AttributeType>
    std::vector<RequestedType*> find(AttributeType RequestedType::* attribute, AttributeType str){
        std::vector<RequestedType *> result;
        BOOST_FOREACH(RequestedType & item, get<RequestedType>()){
            if(item.*attribute == str){
                result.push_back(&item);
            }
        }
        return result;
    }

    /** Sauvegarde la structure de fichier au format texte
      *
      * Le format est plus portable que la version binaire
      */
    void save(const std::string & filename);

    /** Charge la structure de données du fichier au format texte */
    void load(const std::string & filename);
    void load_lz(const std::string & filename);

    /** Sauvegarde la structure de fichier au format binaire
      *
      * Attention à la portabilité
      */
    void save_bin(const std::string & filename);

    /** Charge la structure de données depuis un fichier au format binaire */
    void load_bin(const std::string & filename);
};

} } //namespace navitia::type
