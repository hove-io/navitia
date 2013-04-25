#include "ptreferential.h"
#include "type/pb_converter.h"
#include "type/data.h"

namespace pt = boost::posix_time;

namespace navitia{ namespace ptref{

pbnavitia::Response extract_data(const type::Data & data, type::Type_e requested_type, std::vector<type::idx_t> & rows, const int depth) {
    pbnavitia::Response result;

    //on utilise la date courante pour choisir si on doit afficher les  messages de perturbation
    pt::ptime today = pt::second_clock::local_time();

    for(type::idx_t idx : rows){
        switch(requested_type){
#define FILL_PB_OBJECT(type_name, collection_name) case type::Type_e::type_name: fill_pb_object(idx, data, result.add_##collection_name(), depth, today); break;
        ITERATE_NAVITIA_PT_TYPES(FILL_PB_OBJECT)
        default: break;
        }
    }
    return result;
}

pbnavitia::Response query_pb(type::Type_e requested_type, std::string request, const int depth, const type::Data &data){
    std::vector<type::idx_t> final_indexes;
    pbnavitia::Response pb_response;
    try {
        final_indexes = make_query(requested_type, request, data);
    } catch(parsing_error parse_error) {
        pb_response.set_error(parse_error.more);
        return pb_response;
    }

    return extract_data(data, requested_type, final_indexes, depth);
}


}}
