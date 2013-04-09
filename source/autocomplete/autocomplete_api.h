#pragma once
#include "type/type.pb.h"
#include "type/response.pb.h"
#include "type/request.pb.h"

namespace navitia {

namespace type {
struct Data;
enum class Type_e;
}

namespace autocomplete {

/** Trouve tous les objets définis par filter dont le nom contient name */
pbnavitia::Response autocomplete(const std::string &name,
                                 const std::vector<navitia::type::Type_e> &filter,
                                 uint32_t depth,
                                 int nbmax,
                                 const std::vector <std::string> &admins,
                                 const type::Data &d);


}}
