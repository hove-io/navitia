#pragma once
#include "pt_data.h"
#include <boost/serialization/version.hpp>
#include <boost/thread/shared_mutex.hpp>
#include "street_network/types.h"
#include "utils/logger.h"
#include "utils/configuration.h"
#include "boost/utility.hpp"
#include "meta_data.h"

namespace navitia { namespace type {

/** Contient toutes les données théoriques du référentiel transport en communs
  *
  * Il existe trois formats de stockage : texte, binaire, binaire compressé
  * Il est conseillé de toujours utiliser le format compressé (la compression a un surcoût quasiment nul et
  * peut même (sur des disques lents) accélerer le chargement).
  */
class Data : boost::noncopyable{
public:

    static const unsigned int data_version = 3; //< Numéro de la version. À incrémenter à chaque que l'on modifie les données sérialisées
    int nb_threads; //< Nombre de threads. IMPORTANT ! Sans cette variable, ça ne compile pas
    unsigned int version; //< Numéro de version des données chargées
    bool loaded; //< Est-ce que lse données ont été chargées

    MetaData meta;

    /** Le map qui contient la liste des Alias utilisé par
          * 1. module de binarisation des données ou le module de chargement.
            2. Module firstletter à chaque appel.
          */
    std::map<std::string, std::string> Alias_List;

    // Référentiels de données

    /// Référentiel de transport en commun
    PT_Data pt_data;

    /// streetnetwork
    navitia::streetnetwork::StreetNetwork street_network;

    /// Fixe les villes des voiries du filaire
    void set_cities();

    /// Mutex servant à protéger le load des données
    boost::shared_mutex load_mutex;

    friend class boost::serialization::access;
    public:

    /// Constructeur de data, définit le nombre de threads, charge les données
    Data() : nb_threads(8), loaded(false){
        if(Configuration::is_instanciated()){
            init_logger();
            log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
            LOG4CPLUS_INFO(logger, "Chargement de l'application");
            Configuration * conf = Configuration::get();
            nb_threads = conf->get_as<int>("GENERAL", "nb_threads", 1);
            std::string conf_file = conf->get_string("path") + conf->get_string("application") + ".ini";
            LOG4CPLUS_INFO(logger, "On tente de charger le fichier de configuration pour les logs : " + conf_file);
            init_logger(conf_file);
            LOG4CPLUS_INFO(logger, "On tente de charger le fichier de configuration général : " + conf_file);
            conf->load_ini(conf_file);
        }
    }

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
    void load_lz4(const std::string & filename);

    /** Sauvegarde les données en binaire compressé avec FastLZ*/
    void lz4(const std::string & filename);

    /** Sauvegarde la structure de fichier au format binaire
      *
      * Attention à la portabilité
      */
    void save_bin(const std::string & filename);

    /** Charge la structure de données depuis un fichier au format binaire */
    void load_bin(const std::string & filename);

    /** Construit l'indexe ExternelCode */
    void build_external_code();

    /** Construit l'indexe FirstLetter */
    void build_first_letter();

    /** Construit l'indexe ProximityList */
    void build_proximity_list();


};


} } //namespace navitia::type

BOOST_CLASS_VERSION(navitia::type::Data, navitia::type::Data::data_version)
