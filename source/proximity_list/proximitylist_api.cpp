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
void create_pb(const std::vector<t_result>& result, uint32_t depth, const nt::Data& data,
               pbnavitia::Response& pb_response, type::GeographicalCoord coord){

    for(auto result_item : result){
        pbnavitia::Place* place = pb_response.add_places_nearby();
        //on récupére la date pour les impacts
        auto current_date = boost::posix_time::second_clock::local_time();
        auto idx = std::get<0>(result_item);
        auto coord_item = std::get<1>(result_item);
        auto type = std::get<2>(result_item);
        switch(type){
        case nt::Type_e::StopArea:
            fill_pb_object(data.pt_data->stop_areas[idx], data, place->mutable_stop_area(), depth,
                           current_date);
            place->set_name(data.pt_data->stop_areas[idx]->name);
            place->set_uri(data.pt_data->stop_areas[idx]->uri);
            place->set_distance(coord.distance_to(coord_item));
            place->set_embedded_type(pbnavitia::STOP_AREA);
            break;
        case nt::Type_e::StopPoint:
            fill_pb_object(data.pt_data->stop_points[idx], data, place->mutable_stop_point(), depth,
                           current_date);
            place->set_name(data.pt_data->stop_points[idx]->name);
            place->set_uri(data.pt_data->stop_points[idx]->uri);
            place->set_distance(coord.distance_to(coord_item));
            place->set_embedded_type(pbnavitia::STOP_POINT);
            break;
        case nt::Type_e::POI:
            fill_pb_object(data.geo_ref->pois[idx], data, place->mutable_poi(), depth,
                           current_date);
            place->set_name(data.geo_ref->pois[idx]->name);
            place->set_uri(data.geo_ref->pois[idx]->uri);
            place->set_distance(coord.distance_to(coord_item));
            place->set_embedded_type(pbnavitia::POI);
            break;
        case nt::Type_e::Address:
            fill_pb_object(data.geo_ref->ways[idx], data, place->mutable_address(),
                           data.geo_ref->ways[idx]->nearest_number(coord), coord , depth);
            place->set_name(data.geo_ref->ways[idx]->name);
            place->set_uri(data.geo_ref->ways[idx]->uri + ":" + boost::lexical_cast<std::string>(data.geo_ref->ways[idx]->nearest_number(coord)));
            place->set_distance(coord.distance_to(coord_item));
            place->set_embedded_type(pbnavitia::ADDRESS);
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
           try {
               georef::edge_t e = data.geo_ref->nearest_edge(coord, data.geo_ref->pl);
               ++total_result;
               georef::Way *way = data.geo_ref->ways[data.geo_ref->graph[e].way_idx];
               result.push_back(t_result(way->idx, coord, type));
           } catch(proximitylist::NotFound) {}
        } else{
            vector_idx_coord list;
            std::vector<type::idx_t> indexes;
            if(filter != "") {
                try {
                    indexes = ptref::make_query(type, filter, data);
                } catch(const ptref::parsing_error &parse_error) {
                    fill_pb_error(pbnavitia::Error::unable_to_parse,
                                  "Unable to parse :" + parse_error.more, response.mutable_error());
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
            //We find the intersection between indexes and list
            auto it_indexes = indexes.begin();
            auto it_list = list.begin();
            vector_idx_coord final_list;
            if(filter=="") {
                final_list = list;
            } else {
                while(it_indexes != indexes.end() && it_list != list.end()) {
                    if(*it_indexes < it_list->first) ++it_indexes;
                    else if(it_list->first < *it_indexes) ++ it_list;
                    else {
                        final_list.push_back(*it_list);
                        ++it_list;
                        ++it_indexes;
                    }
                }
            }
            total_result += final_list.size();
            sort_cut<idx_coord>(final_list, end_pagination, coord);
            for(auto idx_coord : final_list) {
                result.push_back(t_result(idx_coord.first, idx_coord.second, type));
            }
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
