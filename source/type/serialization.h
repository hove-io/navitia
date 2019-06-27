/* Copyright © 2001-2019, Canal TP and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
Help us simplify mobility and open public transport:
    a non ending quest to the responsive locomotion way of traveling!

LICENCE: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.

Stay tuned using
twitter @navitia
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/
#pragma once
#include <eos_portable_archive/portable_iarchive.hpp>
#include <eos_portable_archive/portable_oarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

// TODO: should be removed when migration is completed
#include <boost/serialization/weak_ptr.hpp>

#define SERIALIZABLE(T)                                                                                                \
    template void T::serialize<boost::archive::binary_oarchive>(boost::archive::binary_oarchive&, const unsigned int); \
    template void T::serialize<boost::archive::binary_iarchive>(boost::archive::binary_iarchive&, const unsigned int); \
    template void T::serialize<eos::portable_oarchive>(eos::portable_oarchive&, const unsigned int);                   \
    template void T::serialize<eos::portable_iarchive>(eos::portable_iarchive&, const unsigned int);

#define SPLIT_SERIALIZABLE(T)                                                                                     \
    template void T::save<boost::archive::binary_oarchive>(boost::archive::binary_oarchive&, const unsigned int)  \
        const;                                                                                                    \
    template void T::save<eos::portable_oarchive>(eos::portable_oarchive&, const unsigned int) const;             \
    template void T::load<boost::archive::binary_iarchive>(boost::archive::binary_iarchive&, const unsigned int); \
    template void T::load<eos::portable_iarchive>(eos::portable_iarchive&, const unsigned int);
