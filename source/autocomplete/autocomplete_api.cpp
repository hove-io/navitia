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

#include "autocomplete_api.h"
#include "type/pb_converter.h"
#include "autocomplete/autocomplete.h"
#include "autocomplete/utils.h"
#include "utils/functions.h"
#include <algorithm>

namespace navitia { namespace autocomplete {

struct AutocompleteResult {
    type::Type_e type;
    Autocomplete<nt::idx_t>::fl_quality fl_result;

    AutocompleteResult(){}

    AutocompleteResult(const type::Type_e& type, const Autocomplete<nt::idx_t>::fl_quality& fl_result):
        type(type), fl_result(fl_result) {}
};

static void create_place_pb(const std::vector<AutocompleteResult>& result,
                            uint32_t depth,
                            const nt::Data& data,
                            navitia::PbCreator& pb_creator){
    for(const auto& r : result){
        pbnavitia::PtObject* place = pb_creator.add_places();
        place->set_quality(r.fl_result.quality);
        place->add_scores(std::get<0>(r.fl_result.scores));
        place->add_scores(std::get<1>(r.fl_result.scores));
        place->add_scores(std::get<2>(r.fl_result.scores));
        switch(r.type){
        case nt::Type_e::StopArea:
            pb_creator.fill(data.pt_data->stop_areas[r.fl_result.idx], place, depth);
            break;
        case nt::Type_e::Admin:
            pb_creator.fill(data.geo_ref->admins[r.fl_result.idx], place, depth);
            break;
        case nt::Type_e::StopPoint:
            pb_creator.fill(data.pt_data->stop_points[r.fl_result.idx], place, depth);
            break;
        case nt::Type_e::Address:{
            const auto& way_coord = navitia::WayCoord(data.geo_ref->ways[r.fl_result.idx],
                    r.fl_result.coord, r.fl_result.house_number);
            pb_creator.fill(&way_coord, place, depth);
            break;
        }
        case nt::Type_e::POI:
            pb_creator.fill(data.geo_ref->pois[r.fl_result.idx], place, depth);
            break;
        case nt::Type_e::Network:
            pb_creator.fill(data.pt_data->networks[r.fl_result.idx], place, depth);
            break;
        case nt::Type_e::CommercialMode:
            pb_creator.fill(data.pt_data->commercial_modes[r.fl_result.idx], place, depth);
            break;
        case nt::Type_e::Line:
            pb_creator.fill(data.pt_data->lines[r.fl_result.idx], place, depth);
            break;
        case nt::Type_e::Route:
            pb_creator.fill(data.pt_data->routes[r.fl_result.idx], place, depth);
            break;
        default:
            break;
        }
    }
}


namespace {
/*
 * An admin is not attached to itself, so for the admin we need to
 * explictly check if the wanted admin is not oneself
 */
template <typename T>
bool self_admin_check(const T*, const georef::Admin*) { return false; }

bool self_admin_check(const georef::Admin* obj, const georef::Admin* admin) {
    return obj->uri == admin->uri;
}

}

template<class T>
struct ValidAdminPtr {
    const std::vector<T*>& objects;
    const std::vector<const georef::Admin*> required_admins;

    ValidAdminPtr(const std::vector<T*> & objects,
            const std::vector<const georef::Admin*> required_admins) :
                objects(objects), required_admins(required_admins) {}

