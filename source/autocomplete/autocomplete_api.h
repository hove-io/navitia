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

/** Trouve tous les objets définis par filter dont le nom contient q */
pbnavitia::Response autocomplete(const std::string &q,
                                 const std::vector<navitia::type::Type_e> &filter,
                                 uint32_t depth,
                                 int nbmax,
                                 const std::vector <std::string> &admins,
                                 int search_type,
                                 const type::Data &d);


}}
