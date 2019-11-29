#include "autocomplete/utils.h"
#include "type/type.h"

namespace navitia {
namespace autocomplete {

int get_type_e_order(const type::Type_e& type) {
    switch (type) {
        case type::Type_e::Network:
            return 1;
        case type::Type_e::CommercialMode:
            return 2;
        case type::Type_e::Admin:
            return 3;
        case type::Type_e::StopArea:
            return 4;
        case type::Type_e::POI:
            return 5;
        case type::Type_e::Address:
            return 6;
        case type::Type_e::Line:
            return 7;
        case type::Type_e::Route:
            return 8;
        default:
            return 9;
    }
}

bool compare_type_e(const type::Type_e& a, const type::Type_e& b) {
    const auto a_order = get_type_e_order(a);
    const auto b_order = get_type_e_order(b);
    return a_order < b_order;
}

std::vector<std::vector<type::Type_e>> build_type_groups(std::vector<type::Type_e> filter) {
    std::vector<std::vector<type::Type_e>> result;
    std::sort(filter.begin(), filter.end(), compare_type_e);
    int current_order = -1;
    for (const auto& f : filter) {
        auto order = get_type_e_order(f);
        if (order != current_order) {
            current_order = order;
            result.emplace_back();
        }
        result.back().push_back(f);
    }
    return result;
}

}  // namespace autocomplete
}  // namespace navitia
