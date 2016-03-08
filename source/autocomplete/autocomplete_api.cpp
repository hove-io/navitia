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
#include "type/pt_data.h"
#include "utils/functions.h"

namespace navitia { namespace autocomplete {

static void create_place_pb(const std::vector<Autocomplete<nt::idx_t>::fl_quality>& result,
                            const nt::Type_e type,
                            uint32_t depth,
                            const nt::Data& data,
                            navitia::PbCreator& pb_creator){
    for(auto result_item : result){
        pbnavitia::PtObject* place = pb_creator.add_places();
        switch(type){
        case nt::Type_e::StopArea:
            pb_creator.fill(data.pt_data->stop_areas[result_item.idx], place, depth);
            place->set_quality(result_item.quality);
            place->set_score(result_item.score);
            break;
        case nt::Type_e::Admin:
            pb_creator.fill(data.geo_ref->admins[result_item.idx], place, depth);
            place->set_quality(result_item.quality);
            place->set_score(result_item.score);
            break;
        case nt::Type_e::StopPoint:
            pb_creator.fill(data.pt_data->stop_points[result_item.idx], place, depth);
            place->set_quality(result_item.quality);
            place->set_score(result_item.score);
            break;
        case nt::Type_e::Address:{
            const auto& way_coord = navitia::WayCoord(data.geo_ref->ways[result_item.idx],
                    result_item.coord, result_item.house_number);
            pb_creator.fill(&way_coord, place, depth);
            place->set_quality(result_item.quality);
            place->set_score(result_item.score);
            break;
        }
        case nt::Type_e::POI:
            pb_creator.fill(data.geo_ref->pois[result_item.idx], place, depth);
            place->set_quality(result_item.quality);
            place->set_score(result_item.score);
            break;
        case nt::Type_e::Network:
            pb_creator.fill(data.pt_data->networks[result_item.idx], place, depth);
            break;
        case nt::Type_e::CommercialMode:
            pb_creator.fill(data.pt_data->commercial_modes[result_item.idx], place, depth);
            break;
        case nt::Type_e::Line:
            pb_creator.fill(data.pt_data->lines[result_item.idx], place, depth);
            break;
        case nt::Type_e::Route:
            pb_creator.fill(data.pt_data->routes[result_item.idx], place, depth);
            break;
        default:
            break;
        }
    }
}

static int get_embedded_type_order(pbnavitia::NavitiaType type){
    switch(type){
    case pbnavitia::NETWORK:
        return 1;
    case pbnavitia::COMMERCIAL_MODE:
        return 2;
    case pbnavitia::ADMINISTRATIVE_REGION:
        return 3;
    case pbnavitia::STOP_AREA:
        return 4;
    case pbnavitia::POI:
        return 5;
    case pbnavitia::ADDRESS:
        return 6;
    case pbnavitia::LINE:
        return 7;
    case pbnavitia::ROUTE:
        return 8;
    default:
        return 9;
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


pbnavitia::Response autocomplete(const std::string &q,
                                 const std::vector<nt::Type_e> &filter,
                                 uint32_t depth,
                                 int nbmax,
                                 const std::vector<std::string> &admins,
                                 int search_type,
                                 const navitia::type::Data &d) {

    navitia::PbCreator pb_creator(d, pt::not_a_date_time, null_time_period);
    if (q.empty()) {
        pb_creator.fill_pb_error(pbnavitia::Error::bad_filter, "Autocomplete : value of q absent");
        return pb_creator.get_response();
    }
    int nbmax_temp = nbmax;
    //For each object type we search in the dictionnary and keep (nbmax x 3) objects in the result.
    //It's always better to get more objects from the disctionnary and apply some rules to delete
    //unwanted objects.
    nbmax = nbmax * 10;
    //bool addType = d.pt_data->stop_area_autocomplete.is_address_type(q, d.geo_ref->synonyms);
    std::vector<const georef::Admin*> admin_ptr = admin_uris_to_admin_ptr(admins, d);

    //Compute number of words in the query:
    std::set<std::string> query_word_vec = d.geo_ref->fl_admin.tokenize(q, d.geo_ref->ghostwords);

    ///Find max(100, count) éléments for each pt_object
    for(nt::Type_e type : filter) {
        std::vector<Autocomplete<nt::idx_t>::fl_quality> result;
        switch(type){
        case nt::Type_e::StopArea:
            if (search_type==0) {
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

        //Compute quality based on difference of word count in the result and the query
        if (search_type == 0) {
            update_quality(result, query_word_vec.size());
        }
        create_place_pb(result, type, depth, d, pb_creator);
    }

    //If n-gram is used to get de result we base on quality computed
    //in the dictionnary to delete unwanted objects and re-sort the final result
    auto compare_by_quality = [](pbnavitia::PtObject a, pbnavitia::PtObject b){
        if (a.quality() == b.quality()){
            return boost::algorithm::lexicographical_compare(a.name(), b.name(), boost::is_iless());
        }
        else {
            return a.quality() > b.quality();
        }
    };

    if (search_type != 0) {
        nbmax = nbmax_temp;
        auto mutable_places = pb_creator.get_mutable_places();
        sort_and_truncate(*mutable_places, nbmax, compare_by_quality);
    }


    //Sort the list of objects (sort by object type , score, quality and name)
    //delete unwanted objects at the end of the list
    auto compare_attributs = [](pbnavitia::PtObject a, pbnavitia::PtObject b)->bool {
        //Sort by object type
        if (a.embedded_type() != b.embedded_type()){
            const auto a_order = get_embedded_type_order(a.embedded_type());
            const auto b_order = get_embedded_type_order(b.embedded_type());
            return  a_order < b_order;
        }
        if ((a.quality() != b.quality()) && (a.quality() == 100  || b.quality() == 100)) {
            return a.quality() > b.quality();
        }
        if (a.score() != b.score()) {
            return a.score() > b.score();
        }
        if (a.quality() != b.quality()) {
            return a.quality() > b.quality();
        }
        return boost::algorithm::lexicographical_compare(a.name(), b.name(), boost::is_iless());
    };

    nbmax = nbmax_temp;
    auto mutable_places = pb_creator.get_mutable_places();
    sort_and_truncate(*mutable_places, nbmax, compare_attributs);
    const int result_size = mutable_places->size();

    //Pagination
    pb_creator.make_paginate(result_size, 0, nbmax, result_size);
    return pb_creator.get_response();
}

}} //namespace navitia::autocomplete
