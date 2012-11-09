#include "ptreferential/ptreferential.h"
#include "routing/raptor.h"

namespace navitia { namespace timetables {

typedef std::pair<routing::DateTime, const type::idx_t> dt_st;
struct comp_st {
    bool operator()(const dt_st st1, const dt_st st2) const {
        return (st1.first < st2.first) || ((st1.first ==  st2.first) && (st1.second < st2.second));
    }
};


pbnavitia::Response next_departures(std::string request, const std::string &str_dt, const uint32_t nb_departures, type::Data & data, routing::raptor::RAPTOR &raptor);
std::vector<type::idx_t> next_departures(const std::vector<type::idx_t> &route_points, const routing::DateTime &dt, const uint32_t nd_departures, routing::raptor::RAPTOR &raptor);
}}
