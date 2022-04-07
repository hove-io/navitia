/* Copyright © 2001-2022, Hove and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Hove (www.hove.com).
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
channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#pragma once
#include "type/validity_pattern.h"
#include "type/address_from_ntfs.h"
#include "data_exceptions.h"
#include "utils/obj_factory.h"
#include "utils/ptime.h"
#include "type/fwd_type.h"

#include <boost/serialization/split_member.hpp>
#include <boost/utility.hpp>
#include <boost/serialization/version.hpp>
#include <boost/format.hpp>
#include <boost/optional.hpp>

#include <atomic>
#include <set>

// workaround missing "is_trivially_copyable" in g++ < 5.0
#if __GNUG__ && __GNUC__ < 5
#define IS_TRIVIALLY_COPYABLE(T) __has_trivial_copy(T)
#else
#define IS_TRIVIALLY_COPYABLE(T) std::is_trivially_copyable<T>::value
#endif

namespace navitia {
namespace type {

template <typename T>
struct ContainerTrait {
    using vect_type = std::vector<T*>;
    using associative_type = std::unordered_map<std::string, T*>;
};

// specialization for impact
// Instead of pure pointer, we can only get a weak_ptr when requesting impacts
template <>
struct ContainerTrait<type::disruption::Impact> {
    using vect_type = std::vector<boost::weak_ptr<type::disruption::Impact> >;
    // for impacts, we don't want to have a map, we use the vector as the associative_type
    using associative_type = vect_type;
};

// specialization for StopPointConnection, there is no map too
template <>
struct ContainerTrait<type::StopPointConnection> {
    using vect_type = std::vector<type::StopPointConnection*>;
    using associative_type = vect_type;
};

template <>
struct ContainerTrait<navitia::georef::POIType> {
    using vect_type = std::vector<navitia::georef::POIType*>;
    using associative_type = std::map<std::string, navitia::georef::POIType*>;
};
template <>
struct ContainerTrait<navitia::georef::POI> {
    using vect_type = std::vector<navitia::georef::POI*>;
    using associative_type = std::map<std::string, navitia::georef::POI*>;
};
template <>
struct ContainerTrait<navitia::routing::JourneyPattern> {
    using vect_type = std::vector<navitia::routing::JourneyPattern*>;
    using associative_type = vect_type;
};
template <>
struct ContainerTrait<navitia::routing::JourneyPatternPoint> {
    using vect_type = std::vector<navitia::routing::JourneyPatternPoint*>;
    using associative_type = vect_type;
};
// specialization for meta-vj
// Instead of vector, we can only get an objFactory when requesting meta-vj
template <>
struct ContainerTrait<type::MetaVehicleJourney> {
    using vect_type = ObjFactory<MetaVehicleJourney>;
    using associative_type = vect_type;
};

/** Contains all the Public Transport Referential data (aka. PT-Ref), base-schedule and realtime.
 *
 * There are 3 storage formats : text, binary, compressed binary
 * It is advised to use the compressed binary format (compression has almost no additional overhead and can even
 * speed up loading from slow disks)
 */
class Data : boost::noncopyable {
    static_assert(IS_TRIVIALLY_COPYABLE(navitia::ptime),
                  "ptime isn't is_trivially_copyable and can't be used with std::atomic");
    mutable std::atomic<navitia::ptime> _last_rt_data_loaded;  // datetime of the last Real Time loaded data
public:
    static const unsigned int data_version;  //< Data version number. *INCREMENT* in cpp file
    unsigned int version = 0;                //< Version of loaded data
    std::atomic<bool> loaded{};              //< have the data been loaded ?
    std::atomic<bool> loading{};             //< Is the data being loaded
    std::atomic<bool> disruption_error;      // disruption error flag
    size_t data_identifier = 0;

    std::unique_ptr<MetaData> meta;

    // data referential

    // public transport (PT) referential
    std::unique_ptr<PT_Data> pt_data;

    std::unique_ptr<navitia::georef::GeoRef> geo_ref;

