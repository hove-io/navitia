#pragma once
#include "pt_data.h"
#include <boost/serialization/version.hpp>
#include <boost/thread/shared_mutex.hpp>
#include "street_network/types.h"

namespace navitia { namespace type {

/** Contient toutes les données théoriques du référentiel transport en communs
  *
  * Il existe trois formats de stockage : texte, binaire, binaire compressé
  * Il est conseillé de toujours utiliser le format compressé (la compression a un surcoût quasiment nul et
  * peut même (sur des disques lents) accélerer le chargement).
  */
class Data{
public:

    static const unsigned int data_version = 3; //< Numéro de la version. À incrémenter à chaque que l'on modifie les données sérialisées
    int nb_threads; //< Nombre de threads. IMPORTANT ! Sans cette variable, ça ne compile pas
    unsigned int version; //< Numéro de version des données chargées
    bool loaded; //< Est-ce que lse données ont été chargées

    // Référentiels de données

    /// Référentiel de transport en commun
    PT_Data pt_data;

    ///streetnetwork
    navitia::streetnetwork::StreetNetwork street_network;

    boost::shared_mutex load_mutex;

    friend class boost::serialization::access;
    public:

    Data() : nb_threads(8), loaded(false){}
    /** Fonction qui permet de sérialiser (aka binariser la structure de données
      *
      * Elle est appelée par boost et pas directement
      */
    template<class Archive> void serialize(Archive & ar, const unsigned int version) {
        this->version = version;
        if(this->version != data_version){
            std::cerr << "Attention le fichier de données est à la version " << version << " (version actuelle : " << data_version << ")" << std::endl;
        }

        ar & pt_data & street_network;
    }

    /** Sauvegarde la structure de fichier au format texte
      *
      * Le format est plus portable que la version binaire
      */
    void save(const std::string & filename);

    /** Charge la structure de données du fichier au format texte */
    void load(const std::string & filename);

    /** Charge les données binaires compressées en FastLZ
      *
      * La compression FastLZ est extrèmement rapide mais moyennement performante
      * Le but est que la lecture du fichier compression soit aussi rapide que sans compression
      */
    void load_flz(const std::string & filename);

    /** Sauvegarde les données en binaire compressé avec FastLZ*/
    void save_flz(const std::string & filename);

    /** Sauvegarde la structure de fichier au format binaire
      *
      * Attention à la portabilité
      */
    void save_bin(const std::string & filename);

    /** Charge la structure de données depuis un fichier au format binaire */
    void load_bin(const std::string & filename);

};


} } //namespace navitia::type

BOOST_CLASS_VERSION(navitia::type::Data, navitia::type::Data::data_version)