    bool operator()(type::idx_t idx) const {
        if (required_admins.empty()){
            return true;
        }
        const T* object = objects[idx];

        for(const georef::Admin* admin : required_admins) {
            const auto & admin_list = object->admin_list;
            if(std::find(admin_list.begin(), admin_list.end(), admin) != admin_list.end()) { return true; }
            if (self_admin_check(object, admin)) { return true; }
        }

        return false;
    }
};

template<class T>
ValidAdminPtr<T> valid_admin_ptr (const std::vector<T*> & objects,
        const std::vector<const georef::Admin*>& required_admins)  {
    return ValidAdminPtr<T>(objects, required_admins);
}


static std::vector<const georef::Admin*>
admin_uris_to_admin_ptr(const std::vector<std::string>& admin_uris,
                        const nt::Data& d){
    std::vector<const georef::Admin*> admins;
    for(auto admin_uri : admin_uris){
        for(const navitia::georef::Admin* admin : d.geo_ref->admins){
            if(admin_uri == admin->uri){
                admins.push_back(admin);
            }
        }
    }
    return admins;
}


//Penalty = (word count difference * 10)
//quality = quality (100) - penalty
static void update_quality(std::vector<Autocomplete<nt::idx_t>::fl_quality>& ac_result,
                           const int query_word_count) {
    for (auto &item: ac_result) {
        item.quality -= (item.nb_found - query_word_count) * 10;
    }
}

static std::vector<Autocomplete<nt::idx_t>::fl_quality> complete(const type::Data& d,
                                                                 const type::Type_e& type,
                                                                 const std::string& q,
                                                                 const std::vector<const georef::Admin*>& admin_ptr,
                                                                 size_t nbmax,
                                                                 int search_type){
    //TODO Refacto this ...
    std::vector<Autocomplete<nt::idx_t>::fl_quality> result;
    switch(type){
        case nt::Type_e::StopArea:
            if (search_type == 0) {
                result = d.pt_data->stop_area_autocomplete.find_complete(q,
                        nbmax, valid_admin_ptr(d.pt_data->stop_areas, admin_ptr), d.geo_ref->ghostwords);
            } else {
                result = d.pt_data->stop_area_autocomplete.find_partial_with_pattern(q,
                        d.geo_ref->word_weight,
                        nbmax, valid_admin_ptr(d.pt_data->stop_areas, admin_ptr), d.geo_ref->ghostwords);
            }
            break;
        case nt::Type_e::StopPoint:
            if (search_type==0) {
                result = d.pt_data->stop_point_autocomplete.find_complete(q,
                        nbmax, valid_admin_ptr(d.pt_data->stop_points, admin_ptr), d.geo_ref->ghostwords);
            } else {
                result = d.pt_data->stop_point_autocomplete.find_partial_with_pattern(q,
                        d.geo_ref->word_weight, nbmax,
                        valid_admin_ptr(d.pt_data->stop_points, admin_ptr), d.geo_ref->ghostwords);
            }
            break;
        case nt::Type_e::Admin:
            if (search_type==0) {
                result = d.geo_ref->fl_admin.find_complete(q,
                        nbmax, valid_admin_ptr(d.geo_ref->admins, admin_ptr), d.geo_ref->ghostwords);
            } else {
                result = d.geo_ref->fl_admin.find_partial_with_pattern(q,
                        d.geo_ref->word_weight,
                        nbmax, valid_admin_ptr(d.geo_ref->admins, admin_ptr), d.geo_ref->ghostwords);
            }
            break;
        case nt::Type_e::Address:
            result = d.geo_ref->find_ways(q, nbmax, search_type,
                    valid_admin_ptr(d.geo_ref->ways, admin_ptr), d.geo_ref->ghostwords);
            break;
        case nt::Type_e::POI:
            if (search_type==0) {
                result = d.geo_ref->fl_poi.find_complete(q,
                        nbmax, valid_admin_ptr(d.geo_ref->pois, admin_ptr), d.geo_ref->ghostwords);
            } else {
                result = d.geo_ref->fl_poi.find_partial_with_pattern(q,
                        d.geo_ref->word_weight, nbmax,
                        valid_admin_ptr(d.geo_ref->pois, admin_ptr), d.geo_ref->ghostwords);
            }
            break;
        case nt::Type_e::Network:
            if (search_type==0) {
                result = d.pt_data->network_autocomplete.find_complete(q,
                        nbmax, [](type::idx_t){return true;}, d.geo_ref->ghostwords);
            } else {
                result = d.pt_data->network_autocomplete.find_partial_with_pattern(q,
                        d.geo_ref->word_weight, nbmax,
                        [](type::idx_t){return true;}, d.geo_ref->ghostwords);
            }
            break;
        case nt::Type_e::CommercialMode:
            if (search_type==0) {
                result = d.pt_data->mode_autocomplete.find_complete(q,
                        nbmax, [](type::idx_t){return true;}, d.geo_ref->ghostwords);
            } else {
                result = d.pt_data->mode_autocomplete.find_partial_with_pattern(q,
                        d.geo_ref->word_weight, nbmax,
                        [](type::idx_t){return true;}, d.geo_ref->ghostwords);
            }
            break;
        case nt::Type_e::Line:
            if (search_type==0) {
                result = d.pt_data->line_autocomplete.find_complete(q,
                        nbmax, [](type::idx_t){return true;}, d.geo_ref->ghostwords);
            } else {
                result = d.pt_data->line_autocomplete.find_partial_with_pattern(q,
                        d.geo_ref->word_weight,
                        nbmax, [](type::idx_t){return true;}, d.geo_ref->ghostwords);
            }
            break;
        case nt::Type_e::Route:
            if (search_type==0) {
                result = d.pt_data->route_autocomplete.find_complete(q,
                        nbmax, [](type::idx_t){return true;}, d.geo_ref->ghostwords);
            } else {
                result = d.pt_data->route_autocomplete.find_partial_with_pattern(q,
                        d.geo_ref->word_weight,
                        nbmax, [](type::idx_t){return true;}, d.geo_ref->ghostwords);
            }
            break;
        default: break;
    }
    return result;
}

static std::string get_name(const type::Data& d, const type::Type_e& type, const type::idx_t& idx){
    switch(type){
        case nt::Type_e::StopArea:
            return d.pt_data->stop_areas[idx]->name;
        case nt::Type_e::StopPoint:
            return d.pt_data->stop_points[idx]->name;
        case nt::Type_e::Admin:
            return d.geo_ref->admins[idx]->name;
        case nt::Type_e::Address:
            return d.geo_ref->ways[idx]->name;
        case nt::Type_e::POI:
            return d.geo_ref->pois[idx]->name;
        case nt::Type_e::Network:
            return d.pt_data->networks[idx]->name;
        case nt::Type_e::CommercialMode:
            return d.pt_data->commercial_modes[idx]->name;
        case nt::Type_e::Line:
            return d.pt_data->lines[idx]->name;
        case nt::Type_e::Route:
            return d.pt_data->routes[idx]->name;
        default:
            return "";
    }
}


struct compare_by_quality {
    const type::Data& data;

