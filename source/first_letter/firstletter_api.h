#pragma once
#include "type/data.h"
#include "type/type.pb.h"

namespace navitia { namespace firstletter {

/** Trouve tous les objets définis par filter dont le nom contient name */
pbnavitia::Response firstletter(const std::string &name, const std::vector<navitia::type::Type_e> &filter, const navitia::type::Data &d);

}}
