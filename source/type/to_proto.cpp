#include "to_proto.h"

namespace navitia { namespace type {
void to_proto(navitia::type::GeographicalCoord geo, pbnavitia::GeographicalCoord *geopb ) {
    geopb->set_x(geo.x);
    geopb->set_y(geo.y);
}

void to_proto(navitia::type::StopArea sa, pbnavitia::StopArea *sapb) {
    sapb->set_name(sa.name);
    to_proto(sa.coord, sapb->mutable_coord());
}


void to_proto(navitia::type::StopPoint sp, pbnavitia::StopPoint *sppb) {
    sppb->set_name(sp.name);
    to_proto(sp.coord, sppb->mutable_coord());
}

void to_proto(navitia::type::StopTime st, pbnavitia::StopTime* stpb) {
    stpb->set_arrival_time(st.arrival_time);
    stpb->set_departure_time(st.departure_time);
}
}}
