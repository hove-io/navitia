#include "dataraptor.h"
#include "routing.h"
namespace navitia { namespace routing { namespace raptor {

void dataRAPTOR::load(const type::PT_Data &data)
{
    retour_constant.resize(data.stop_points.size());
    retour_constant_reverse.resize(data.stop_points.size());

    for(auto &r : retour_constant_reverse) {
        r.dt = DateTime::min;
    }

    for(auto &r : retour_constant) {
        r.dt = DateTime::inf;
    }
    //Construction de la liste des marche à pied à partir des connections renseignées

    std::vector<list_connections> footpath_temp;
    footpath_temp.resize(data.stop_points.size());
    for(navitia::type::Connection connection : data.connections) {
        footpath_temp[connection.departure_stop_point_idx][connection.destination_stop_point_idx] = connection;
        navitia::type::Connection inverse;
        inverse.departure_stop_point_idx = connection.destination_stop_point_idx;
        inverse.destination_stop_point_idx = connection.departure_stop_point_idx;
        inverse.duration = connection.duration;
        footpath_temp[connection.destination_stop_point_idx][connection.departure_stop_point_idx] = inverse;
    }

    //On rajoute des connexions entre les stops points d'un même stop area si elles n'existent pas
    footpath_index.resize(data.stop_points.size());
    footpath_index.resize(data.stop_points.size());
    for(navitia::type::StopPoint sp : data.stop_points) {
        navitia::type::StopArea sa = data.stop_areas[sp.stop_area_idx];
        footpath_index[sp.idx].first = foot_path.size();

        int size = 0;
        for(auto conn : footpath_temp[sp.idx]) {
            foot_path.push_back(conn.second);
            ++size;
        }

        for(navitia::type::idx_t spidx : sa.stop_point_list) {
            navitia::type::StopPoint sp2 = data.stop_points[spidx];
            if(sp.idx != sp2.idx) {
                if(footpath_temp[sp.idx].find(sp2.idx) == footpath_temp[sp.idx].end()) {
                    navitia::type::Connection c;
                    c.departure_stop_point_idx = sp.idx;
                    c.destination_stop_point_idx = sp2.idx;
                    c.duration = 2 * 60;
                    foot_path.push_back(c);
                    ++size;
                }
            }
        }
        footpath_index[sp.idx].second = size;
    }




    typedef std::unordered_map<navitia::type::idx_t, vector_idx> idx_vector_idx;
    idx_vector_idx ridx_route;
    for(auto & route : data.routes) {
        ridx_route[route.idx] = vector_idx();
        if(route.route_point_list.size() > 0) {
            idx_vector_idx vp_vj;
            //Je regroupe entre elles les routes ayant le même validity pattern
            for(navitia::type::idx_t vjidx : route.vehicle_journey_list) {
                if(vp_vj.find(data.vehicle_journeys[vjidx].validity_pattern_idx) == vp_vj.end())
                    vp_vj[data.vehicle_journeys[vjidx].validity_pattern_idx] = vector_idx();
                vp_vj[data.vehicle_journeys[vjidx].validity_pattern_idx].push_back(vjidx);
            }

            for(idx_vector_idx::value_type vec : vp_vj) {
                if(vec.second.size() > 0) {
                    Route_t r;
                    r.idx = route.idx;
                    r.nbStops = route.route_point_list.size();
                    r.nbTrips = vec.second.size();
                    r.vp = data.validity_patterns[vec.first].idx;
                    r.firstStopTime = stopTimes.size();
                    for(auto vjidx : vec.second) {
                        for(navitia::type::idx_t stidx : data.vehicle_journeys[vjidx].stop_time_list) {
                            StopTime_t st =  data.stop_times[stidx];
                            stopTimes.push_back(st);
                        }
                    }
                    ridx_route[route.idx].push_back(routes.size());
                    routes.push_back(r);
                }
            }
        }
    }

    sp_indexrouteorder.resize(data.stop_points.size());
    sp_indexrouteorder_reverse.resize(data.stop_points.size());
    for(auto &sp : data.stop_points) {
        pair_int temp_index, temp_index_reverse;
        temp_index.first = sp_routeorder_const.size();
        temp_index_reverse.first = sp_routeorder_const_reverse.size();
        std::map<navitia::type::idx_t, int> temp, temp_reverse;
        for(navitia::type::idx_t idx_rp : sp.route_point_list) {
            auto &rp = data.route_points[idx_rp];
            for(auto rid : ridx_route[rp.route_idx]) {
                auto it = temp.find(rid), it_reverse = temp_reverse.find(rid);
                if(it == temp.end()) {
                    temp[rid] = rp.order;
                } else if(temp[rp.route_idx] > rp.order) {
                    temp[rid] = rp.order;
                }

                if(it_reverse == temp_reverse.end()) {
                    temp_reverse[rid] = rp.order;
                } else if(temp_reverse[rp.route_idx] < rp.order) {
                    temp_reverse[rid] = rp.order;
                }
            }

        }

        for(auto it : temp) {
            sp_routeorder_const.push_back(it);
        }

        for(auto it : temp_reverse) {
            sp_routeorder_const_reverse.push_back(it);
        }

        temp_index.second = sp_routeorder_const.size() - temp_index.first;
        temp_index_reverse.second = sp_routeorder_const_reverse.size() - temp_index_reverse.first;
        sp_indexrouteorder[sp.idx] = temp_index;
        sp_indexrouteorder_reverse[sp.idx] = temp_index_reverse;
    }


    std::cout << "Nb data stop times : " << data.stop_times.size() << " stopTimes : " << stopTimes.size()
              << " nb foot path : " << foot_path.size() << " Nombre de stop points : " << data.stop_points.size() << "nb vp : " << data.validity_patterns.size() << " nb routes " << routes.size() <<  std::endl;

}


}}}
