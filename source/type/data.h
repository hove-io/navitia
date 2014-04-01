#pragma once
#include "utils/logger.h"
#include "utils/configuration.h"
#include <boost/utility.hpp>
#include <boost/serialization/version.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/format.hpp>
#include <atomic>
#include "type/type.h"
#include "utils/serialization_unique_ptr.h"

//forward declare
namespace navitia{
    namespace georef{
        class GeoRef;
    }
    namespace fare{
        class Fare;
    }
    namespace routing{
        class dataRAPTOR;
    }
    namespace type{
        class MetaData;
    }
}

namespace navitia { namespace type {


/** Contient toutes les données théoriques du référentiel transport en communs
  *
  * Il existe trois formats de stockage : texte, binaire, binaire compressé
  * Il est conseillé de toujours utiliser le format compressé (la compression a un surcout quasiment nul et
  * peut même (sur des disques lents) accélerer le chargement).
  */
class Data : boost::noncopyable{
public:

    static const unsigned int data_version = 20; //< Numéro de la version. À incrémenter à chaque que l'on modifie les données sérialisées
    unsigned int version = 0; //< Numéro de version des données chargées
    std::atomic<bool> loaded; //< Est-ce que lse données ont été chargées

    std::unique_ptr<MetaData> meta;

    // Référentiels de données

    /// Référentiel de transport en commun
    std::unique_ptr<PT_Data> pt_data;

    std::unique_ptr<navitia::georef::GeoRef> geo_ref;

    /// Données précalculées pour le raptor
    std::unique_ptr<navitia::routing::dataRAPTOR> dataRaptor;

    /// Fare data
    std::unique_ptr<navitia::fare::Fare> fare;

    /** Retourne la structure de données associée au type */
    /// TODO : attention aux perfs à faire la copie
    template<typename T> std::vector<T*> & get_data();
    template<typename T> std::vector<T*> get_data() const;

    /** Retourne tous les indices d'un type donné
      *
      * Concrètement, on a un tableau avec des éléments allant de 0 à (n-1) où n est le nombre d'éléments
      */
    std::vector<idx_t> get_all_index(Type_e type) const;


    /** Étant donné une liste d'indexes pointant vers source,
      * retourne une liste d'indexes pointant vers target
      */
    std::vector<idx_t> get_target_by_source(Type_e source, Type_e target, std::vector<idx_t> source_idx) const;

    /** Étant donné un index pointant vers source,
      * retourne une liste d'indexes pointant vers target
      */
    std::vector<idx_t> get_target_by_one_source(Type_e source, Type_e target, idx_t source_idx) const ;


    /// Fixe les villes des voiries du filaire
    // les admins des objets
//    void set_admins();

    /// Mutex servant à protéger le load des données
    boost::shared_mutex load_mutex;

    friend class boost::serialization::access;

    bool last_load = true;
    boost::posix_time::ptime last_load_at;
    std::atomic<bool> is_connected_to_rabbitmq;

    Data();
    ~Data();

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

        ar & pt_data & geo_ref & meta & fare;
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
    /** Set admins*/
    void build_administrative_regions();
    /** Construit les données raptor */
    void build_raptor();
    /** Construction des validity pattern des StopTimes **/
    void build_midnight_interchange();

    void build_associated_calendar();

    void build_grid_validity_pattern();
    void complete();
    Data& operator=(Data&& other);

    /** Retourne le type de l'id donné */
    Type_e get_type_of_id(const std::string & id) const;
private:
    /** Charge les données binaires compressées en LZ4
      *
      * La compression LZ4 est extrèmement rapide mais moyennement performante
      * Le but est que la lecture du fichier compression soit aussi rapide que sans compression
      */
    void load_lz4(const std::string & filename);

    /** Sauvegarde les données en binaire compressé avec LZ4*/
    void save_lz4(const std::string & filename);
    /** Recherche d'une ValidityPattern lors de clacul du passe-minuit**/
    ValidityPattern* get_or_create_validity_pattern(ValidityPattern* ref_validity_pattern, const uint32_t time);
    /** Get similar validitypattern **/
    ValidityPattern* get_similar_validity_pattern(ValidityPattern* vp) const;
};


/**
  * we want the resulting bit set that model the differences between
  * the calender validity pattern and the vj validity pattern.
  *
  * We want to limit this differences for the days the calendar is active.
  * we thus do a XOR to have the differences between the 2 bitsets and then do a AND on the calendar
  * to restrict those diff on the calendar
  */
template <size_t N>
std::bitset<N> get_difference(const std::bitset<N>& calendar, const std::bitset<N>& vj) {
    auto res = (calendar ^ vj) & calendar;
    return res;
}

std::vector<std::pair<const Calendar*, ValidityPattern::year_bitset>>
find_matching_calendar(const Data& data, const VehicleJourney* vehicle_journey, double relative_threshold = 0.1);

}} //namespace navitia::type

BOOST_CLASS_VERSION(navitia::type::Data, navitia::type::Data::data_version)
