#pragma once
#include "pt_data.h"
#include <boost/serialization/version.hpp>
#include <boost/thread/shared_mutex.hpp>
//#include "street_network/types.h"
#include "georef/georef.h"
#include "utils/logger.h"
#include "utils/configuration.h"
#include "boost/utility.hpp"
#include "meta_data.h"
#include <boost/format.hpp>
#include "routing/dataraptor.h"
#include <atomic>
#include "georef/adminref.h"

namespace navitia { namespace type {

/** Contient toutes les données théoriques du référentiel transport en communs
  *
  * Il existe trois formats de stockage : texte, binaire, binaire compressé
  * Il est conseillé de toujours utiliser le format compressé (la compression a un surcout quasiment nul et
  * peut même (sur des disques lents) accélerer le chargement).
  */
class Data : boost::noncopyable{
public:

    static const unsigned int data_version = 19; //< Numéro de la version. À incrémenter à chaque que l'on modifie les données sérialisées
    int nb_threads; //< Nombre de threads. IMPORTANT ! Sans cette variable, ça ne compile pas
    unsigned int version; //< Numéro de version des données chargées
    std::atomic<bool> loaded; //< Est-ce que lse données ont été chargées

    MetaData meta;

    // Référentiels de données

    /// Référentiel de transport en commun
    PT_Data pt_data;


    navitia::georef::GeoRef geo_ref;

    /// Données précalculées pour le raptor
    routing::dataRAPTOR dataRaptor;

    /// Fixe les villes des voiries du filaire
    // les admins des objets
    void set_admins();

    /// Mutex servant à protéger le load des données
    boost::shared_mutex load_mutex;

    friend class boost::serialization::access;

    bool last_load;
    boost::posix_time::ptime last_load_at;

    std::atomic<bool> to_load;


    /// Constructeur de data, définit le nombre de threads, charge les données
    Data() : nb_threads(8), version(0), loaded(false), last_load(true), to_load(true){
        if(Configuration::is_instanciated()){
            init_logger();
            log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
            LOG4CPLUS_TRACE(logger, "Chargement de l'application");
            Configuration * conf = Configuration::get();
            std::string conf_file = conf->get_string("path") + conf->get_string("application") + ".ini";
            LOG4CPLUS_TRACE(logger, "On tente de charger le fichier de configuration pour les logs : " + conf_file);
            init_logger(conf_file);
            LOG4CPLUS_TRACE(logger, "On tente de charger le fichier de configuration général : " + conf_file);
            conf->load_ini(conf_file);
            nb_threads = conf->get_as<int>("GENERAL", "nb_threads", 1);
            geo_ref.word_weight =  conf->get_as<int>("AUTOCOMPLETE", "wordweight", 5);
        }
    }

    /** Fonction qui permet de sérialiser (aka binariser la structure de données
      *
      * Elle est appelée par boost et pas directement
      */
    template<class Archive> void serialize(Archive & ar, const unsigned int version) {
        this->version = version;
        if(this->version != data_version){
            log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
            unsigned int v = data_version;//sinon ca link pas...
            LOG4CPLUS_WARN(logger, boost::format("Attention le fichier de données est à la version %u (version actuelle : %d)") % version % v);
        }

        ar & pt_data & geo_ref & meta;
    }

    /** Charge les données et effectue les initialisations nécessaires */
    bool load(const std::string & filename);

    /** Sauvegarde les données */
    void save(const std::string & filename);

    /** Construit l'indexe ExternelCode */
    void build_uri();

    /** Construit l'indexe Autocomplete */
    void build_autocomplete();

    /** Construit l'indexe ProximityList */
    void build_proximity_list();

    /** Construit les données raptor */
    void build_raptor();

    Data& operator=(Data&& other);

private:
    /** Charge les données binaires compressées en LZ4
      *
      * La compression LZ4 est extrèmement rapide mais moyennement performante
      * Le but est que la lecture du fichier compression soit aussi rapide que sans compression
      */
    void load_lz4(const std::string & filename);

    /** Sauvegarde les données en binaire compressé avec LZ4*/
    void save_lz4(const std::string & filename);

};


} } //namespace navitia::type

BOOST_CLASS_VERSION(navitia::type::Data, navitia::type::Data::data_version)
