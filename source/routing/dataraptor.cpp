#include "dataraptor.h"
#include "routing.h"
namespace navitia { namespace routing { namespace raptor {

void dataRAPTOR::load(const type::PT_Data &data)
{
    retour_constant.resize(data.route_points.size());
    retour_constant_reverse.resize(data.route_points.size());

    for(auto &r : retour_constant_reverse) {
        r.arrival = DateTime::min;
        r.departure = DateTime::min;
    }

    for(auto &r : retour_constant) {
        r.arrival = DateTime::inf;
        r.departure = DateTime::inf;
    }
    
    foot_path.clear();
    std::vector<list_connections> footpath_temp;
    footpath_temp.resize(data.route_points.size());

    //Construction des connexions entre routepoints
    //(sert pour les prolongements de service ainsi que les correpondances garanties
    for(type::RoutePointConnection rpc : data.route_point_connections) {
        footpath_rp_forward.insert(std::make_pair(rpc.departure_route_point_idx, rpc));        
        footpath_rp_backward.insert(std::make_pair(rpc.destination_route_point_idx, rpc));
    }

    //Construction de la liste des marche à pied à partir des connexions renseignées
    for(type::Connection connection : data.connections) {
        footpath_temp[connection.departure_stop_point_idx][connection.destination_stop_point_idx] = connection;
    }

    for(type::Connection connection : data.connections) {
        if(footpath_temp[connection.destination_stop_point_idx].find(connection.departure_stop_point_idx) ==
           footpath_temp[connection.destination_stop_point_idx].end() ) {
            
            type::Connection inverse;
            inverse.duration = connection.duration;
            inverse.departure_stop_point_idx = connection.destination_stop_point_idx;
            inverse.destination_stop_point_idx = connection.departure_stop_point_idx;
            footpath_temp[inverse.departure_stop_point_idx][inverse.destination_stop_point_idx] = inverse;
        }
    }

    //On rajoute des connexions entre les stops points d'un même stop area si elles n'existent pas
    footpath_index.resize(data.stop_points.size());
    footpath_index.resize(data.stop_points.size());
    for(type::StopPoint sp : data.stop_points) {
        type::StopArea sa = data.stop_areas[sp.stop_area_idx];
        footpath_index[sp.idx].first = foot_path.size();

        int size = 0;
        for(auto conn : footpath_temp[sp.idx]) {
            foot_path.push_back(conn.second);
            ++size;
        }


        for(type::idx_t spidx2 : sa.stop_point_list) {
            if(sp.idx != spidx2 && 
                footpath_temp[sp.idx].find(spidx2) == footpath_temp[sp.idx].end()) {
                type::Connection c;
                c.departure_stop_point_idx = sp.idx;
                c.destination_stop_point_idx = spidx2;
                c.duration = 2 * 60;
                foot_path.push_back(c);
                ++size;
            }
        }
        footpath_index[sp.idx].second = size;
    }




    typedef std::unordered_map<navitia::type::idx_t, vector_idx> idx_vector_idx;
    idx_vector_idx ridx_route;
    
    arrival_times.clear();
    departure_times.clear();
    st_idx_forward.clear();
    st_idx_backward.clear();
    first_stop_time.clear();
    vp_idx_forward.clear();
    vp_idx_backward.clear();


    //On recopie les validity patterns
    for(auto vp : data.validity_patterns) 
        validity_patterns.push_back(vp.days);

    for(const type::Route & route : data.routes) {
        first_stop_time.push_back(arrival_times.size());
        nb_trips.push_back(route.vehicle_journey_list.size());
        for(unsigned int i=0; i < route.route_point_list.size(); ++i) {
            std::vector<type::idx_t> vec_stdix;
            for(type::idx_t vjidx : route.vehicle_journey_list) {
                vec_stdix.push_back(data.vehicle_journeys[vjidx].stop_time_list[i]);
            }
            std::sort(vec_stdix.begin(), vec_stdix.end(),
                      [&](type::idx_t stidx1, type::idx_t stidx2)->bool{
                        const type::StopTime & st1 = data.stop_times[stidx1];
                        const type::StopTime & st2 = data.stop_times[stidx2];
                        return (st1.departure_time % SECONDS_PER_DAY == st2.departure_time % SECONDS_PER_DAY && stidx1 < stidx2) ||
                               (st1.departure_time % SECONDS_PER_DAY <  st2.departure_time % SECONDS_PER_DAY);});
            st_idx_forward.insert(st_idx_forward.end(), vec_stdix.begin(), vec_stdix.end());
            for(auto stidx : vec_stdix) {
                const auto & st = data.stop_times[stidx];
                departure_times.push_back(st.departure_time % SECONDS_PER_DAY);
                if(st.departure_time > SECONDS_PER_DAY) {
                    auto vp = data.validity_patterns[data.vehicle_journeys[st.vehicle_journey_idx].validity_pattern_idx].days;
                    vp >>=1;
                    auto it = std::find(validity_patterns.begin(), validity_patterns.end(), vp);
                    if(it == validity_patterns.end()) {
                        vp_idx_forward.push_back(validity_patterns.size());
                        validity_patterns.push_back(vp);
                    } else {
                        vp_idx_forward.push_back(it - validity_patterns.begin());
                    }
                } else {
                    vp_idx_forward.push_back(data.vehicle_journeys[st.vehicle_journey_idx].validity_pattern_idx);
                }
            }

            std::sort(vec_stdix.begin(), vec_stdix.end(),
                  [&](type::idx_t stidx1, type::idx_t stidx2)->bool{
                        const type::StopTime & st1 = data.stop_times[stidx1];
                        const type::StopTime & st2 = data.stop_times[stidx2];
                        return (st1.arrival_time % SECONDS_PER_DAY == st2.arrival_time % SECONDS_PER_DAY && stidx1 > stidx2) ||
                               (st1.arrival_time % SECONDS_PER_DAY >  st2.arrival_time % SECONDS_PER_DAY);});

            st_idx_backward.insert(st_idx_backward.end(), vec_stdix.begin(), vec_stdix.end());
            for(auto stidx : vec_stdix) {
                const auto & st = data.stop_times[stidx];
                arrival_times.push_back(st.arrival_time % SECONDS_PER_DAY);
                if(st.arrival_time > SECONDS_PER_DAY) {
                    auto vp = data.validity_patterns[data.vehicle_journeys[st.vehicle_journey_idx].validity_pattern_idx].days;
                    vp >>=1;;
                    auto it = std::find(validity_patterns.begin(), validity_patterns.end(), vp);
                    if(it == validity_patterns.end()) {
                        vp_idx_backward.push_back(validity_patterns.size());
                        validity_patterns.push_back(vp);
                    } else {
                        vp_idx_backward.push_back(it - validity_patterns.begin());
                    }
                } else {
                    vp_idx_backward.push_back(data.vehicle_journeys[st.vehicle_journey_idx].validity_pattern_idx);
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
            //for(auto rid : ridx_route[rp.route_idx]) {
            auto rid = rp.route_idx;
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

        std::vector<pair_int> tmp;
        for(auto it : temp) {
            tmp.push_back(it);
        }
        //        std::sort(tmp.begin(), tmp.end(), [&](pair_int p1, pair_int p2){return routes[p1.first].vp < routes[p2.first].vp;});
        sp_routeorder_const.insert(sp_routeorder_const.end(), tmp.begin(), tmp.end());


        tmp.clear();

        for(auto it : temp_reverse) {
            tmp.push_back(it);
        }
        //        std::sort(tmp.begin(), tmp.end(), [&](pair_int p1, pair_int p2){return routes[p1.first].vp < routes[p2.first].vp;});
        sp_routeorder_const_reverse.insert(sp_routeorder_const_reverse.end(), tmp.begin(), tmp.end());

        temp_index.second = sp_routeorder_const.size() - temp_index.first;
        temp_index_reverse.second = sp_routeorder_const_reverse.size() - temp_index_reverse.first;
        sp_indexrouteorder[sp.idx] = temp_index;
        sp_indexrouteorder_reverse[sp.idx] = temp_index_reverse;
    }


     std::cout << "Nb data stop times : " << data.stop_times.size() << " stopTimes : " << arrival_times.size()
               << " nb foot path : " << foot_path.size() << " Nombre de stop points : " << data.stop_points.size() << "nb vp : " << data.validity_patterns.size() << " nb routes " << routes.size() <<  std::endl;

}


                  }}}
