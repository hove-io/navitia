#include "data.h"


void Data::build_index(){
    stoppoint_of_stoparea.create(stop_areas, stop_points, &StopPoint::stop_area_idx);
}

template<>
Container<StopPoint> & Data::get() {return stop_points;}
template<>
Container<StopArea> & Data::get() {return stop_areas;}
/*template<>
Container<StopArea> & get(Data & data) {return data.stop_areas;}

template<>
Container<Line> & get(Data & data) {return data.lines;}*/