    // precomputed data for raptor (public transport routing algorithm)
    std::unique_ptr<navitia::routing::dataRAPTOR> dataRaptor;

    // Fare data
    std::unique_ptr<navitia::fare::Fare> fare;

    // functor to find admins
    std::function<std::vector<georef::Admin*>(const GeographicalCoord&, georef::AdminRtree&)> find_admins;

    /** Return the vector containing all the objects of type T*/
    template <typename T>
    const typename ContainerTrait<T>::vect_type& get_data() const;
    template <typename T>
    typename ContainerTrait<T>::vect_type& get_data();

    template <typename T>
    const typename ContainerTrait<T>::associative_type& get_assoc_data() const;

    template <typename T>
    typename ContainerTrait<T>::vect_type get_data(const Indexes& indexes) const {
        typename ContainerTrait<T>::vect_type res;
        const auto& objs = get_data<T>();
        for (const auto& idx : indexes) {
            res.push_back(objs[idx]);
        }
        return res;
    }

    /** Returns all indexes of a given type
     *
     * In practice, returns an array of elements ranging from 0 to (n-1) where n is the number of elements
     */
    Indexes get_all_index(Type_e type) const;

    size_t get_nb_obj(Type_e type) const;

    /** Given a list of indexes of 'source' objects
     * returns a list of indexes of 'target' objects
     */
    std::set<idx_t> get_target_by_source(Type_e source, Type_e target, const std::set<idx_t>& source_idx) const;

    /** Given one index of a 'source' object
     * returns a list of indexes of 'target' objects
     */
    Indexes get_target_by_one_source(Type_e source, Type_e target, idx_t source_idx) const;

    bool last_load_succeeded;
    // UTC
    boost::posix_time::ptime last_load_at;

    // This object is the only field mutated in this object. As it is
    // thread safe to mutate it, we mark it as mutable.  Maybe we can
    // find in the future a cleaner way, but now, this is cleaner than
    // before.
    mutable std::atomic<bool> is_connected_to_rabbitmq{};

    mutable std::atomic<bool> is_realtime_loaded{};

    Data(size_t data_identifier = 0);
    ~Data();

    friend class boost::serialization::access;
    template <class Archive>
    void save(Archive& ar, const unsigned int) const;
    template <class Archive>
    void load(Archive& ar, const unsigned int version);
    BOOST_SERIALIZATION_SPLIT_MEMBER()

    // Loading methods
    void load_nav(const std::string& filename);
    void load_disruptions(const std::string& database,
                          int chaos_batch_size,
                          const std::vector<std::string>& contributors = {});
    void build_raptor(size_t cache_size = 10);

    void warmup(const Data& other);

    /** Save data */
    void save(const std::string& filename) const;

    /** Build ExternalCode index */
    void build_uri();

    /** Build Autocomplete index */
    void build_autocomplete();
    void build_autocomplete_partial();

    /** Build ProximityList index */
    void build_proximity_list();
    /** Set admins*/
    void build_administrative_regions();

    void aggregate_odt();
    void build_relations();

    void build_grid_validity_pattern();

    void complete();
    void fill_stop_point_address(const std::unordered_map<std::string, navitia::type::Address*>& address_by_address_id =
                                     std::unordered_map<std::string, navitia::type::Address*>());
    /** For some pt object we compute the label */
    void compute_labels();

    /** Return the type of the id provided */
    Type_e get_type_of_id(const std::string& id) const;

    /** Load data from a binary file compressed using LZ4
     *
     * LZ4 compression is super fast but its efficiency is average
     * The goal is to achieve the same read performance with and without compression
     */
    void load(std::istream& ifs);

    /** Save data in a compressed binary file using LZ4*/
    void save(std::ostream& ofs) const;

    // Deep clone from the given Data.
    void clone_from(const Data&);

    void set_last_rt_data_loaded(const boost::posix_time::ptime&) const;
    const boost::posix_time::ptime last_rt_data_loaded() const;

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

}  // namespace type
}  // namespace navitia
