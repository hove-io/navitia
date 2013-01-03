#pragma once
#include "type/type.h"
#include "type/data.h"
#include "routing.h"
#include "raptor_utils.h"


namespace navitia { namespace routing { namespace raptor{ namespace init {

struct Departure_Type {
    type::idx_t rpidx;
    uint32_t count;
    DateTime arrival, upper_bound;
    float walking_time, ratio;

    Departure_Type() : rpidx(type::invalid_idx), count(0), walking_time(0), ratio(std::numeric_limits<float>::min()) {}
};

std::vector<Departure_Type> getDepartures(const std::vector<std::pair<type::idx_t, double> > &departs, const std::vector<std::pair<type::idx_t, double> > &destinations, bool clockwise, const map_retour_t &retour, const type::Data &data, const float walking_speed);

std::vector<Departure_Type> getDepartures(const std::vector<std::pair<type::idx_t, double> > &departs, const DateTime &dep, bool clockwise, const type::Data & data, const float walking_speed);

std::vector<Departure_Type> getWalkingSolutions(bool clockwise, const std::vector<std::pair<type::idx_t, double> > &departs, const std::vector<std::pair<type::idx_t, double> > &destinations, Departure_Type best, const map_retour_t &retour, const type::Data &data, const float walking_speed);

std::vector<Departure_Type> getParetoFront(bool clockwise, const std::vector<std::pair<type::idx_t, double> > &departs, const std::vector<std::pair<type::idx_t, double> > &destinations, const map_retour_t &retour, const type::Data &data, const float walking_speed);

std::pair<type::idx_t, DateTime>  getFinalRpidAndDate(int count, type::idx_t rpid, const map_retour_t &retour, bool clockwise, const type::Data &data);

float getWalkingTime(int count, type::idx_t rpid, const std::vector<std::pair<type::idx_t, double> > &departs, const std::vector<std::pair<type::idx_t, double> > &destinations, bool clockwise, const map_retour_t &retour, const type::Data &data);
}




}}}
