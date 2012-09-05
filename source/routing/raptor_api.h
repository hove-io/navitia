#pragma once
#include "raptor.h"
#include "type/type.pb.h"


namespace navitia { namespace routing { namespace raptor {

pbnavitia::Response make_response(std::vector<navitia::routing::Path> paths, const nt::Data & d);

}}}
