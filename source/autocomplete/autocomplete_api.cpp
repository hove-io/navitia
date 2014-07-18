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

namespace navitia { namespace autocomplete {
/**
 * se charge de remplir l'objet protocolbuffer autocomplete passé en paramètre
 *
 */
void create_pb(const std::vector<Autocomplete<nt::idx_t>::fl_quality>& result,
               const nt::Type_e type, uint32_t depth, const nt::Data& data,
               pbnavitia::Response & pb_response){
    for(auto result_item : result){
        pbnavitia::Place* place = pb_response.add_places();
        switch(type){
        case nt::Type_e::StopArea:
            fill_pb_placemark(data.pt_data->stop_areas[result_item.idx], data, place, depth);
            place->set_quality(result_item.quality);
            break;
        case nt::Type_e::Admin:
            fill_pb_placemark(data.geo_ref->admins[result_item.idx], data, place, depth);
            place->set_quality(result_item.quality);
            break;
        case nt::Type_e::StopPoint:
            fill_pb_placemark(data.pt_data->stop_points[result_item.idx], data, place, depth);
            place->set_quality(result_item.quality);
            break;
        case nt::Type_e::Address:
            fill_pb_placemark(data.geo_ref->ways[result_item.idx], data, place, result_item.house_number, result_item.coord, depth);
            place->set_quality(result_item.quality);
            break;
        case nt::Type_e::POI:
            fill_pb_placemark(data.geo_ref->pois[result_item.idx], data, place, depth);
            place->set_quality(result_item.quality);
            break;
        case nt::Type_e::Line:
            fill_pb_object(data.pt_data->lines[result_item.idx], data,
                    place->mutable_line(), depth);
            place->set_name(data.pt_data->lines[result_item.idx]->name);
            place->set_uri(data.pt_data->lines[result_item.idx]->uri);
            place->set_quality(result_item.quality);
            place->set_embedded_type(pbnavitia::LINE);
        default:
            break;
        }
    }
}

int penalty_by_type(type::Type_e ntype, bool is_address_type) {
    // Ordre de tri :
    // Add, SA, POI, SP, Admin : si présence de addressType dans le recherche
    // Admin, SA, POI, Add, SP : si non
    if(is_address_type) {
        const std::map<type::Type_e, uint32_t> penalty_map =
        {
            {nt::Type_e::Admin, 8},
            {nt::Type_e::StopArea, 2},
            {nt::Type_e::POI, 4},
            {nt::Type_e::Address, 0},
            {nt::Type_e::StopPoint, 6},
        };
        auto result = penalty_map.find(ntype);
        return result == penalty_map.end() ? 0 : result->second;
    } else {
        const std::map<type::Type_e, uint32_t> penalty_map =
        {
            {nt::Type_e::Admin, 0},
            {nt::Type_e::StopArea, 2},
            {nt::Type_e::POI, 6},
            {nt::Type_e::Address, 8},
            {nt::Type_e::StopPoint, 4},
        };
        auto result = penalty_map.find(ntype);
        return result == penalty_map.end() ? 0 : result->second;
    }
}


///Mettre à jour la qualité sur le poids de POI
void update_quality_by_poi_type(std::vector<Autocomplete<nt::idx_t>::fl_quality>& ac_result,
        const navitia::type::Data &d){
    for(auto &item : ac_result){
        int poi_weight = 0;
        poi_weight = item.quality + d.geo_ref->pois[item.idx]->weight * 2;
        item.quality = std::min(poi_weight, 100);
    }
}

///Find out if the list of cities(admins) contains at least one city with level=8
bool has_admin_level8(const std::vector<georef::Admin*>& admins){
    for(const navitia::georef::Admin* admin : admins){
        if (admin->level == 8){
            return true;
        }
    }
    return false;
}


///Ajouter une pénalité aux objects sans admin par au moin d.geo_ref->word_weight
void update_quality_for_missing_admin(std::vector<Autocomplete<nt::idx_t>::fl_quality>& ac_result,
        const navitia::type::Data &d, navitia::type::Type_e ntype){
    const int penalty = d.geo_ref->word_weight * 8;
    for (auto &item : ac_result){
        bool apply_penalty = false;
        switch(ntype){
        case navitia::type::Type_e::StopArea:
            apply_penalty = !has_admin_level8(d.pt_data->stop_areas[item.idx]->admin_list);
            break;
        case navitia::type::Type_e::POI:
            apply_penalty = !has_admin_level8(d.geo_ref->pois[item.idx]->admin_list);
            break;
        case navitia::type::Type_e::Address:
            apply_penalty = !has_admin_level8(d.geo_ref->ways[item.idx]->admin_list);
            break;
        case navitia::type::Type_e::StopPoint:
            apply_penalty = !has_admin_level8(d.pt_data->stop_points[item.idx]->admin_list);
            break;
        default:
            break;
        }
        if(apply_penalty)
            item.quality -= penalty;
    }
}

void update_quality(std::vector<Autocomplete<nt::idx_t>::fl_quality>& ac_result, navitia::type::Type_e ntype,
                    bool Is_address_type,
                    const navitia::type::Data &d){
    //Mettre à jour la qualité sur la pénalité par type d'adresse
    const int penalty = penalty_by_type(ntype, Is_address_type);
    for(auto &item : ac_result) {
        item.quality -= penalty;
    }

    //Mettre à jour la qualité sur le poids de POI
    //Cette méthode n'est pas utilisé en absence du poids sur poi-type.
//    if (ntype ==navitia::type::Type_e::POI){
//        update_quality_by_poi_type(ac_result, d);
//    }

    //Ajouter une pénalité aux objects sans admin par au moin d.geo_ref->word_weight
    update_quality_for_missing_admin(ac_result, d, ntype);
}

int get_embedded_type_order(int n){
    switch(n){
    case pbnavitia::ADMINISTRATIVE_REGION:
        return 1;
        break;
     case pbnavitia::STOP_AREA:
        return 2;
        break;
    case pbnavitia::POI:
       return 3;
       break;
    case pbnavitia::ADDRESS:
       return 4;
       break;
    default:
       return 5;
       break;
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
        const T* object = objects[idx];
        if (required_admins.empty()){
            return true;
        }

        for(const georef::Admin* admin : required_admins) {
            const auto & admin_list = object->admin_list;
            if(std::find(admin_list.begin(), admin_list.end(), admin) != admin_list.end())
                return true;
        }

        return false;
    }
};

template<class T>
ValidAdminPtr<T> valid_admin_ptr (const std::vector<T*> & objects,
        const std::vector<const georef::Admin*>& required_admins)  {
    return ValidAdminPtr<T>(objects, required_admins);
}


std::vector<const georef::Admin*> admin_uris_to_admin_ptr(const std::vector<std::string> &admin_uris,
        const nt::Data &d){
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


pbnavitia::Response autocomplete(const std::string &q,
                                 const std::vector<nt::Type_e> &filter,
                                 uint32_t depth,
                                 int nbmax,
                                 const std::vector<std::string> &admins,
                                 int search_type,
                                 const navitia::type::Data &d) {

    pbnavitia::Response pb_response;
    if (q.length() == 0) {
        fill_pb_error(pbnavitia::Error::bad_filter, "Autocomplete : value of q absent", pb_response.mutable_error());
        return pb_response;
    }
    int nbmax_temp = nbmax;
    nbmax = std::max(100, nbmax);
    bool addType = d.pt_data->stop_area_autocomplete.is_address_type(q, d.geo_ref->synonyms);
    std::vector<const georef::Admin*> admin_ptr = admin_uris_to_admin_ptr(admins, d);

    ///Récupérer max(100, count) éléments pour chaque type d'ObjectTC
    for(nt::Type_e type : filter) {
        std::vector<Autocomplete<nt::idx_t>::fl_quality> result;
        switch(type){
        case nt::Type_e::StopArea:
            if (search_type==0) {
                result = d.pt_data->stop_area_autocomplete.find_complete(q,
                        d.geo_ref->synonyms, d.geo_ref->word_weight, nbmax,
                        valid_admin_ptr(d.pt_data->stop_areas, admin_ptr));
            } else {
                result = d.pt_data->stop_area_autocomplete.find_partial_with_pattern(q,
                        d.geo_ref->synonyms, d.geo_ref->word_weight,
                        nbmax, valid_admin_ptr(d.pt_data->stop_areas, admin_ptr));
            }
            break;
        case nt::Type_e::StopPoint:
            if (search_type==0) {
                result = d.pt_data->stop_point_autocomplete.find_complete(q,
                        d.geo_ref->synonyms, d.geo_ref->word_weight,
                        nbmax, valid_admin_ptr(d.pt_data->stop_points, admin_ptr));
            } else {
                result = d.pt_data->stop_point_autocomplete.find_partial_with_pattern(q,
                        d.geo_ref->synonyms, d.geo_ref->word_weight, nbmax,
                        valid_admin_ptr(d.pt_data->stop_points, admin_ptr));
            }
            break;
        case nt::Type_e::Admin:
            if (search_type==0) {
                result = d.geo_ref->fl_admin.find_complete(q,
                        d.geo_ref->synonyms, d.geo_ref->word_weight,
                        nbmax, valid_admin_ptr(d.geo_ref->admins, admin_ptr));
            } else {
                result = d.geo_ref->fl_admin.find_partial_with_pattern(q,
                        d.geo_ref->synonyms, d.geo_ref->word_weight,
                        nbmax, valid_admin_ptr(d.geo_ref->admins, admin_ptr));
            }
            break;
        case nt::Type_e::Address:
            result = d.geo_ref->find_ways(q, nbmax, search_type,
                    valid_admin_ptr(d.geo_ref->ways, admin_ptr));
            break;
        case nt::Type_e::POI:
            if (search_type==0) {
                result = d.geo_ref->fl_poi.find_complete(q,
                        d.geo_ref->synonyms, d.geo_ref->word_weight,
                        nbmax, valid_admin_ptr(d.geo_ref->pois, admin_ptr));
            } else {
                result = d.geo_ref->fl_poi.find_partial_with_pattern(q, /*d.geo_ref->alias,*/
                        d.geo_ref->synonyms, d.geo_ref->word_weight, nbmax,
                        valid_admin_ptr(d.geo_ref->pois, admin_ptr));
            }
            break;
        case nt::Type_e::Line:
            if (search_type==0) {
                result = d.pt_data->line_autocomplete.find_complete(q,
                        d.geo_ref->synonyms, d.geo_ref->word_weight,
                        nbmax, [](type::idx_t){return true;});
            } else {
                result = d.pt_data->line_autocomplete.find_complete(q, /*d.geo_ref->alias,*/
                        d.geo_ref->synonyms, d.geo_ref->word_weight,
                        nbmax, [](type::idx_t){return true;});
            }
            break;
        default: break;
        }

        //Mettre à jour les qualités en implémentant une ou plusieurs règles.
        update_quality(result, type, addType, d);

        create_pb(result, type, depth, d, pb_response);
    }

    auto compare = [](pbnavitia::Place a, pbnavitia::Place b){
        return a.quality() > b.quality();
    };

    //Trier le résultat partiallement jusqu'au nbmax(10 par défaut) eléments et supprimer le reste.
    nbmax = nbmax_temp;
    auto mutable_places = pb_response.mutable_places();
    int result_size = std::min(nbmax, mutable_places->size());
    std::partial_sort(mutable_places->begin(),mutable_places->begin() + result_size,
                      mutable_places->end(),compare);

    while (mutable_places->size() > nbmax){
        mutable_places->RemoveLast();
    }

    auto compare_attributs = [](pbnavitia::Place a, pbnavitia::Place b)->bool {
        if (a.embedded_type() != b.embedded_type()){
            const auto a_order = get_embedded_type_order(a.embedded_type());
            const auto b_order = get_embedded_type_order(b.embedded_type());
            return  a_order< b_order;
        } else if(a.quality() == b.quality()) {
            return boost::algorithm::lexicographical_compare(a.name(), b.name(), boost::is_iless());
        }
        else {
            return a.quality() > b.quality();
        }
    };

    //Retrier le résultat final
    result_size = std::min(nbmax, mutable_places->size());
    std::partial_sort(mutable_places->begin(), mutable_places->begin() + result_size,
                      mutable_places->end(), compare_attributs);

    auto pagination = pb_response.mutable_pagination();
    pagination->set_totalresult(result_size);
    pagination->set_startpage(0);
    pagination->set_itemsperpage(nbmax);
    pagination->set_itemsonpage(result_size);
    return pb_response;
}

}} //namespace navitia::autocomplete
