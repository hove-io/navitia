/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.
  
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
#include <string>
#include <boost/utility/enable_if.hpp>

namespace navitia{ namespace ptref {

/// Exception thrown when an unknown member is asked for
struct unknown_member{};

/*
 * Macro used to know if a class has a member.
 *
 * Used as:
 * DECL_HAS_MEMBER(id)
 *
 * Reflect_id<JourneyPattern>::value is true is this member exists.
 *
 * It will thus generate a function to fetch the given member:
 * get_id(journey_pattern)
 *
 * This function will throw a unknown_member exception is the member does not exists
 */
#define DECL_HAS_MEMBER(MEM_NAME) \
template <typename T> \
struct Reflect_##MEM_NAME { \
    typedef char yes[1]; \
    typedef char no[2]; \
    template <typename C> \
    static yes& test(decltype(&C::MEM_NAME)*); \
    template <typename> static no& test(...); \
    static const bool value = sizeof(test<T>(0)) == sizeof(yes); \
};\
template<class T> typename boost::enable_if<Reflect_##MEM_NAME<T>, decltype(T::MEM_NAME)>::type get_##MEM_NAME(T&object){return object.MEM_NAME;}\
template<class T> typename boost::disable_if<Reflect_##MEM_NAME<T>, int>::type get_##MEM_NAME(const T&){throw unknown_member();}\
template<class T> typename boost::enable_if<Reflect_##MEM_NAME<T>, decltype(T::MEM_NAME) T::*>::type ptr_##MEM_NAME(){return &T::MEM_NAME;}\
template<class T> typename boost::disable_if<Reflect_##MEM_NAME<T>, int T::*>::type ptr_##MEM_NAME(){throw unknown_member();}

DECL_HAS_MEMBER(uri)
DECL_HAS_MEMBER(id)
DECL_HAS_MEMBER(idx)
DECL_HAS_MEMBER(name)
DECL_HAS_MEMBER(validity_pattern)
DECL_HAS_MEMBER(code)

}}
