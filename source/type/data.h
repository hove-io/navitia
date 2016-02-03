/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.
  
This file is part of Navitia,
    the software to build cool stuff with public transport.
 
Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
Help us simplify mobility and open public transport:
    a non ending quest to the responsive locomotion way of traveling!
  
LICENCE: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
   
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.
   
You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
  
Stay tuned using
twitter @navitia 
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#pragma once
#include "utils/logger.h"
#include <boost/utility.hpp>
#include <boost/serialization/version.hpp>
#include <boost/format.hpp>
#include <boost/optional.hpp>
#include <atomic>
#include "type/type.h"
#include "utils/serialization_unique_ptr.h"
#include "utils/serialization_atomic.h"
#include "utils/exception.h"
#include "utils/obj_factory.h"

//forward declare
namespace navitia{
    namespace georef{
        struct GeoRef;
    }
    namespace fare{
        struct Fare;
    }
    namespace routing{
        struct dataRAPTOR;
    }
    namespace type{
        struct MetaData;
    }
}

namespace navitia { namespace type {

struct wrong_version : public navitia::exception {
    wrong_version(const std::string& msg): navitia::exception(msg){}
    wrong_version(const wrong_version&) = default;
    wrong_version& operator=(const wrong_version&) = default;
    virtual ~wrong_version() noexcept;
};

template<typename T>
struct DataTraitHelper {
  typedef std::vector<T*> type;
};

// specialization for impact
// Instead of pure pointer, we can only get a weak_ptr when requesting impacts 
template<>
struct DataTraitHelper<type::disruption::Impact> {
  typedef std::vector<boost::weak_ptr<type::disruption::Impact>> type;
};

// specialization for meta-vj
// Instead of vector, we can only get an objFactory when requesting meta-vj
template<>
struct DataTraitHelper<type::MetaVehicleJourney> {
  typedef ObjFactory<MetaVehicleJourney> type;
};

/** Contient toutes les données théoriques du référentiel transport en communs
  *
  * Il existe trois formats de stockage : texte, binaire, binaire compressé
  * Il est conseillé de toujours utiliser le format compressé (la compression a un surcout quasiment nul et
  * peut même (sur des disques lents) accélerer le chargement).
  */
class Data : boost::noncopyable{
public:

    static const unsigned int data_version; //< Data version number. *INCREMENT* in cpp file
    unsigned int version = 0; //< Version of loaded data
    std::atomic<bool> loaded; //< have the data been loaded ?
    std::atomic<bool> loading; //< Is the data being loaded
    size_t data_identifier = 0;

    std::unique_ptr<MetaData> meta;

    // data referential

    /// public transport (PT) referential
    std::unique_ptr<PT_Data> pt_data;

    std::unique_ptr<navitia::georef::GeoRef> geo_ref;

    /// precomputed data for raptor (public transport routing algorithm)
    std::unique_ptr<navitia::routing::dataRAPTOR> dataRaptor;

    /// Fare data
    std::unique_ptr<navitia::fare::Fare> fare;

    // functor to find admins
    std::function<std::vector<georef::Admin*>(const GeographicalCoord&)> find_admins;

    /** Retourne la structure de données associée au type */
    template<typename T> const typename DataTraitHelper<T>::type& get_data() const;


//    template<typename T>
//    std::vector<typename DataTraitHelper<T>::type::value_type>
//    get_data(const std::vector<idx_t>& indexes) const
//    {
//        std::vector<typename DataTraitHelper<T>::type::value_type> res;
//        const auto& objs = get_data<T>();
//        for (const auto& idx: indexes) { res.push_back(objs[idx]); }
//        return res;
//    }

    template<typename T> typename DataTraitHelper<T>::type
    get_data(const std::vector<idx_t>& indexes) const {
        typename DataTraitHelper<T>::type res;
        const auto& objs = get_data<T>();
        for (const auto& idx: indexes) { res.push_back(objs[idx]); }
        return res;
    }
    /*
    template<> typename DataTraitHelper<T>::type
    get_data<Metavj>(const std::vector<idx_t>& indexes) const {
        typename std::vector<const MetaVj*> res;
        const auto& objs = get_data<T>();
        for (const auto& idx: indexes) { res.push_back(objs[idx]); }
        return res;
    }*/

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


    bool last_load = true;
    // UTC
    boost::posix_time::ptime last_load_at;

    boost::posix_time::ptime last_rt_data_loaded; //datetime of the last Real Time loaded data

    // This object is the only field mutated in this object. As it is
    // thread safe to mutate it, we mark it as mutable.  Maybe we can
    // find in the future a cleaner way, but now, this is cleaner than
    // before.
    mutable std::atomic<bool> is_connected_to_rabbitmq;

    mutable std::atomic<bool> is_realtime_loaded;

    Data(size_t data_identifier=0);
    ~Data();

    friend class boost::serialization::access;
    template<class Archive> void save(Archive & ar, const unsigned int) const {
        ar & pt_data & geo_ref & meta & fare & last_load_at & loaded & last_load & is_connected_to_rabbitmq
           & is_realtime_loaded;
    }
    template<class Archive> void load(Archive & ar, const unsigned int version) {
        this->version = version;
        if(this->version != data_version){
            unsigned int v = data_version;//sinon ca link pas...
            auto msg = boost::format("Warning data version don't match with the data version of kraken %u (current version: %d)") % version % v;
            throw wrong_version(msg.str());
        }
        ar & pt_data & geo_ref & meta & fare & last_load_at & loaded & last_load & is_connected_to_rabbitmq
           & is_realtime_loaded;
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()

    /** Charge les données et effectue les initialisations nécessaires */
    bool load(const std::string & filename,
            const boost::optional<std::string>& chaos_database = {},
            const std::vector<std::string>& contributors = {});

    /** Sauvegarde les données */
    void save(const std::string & filename) const;

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

    void build_associated_calendar();

    void aggregate_odt();
    void build_relations();

    void build_grid_validity_pattern();

    void complete();

    /** For some pt object we compute the label */
    void compute_labels();

    /** Retourne le type de l'id donné */

    Type_e get_type_of_id(const std::string & id) const;

    /** Charge les données binaires compressées en LZ4
      *
      * La compression LZ4 est extrèmement rapide mais moyennement performante
      * Le but est que la lecture du fichier compression soit aussi rapide que sans compression
      */
    void load(std::istream& ifs);

    /** Sauvegarde les données en binaire compressé avec LZ4*/
    void save(std::ostream& ifs) const;

    // Deep clone from the given Data.
    void clone_from(const Data&);
private:
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
find_matching_calendar(const Data&, const std::string& name, const ValidityPattern& validity_pattern,
                       const std::vector<Calendar*>& calendar_list, double relative_threshold = 0.1);


}} //namespace navitia::type
