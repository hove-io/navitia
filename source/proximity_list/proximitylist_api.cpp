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

#include "proximitylist_api.h"
#include "type/pb_converter.h"
#include "utils/paginate.h"
#include "ptreferential/ptreferential.h"
#include "type/pt_data.h"


namespace navitia { namespace proximitylist {
/**
 * se charge de remplir l'objet protocolbuffer autocomplete passé en paramètre
 *
 */
typedef std::tuple<nt::idx_t, nt::GeographicalCoord, nt::Type_e> t_result;
typedef std::pair<nt::idx_t, nt::GeographicalCoord> idx_coord;
static void create_pb(const std::vector<t_result>& result, uint32_t depth, const nt::Data& data,
                      pbnavitia::Response& pb_response, type::GeographicalCoord coord){

    for(auto result_item : result){
        pbnavitia::PtObject* place = pb_response.add_places_nearby();
        //on récupére la date pour les impacts
        auto current_date = boost::posix_time::second_clock::universal_time();
        auto idx = std::get<0>(result_item);
        auto coord_item = std::get<1>(result_item);
        auto type = std::get<2>(result_item);
        switch(type){
        case nt::Type_e::StopArea:
            navitia::fill_pb_object(data.pt_data->stop_areas[idx], data, place, depth, current_date);
            place->set_distance(coord.distance_to(coord_item));
            break;
        case nt::Type_e::StopPoint:
            navitia::fill_pb_object(data.pt_data->stop_points[idx], data, place, depth, current_date);
            place->set_distance(coord.distance_to(coord_item));
            break;
        case nt::Type_e::POI:
            navitia::fill_pb_object(data.geo_ref->pois[idx], data, place, depth, current_date);
            place->set_distance(coord.distance_to(coord_item));
            break;
        case nt::Type_e::Address:{
            const auto& way_coord = navitia::WayCoord(data.geo_ref->ways[idx],
                    coord, data.geo_ref->ways[idx]->nearest_number(coord).first);
            navitia::fill_pb_object(&way_coord, data, place, depth, current_date);
            place->set_distance(coord.distance_to(coord_item));
            break;
        }
        default:
            break;
        }
    }
}

template<typename T>
void sort_cut(std::vector<T> &list, const uint32_t end_pagination,
              const type::GeographicalCoord& coord) {
    if(!list.empty()) {
        auto nb_sort = (list.size() < end_pagination) ? list.size():end_pagination;
        std::partial_sort(list.begin(), list.begin() + nb_sort, list.end(),
            [&](idx_coord a, idx_coord b)
            {return coord.distance_to(a.second)< coord.distance_to(b.second);});
        list.resize(nb_sort);
    }
}

pbnavitia::Response find(const type::GeographicalCoord& coord, const double distance,
                         const std::vector<nt::Type_e>& types, const std::string& filter,
                         const uint32_t depth, const uint32_t count, const uint32_t start_page,
                         const type::Data & data) {
    pbnavitia::Response response;
    int total_result = 0;
    std::vector<t_result > result;
    auto end_pagination = (start_page+1) * count;
    for(nt::Type_e type : types){
        if(type == nt::Type_e::Address) {
            //for addresses we use the street network
            try {
                auto nb_w = data.geo_ref->nearest_addr(coord);
                // we'll regenerate the good number in create_pb
                result.push_back(t_result(nb_w.second->idx, coord, type));
                ++total_result;
            } catch(proximitylist::NotFound) {}
            continue;
        }

        vector_idx_coord list;
        std::vector<type::idx_t> indexes;
        if(! filter.empty()) {
            try {
                indexes = ptref::make_query(type, filter, data);
            } catch(const ptref::parsing_error &parse_error) {
                fill_pb_error(pbnavitia::Error::unable_to_parse,
                              "Problem while parsing the query:" + parse_error.more, response.mutable_error());
                return response;
            } catch(const ptref::ptref_error &pt_error) {
                fill_pb_error(pbnavitia::Error::bad_filter,
                              "ptref : " + pt_error.more, response.mutable_error());
                return response;
            }
        }
        switch(type){
        case nt::Type_e::StopArea:
            list = data.pt_data->stop_area_proximity_list.find_within(coord, distance);
            break;
        case nt::Type_e::StopPoint:
            list = data.pt_data->stop_point_proximity_list.find_within(coord, distance);
            break;
        case nt::Type_e::POI:
            list = data.geo_ref->poi_proximity_list.find_within(coord, distance);
            break;
        default: break;
        }

        vector_idx_coord final_list;
        if(filter.empty()) {
            final_list = list;
        } else {
            //We find the intersection between indexes and list
            //we sort the proximity list on the indexes
            auto comp = [](idx_coord a, idx_coord b) { return a.first < b.first; };
            std::sort(list.begin(), list.end(), comp);

            for (auto idx : indexes) {
                auto it = std::lower_bound(list.begin(), list.end(), std::make_pair(idx, nt::GeographicalCoord()), comp); //we create a mock pair and only compare the first elt
                if (it != list.end() && (*it).first == idx) {
                    final_list.push_back(*it);
                    if (final_list.size() == list.size()) {
                        //small speedup, if all geographical elt have been added we can stop
                        //(since the indexes list might be way bigger than the proximity list)
                        break;
                    }
                }
            }
        }
        total_result += final_list.size();
        sort_cut<idx_coord>(final_list, end_pagination, coord);
        for(auto idx_coord : final_list) {
            result.push_back(t_result(idx_coord.first, idx_coord.second, type));
        }
    }
    auto nb_sort = (result.size() < end_pagination) ? result.size():end_pagination;
    std::partial_sort(result.begin(), result.begin() + nb_sort, result.end(),
        [coord](t_result t1, t_result t2)->bool {
            auto a = std::get<1>(t1); auto b = std::get<1>(t2);
            return coord.distance_to(a)< coord.distance_to(b);
        });
    result = paginate(result, count, start_page);
    create_pb(result, depth, data, response, coord);
    auto pagination = response.mutable_pagination();
    pagination->set_totalresult(total_result);
    pagination->set_startpage(start_page);
    pagination->set_itemsperpage(count);
    pagination->set_itemsonpage(result.size());
    return response;
}
}} // namespace navitia::proximitylist
