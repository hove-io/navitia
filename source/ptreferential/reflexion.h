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
#include <boost/variant.hpp>
#include <boost/utility/enable_if.hpp>

namespace navitia{ namespace ptref {
typedef boost::variant<std::string, int>  col_t;

/// Exception levée lorsqu'on demande un membre qu'on ne connait pas
struct unknown_member{};

/*Macro qui permet de savoir si une classe implémente un membre :
On l'utilise de la manière suivante :
DECL_HAS_MEMBER(id)
Reflect_id<JourneyPattern>::value vaut true si le membre existe
Cela génère ensuite une fonction permettant d'avoir le membre d'un objet :
get_id(journey_pattern)
la fonction lève une exception lorsque l'objet n'a pas de membre*/
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

/// Wrapper pour éviter de devoir définir explicitement le type
template<class T>
col_t get_value(T& object, const std::string & name){
    if(name == "id") return get_id(object);
    else if(name == "idx") return get_idx(object);
    else if(name == "uri") return get_uri(object);
    else if(name == "name") return get_name(object);
    else
        throw unknown_member();
}

}}