    compare_by_quality(const type::Data& data) : data(data){}

    bool operator()(const AutocompleteResult& a, const AutocompleteResult& b) const{
        if (a.fl_result.quality == b.fl_result.quality){
            auto a_name = get_name(data, a.type, a.fl_result.idx);
            auto b_name = get_name(data, b.type, b.fl_result.idx);
            return boost::algorithm::lexicographical_compare(a_name, b_name, boost::is_iless());
        }else {
            return a.fl_result.quality > b.fl_result.quality;
        }
    }
};

struct compare_attributs {
    const type::Data& data;

    compare_attributs(const type::Data& data) : data(data){}

    bool operator()(const AutocompleteResult& a, const AutocompleteResult& b) const{
        //Sort by object type
        if (a.type != b.type){
            return get_type_e_order(a.type) < get_type_e_order(b.type);
        }
        if ((a.fl_result.quality != b.fl_result.quality) && (a.fl_result.quality == 100  || b.fl_result.quality == 100)) {
            return a.fl_result.quality > b.fl_result.quality;
        }
        if (std::get<0>(a.fl_result.scores) != std::get<0>(b.fl_result.scores)) {
            return std::get<0>(a.fl_result.scores) > std::get<0>(b.fl_result.scores);
        }
        if (std::get<1>(a.fl_result.scores) != std::get<1>(b.fl_result.scores)) {
            return std::get<1>(a.fl_result.scores) > std::get<1>(b.fl_result.scores);
        }
        if (std::get<2>(a.fl_result.scores) != std::get<2>(b.fl_result.scores)) {
            return std::get<2>(a.fl_result.scores) > std::get<2>(b.fl_result.scores);
        }

        if (a.fl_result.quality != b.fl_result.quality) {
            return a.fl_result.quality > b.fl_result.quality;
        }
        auto a_name = get_name(data, a.type, a.fl_result.idx);
        auto b_name = get_name(data, b.type, b.fl_result.idx);
        return boost::algorithm::lexicographical_compare(a_name, b_name, boost::is_iless());
    }
};


void autocomplete(navitia::PbCreator& pb_creator,
                  const std::string &q,
                  const std::vector<nt::Type_e>& filter,
                  uint32_t depth,
                  const int nbmax,
                  const std::vector<std::string> &admins,
                  int search_type,
                  const navitia::type::Data &d) {

    if (q.empty()) {
        pb_creator.fill_pb_error(pbnavitia::Error::bad_filter, "Autocomplete : value of q absent");
        return;
    }
    //For each object type we search in the dictionnary and keep (nbmax x 10) objects in the result.
    //It's always better to get more objects from the disctionnary and apply some rules to delete
    //unwanted objects.
    size_t nb_items_to_search = nbmax * 10;
    std::vector<const georef::Admin*> admin_ptr = admin_uris_to_admin_ptr(admins, d);

    //Compute number of words in the query:
    std::set<std::string> query_word_vec = d.geo_ref->fl_admin.tokenize(q, d.geo_ref->ghostwords);


    std::vector<AutocompleteResult> results;
    for(const auto group: build_type_groups(filter)){
        for(nt::Type_e type : group) {
            //search for candidate
            auto found = complete(d, type, q, admin_ptr, nb_items_to_search, search_type);
            //Compute quality based on difference of word count in the result and the query
            if (search_type == 0) {
                update_quality(found, query_word_vec.size());
            }
            for(const auto& r: found){
                results.push_back(AutocompleteResult(type, r));
            }
        }
        if(search_type == 0 && results.size() > size_t(nbmax)){
            //In searchtype==0 we can stop once we have found the number of desired results
            //we never ever return a object of a type that as a lower priority if an object with
            //a bigger priority has been returned by fl.
            break;
        }
    }
    //If n-gram is used to get the result we base on quality computed
    //in the dictionnary to delete unwanted objects and re-sort the final result
    if (search_type == 1) {
        sort_and_truncate(results, nbmax, compare_by_quality(d));
    }

    //Sort the list of objects (sort by object type , score, quality and name)
    //delete unwanted objects at the end of the list
    sort_and_truncate(results, nbmax, compare_attributs(d));

    create_place_pb(results, depth, d, pb_creator);
    auto mutable_places = pb_creator.get_mutable_places();
    const int result_size = mutable_places->size();
    pb_creator.make_paginate(result_size, 0, nbmax, result_size);
}

}} //namespace navitia::autocomplete
