#include "data.h"


void Data::build_index(){
    stoppoint_of_stoparea.create(stop_areas, stop_points, &StopPoint::stop_area_idx);
}
