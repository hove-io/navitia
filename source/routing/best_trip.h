#pragma once

#include "type/data.h"
#include "routing/dataraptor.h"


namespace navitia { namespace routing {
///Cherche le premier trip partant apres dt sur la route au route point order
int earliest_trip(const type::Route & route, const unsigned int order, const DateTime &dt, const type::Data &data);
///Cherche le premier trip partant avant dt sur la route au route point order
int tardiest_trip(const type::Route & route, const unsigned int order, const DateTime &dt, const type::Data &data);
}}
