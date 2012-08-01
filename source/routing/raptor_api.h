#pragma once
#include "raptor.h"
#include "type/type.pb.h"


namespace navitia { namespace routing { namespace raptor {

pbnavitia::Response make_response(std::vector<navitia::routing::Path> pathes, const nt::Data & d);
void create_pb_itineraire(navitia::routing::Path &path, const nt::Data & data, pbnavitia::ItineraireDetail &itineraire);
void create_pb_froute(navitia::routing::Path & path, const nt::Data & data, pbnavitia::FeuilleRoute &solution);

}
}
}
