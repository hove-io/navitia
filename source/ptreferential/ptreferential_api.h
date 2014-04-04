#pragma once

namespace pbnavitia { struct Response;}

namespace navitia{ namespace ptref{
/// Execute une requête et génère la sortie protobuf
pbnavitia::Response extract_data(const type::Data & data,
                                 type::Type_e requested_type,
                                 std::vector<type::idx_t> & rows,
                                 const int depth);

/// Construit la réponse proto buf, une fois que l'on trouvé les indices
pbnavitia::Response query_pb(type::Type_e requested_type, std::string request,
                             const std::vector<std::string>& forbidden_uris,
                             const int depth, const bool show_codes,
                             const int startPage,
                             const int count, const type::Data &data);
}}
