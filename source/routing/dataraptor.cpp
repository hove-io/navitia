#include "dataraptor.h"
#include "routing.h"
namespace navitia { namespace routing { namespace raptor {

void dataRAPTOR::load(const type::PT_Data &data)
{
    labels_const.resize(data.journey_pattern_points.size());
    labels_const_reverse.resize(data.journey_pattern_points.size());

    for(auto &r : labels_const_reverse) {
        r.arrival = navitia::type::DateTime::min;
        r.departure = navitia::type::DateTime::min;
    }

    for(auto &r : labels_const) {
        r.arrival = navitia::type::DateTime::inf;
        r.departure = navitia::type::DateTime::inf;
    }
    
    foot_path_forward.clear();
    foot_path_backward.clear();
    footpath_index_backward.clear();
    footpath_index_forward.clear();
    footpath_rp_backward.clear();
    footpath_rp_forward.clear();
    std::vector<list_connections> footpath_temp_forward, footpath_temp_backward;
    footpath_temp_forward.resize(data.journey_pattern_points.size());
    footpath_temp_backward.resize(data.journey_pattern_points.size());

    //Construction des connexions entre journey_patternpoints
    //(sert pour les prolongements de service ainsi que les correpondances garanties
    for(type::JourneyPatternPointConnection rpc : data.journey_pattern_point_connections) {
        footpath_rp_forward.insert(std::make_pair(rpc.departure_journey_pattern_point_idx, rpc));        
        footpath_rp_backward.insert(std::make_pair(rpc.destination_journey_pattern_point_idx, rpc));
    }

    //Construction de la liste des marche à pied à partir des connexions renseignées
    for(type::Connection connection : data.connections) {
        footpath_temp_forward[connection.departure_stop_point_idx][connection.destination_stop_point_idx] = connection;
    }

    for(type::Connection connection : data.connections) {
        if(footpath_temp_backward[connection.destination_stop_point_idx].find(connection.departure_stop_point_idx) ==
           footpath_temp_backward[connection.destination_stop_point_idx].end() ) {
            
            type::Connection inverse;
            inverse.duration = connection.duration;
            inverse.departure_stop_point_idx = connection.destination_stop_point_idx;
            inverse.destination_stop_point_idx = connection.departure_stop_point_idx;
            footpath_temp_backward[inverse.departure_stop_point_idx][inverse.destination_stop_point_idx] = inverse;
        } else if(footpath_temp_backward[connection.destination_stop_point_idx][connection.departure_stop_point_idx].duration > connection.duration) {
            footpath_temp_backward[connection.destination_stop_point_idx][connection.departure_stop_point_idx].duration = connection.duration;
        }

    }

    //On rajoute des connexions entre les stops points d'un même stop area si elles n'existent pas
    footpath_index_forward.resize(data.stop_points.size());
    footpath_index_backward.resize(data.stop_points.size());
    for(type::StopPoint sp : data.stop_points) {
        if(sp.stop_area_idx != type::invalid_idx) {
            type::StopArea sa = data.stop_areas[sp.stop_area_idx];
            footpath_index_forward[sp.idx].first = foot_path_forward.size();
            footpath_index_backward[sp.idx].first = foot_path_backward.size();
            int size_forward = 0, size_backward = 0;
            for(auto conn : footpath_temp_forward[sp.idx]) {
                foot_path_forward.push_back(conn.second);
                ++size_forward;
            }
            for(auto conn : footpath_temp_backward[sp.idx]) {
                foot_path_backward.push_back(conn.second);
                ++size_backward;
            }


            for(type::idx_t spidx2 : sa.stop_point_list) {
                if(sp.idx != spidx2) {
                    type::Connection c;
                    c.departure_stop_point_idx = sp.idx;
                    c.destination_stop_point_idx = spidx2;
                    c.duration = 2 * 60;
                    if(footpath_temp_forward[sp.idx].find(spidx2) == footpath_temp_forward[sp.idx].end()) {
                        foot_path_forward.push_back(c);
                        ++size_forward;
                    }

                    if(footpath_temp_backward[spidx2].find(sp.idx) == footpath_temp_backward[spidx2].end()) {
                        foot_path_backward.push_back(c);
                        ++size_backward;
                    }


                }

            }
            footpath_index_forward[sp.idx].second = size_forward;
            footpath_index_backward[sp.idx].second = size_backward;
        }
    }




    typedef std::unordered_map<navitia::type::idx_t, vector_idx> idx_vector_idx;
    idx_vector_idx ridx_journey_pattern;
    
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

    for(const type::JourneyPattern & journey_pattern : data.journey_patterns) {
        first_stop_time.push_back(arrival_times.size());
        nb_trips.push_back(journey_pattern.vehicle_journey_list.size());
        // On regroupe ensemble tous les horaires de tous les journey_pattern_point
        for(unsigned int i=0; i < journey_pattern.journey_pattern_point_list.size(); ++i) {
            std::vector<type::idx_t> vec_stdix;
            for(type::idx_t vjidx : journey_pattern.vehicle_journey_list) {
                vec_stdix.push_back(data.vehicle_journeys[vjidx].stop_time_list[i]);
            }
            std::sort(vec_stdix.begin(), vec_stdix.end(),
                      [&](type::idx_t stidx1, type::idx_t stidx2)->bool{
                        const type::StopTime & st1 = data.stop_times[stidx1];
                        const type::StopTime & st2 = data.stop_times[stidx2];
                        uint32_t time1, time2;
                        if(!st1.is_frequency())
                            time1 = st1.departure_time % SECONDS_PER_DAY;
                        else
                            time1 = st1.end_time;
                        if(!st2.is_frequency())
                            time2 = st2.departure_time % SECONDS_PER_DAY;
                        else
                            time2 = st2.end_time;

                        return (time1 == time2 && stidx1 < stidx2) || (time1 < time2);});


            st_idx_forward.insert(st_idx_forward.end(), vec_stdix.begin(), vec_stdix.end());

            for(auto stidx : vec_stdix) {
                const auto & st = data.stop_times[stidx];
                uint32_t time;
                if(!st.is_frequency())
                    time = st.departure_time % SECONDS_PER_DAY;
                else
                    time = st.end_time;
                departure_times.push_back(time);
                if(st.departure_time > SECONDS_PER_DAY) {
                    auto vp = data.validity_patterns[data.vehicle_journeys[st.vehicle_journey_idx].validity_pattern_idx].days;
                    vp <<=1;
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
                      uint32_t time1, time2;
                      if(!st1.is_frequency())
                          time1 = st1.arrival_time % SECONDS_PER_DAY;
                      else
                          time1 = st1.start_time;
                      if(!st2.is_frequency())
                          time2 = st2.arrival_time% SECONDS_PER_DAY;
                      else
                          time2 = st2.start_time;

                      return (time1 == time2 && stidx1 > stidx2) || (time1 > time2);});


            st_idx_backward.insert(st_idx_backward.end(), vec_stdix.begin(), vec_stdix.end());
            for(auto stidx : vec_stdix) {
                const auto & st = data.stop_times[stidx];
                uint32_t time;
                if(!st.is_frequency())
                    time = st.arrival_time % SECONDS_PER_DAY;
                else
                    time = st.start_time;
                arrival_times.push_back(time);
                if(st.arrival_time > SECONDS_PER_DAY) {
                    auto vp = data.validity_patterns[data.vehicle_journeys[st.vehicle_journey_idx].validity_pattern_idx].days;
                    vp <<=1;;
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


}


}}}
