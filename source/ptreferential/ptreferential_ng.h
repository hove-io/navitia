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

#include "utils/exception.h"
#include "type/type_interfaces.h"
#include "type/data.h"

#include <boost/optional.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/fusion/adapted/struct.hpp>
#include <boost/variant/recursive_variant.hpp>

namespace navitia{
namespace ptref{

type::Indexes make_query_ng(const type::Type_e requested_type,
                            const std::string& request,
                            const std::vector<std::string>& forbidden_uris,
                            const type::OdtLevel_e odt_level,
                            const boost::optional<boost::posix_time::ptime>& since,
                            const boost::optional<boost::posix_time::ptime>& until,
                            const type::Data& data);

namespace ast {

struct All {};

struct Empty {};

struct Fun {
    std::string type;
    std::string method;
    std::vector<std::string> args;
};

typedef boost::variant<
    All,
    Empty,
    Fun>
Pred;

struct GetCorresponding;
struct And;
struct Or;
struct Diff;
template<typename OpTag>
struct BinaryOp;

typedef boost::variant<
    Pred,
    boost::recursive_wrapper<GetCorresponding>,
    boost::recursive_wrapper<BinaryOp<And>>,
    boost::recursive_wrapper<BinaryOp<Or>>,
    boost::recursive_wrapper<BinaryOp<Diff>>>
Expr;

struct GetCorresponding {
    std::string type;
    Expr expr;
};
template<typename OpTag>
struct BinaryOp {
    Expr lhs;
    Expr rhs;
    BinaryOp(Expr l, Expr r): lhs(l), rhs(r) {}
};

template<typename OS> OS& operator<<(OS& os, const All&) { return os << "all"; }
template<typename OS> OS& operator<<(OS& os, const Empty&) { return os << "empty"; }
template<typename OS>
void print_quoted_string(OS& os, const std::string& s) {
    os << '"';
    for (const auto& c: s) {
        switch (c) {
        case '\\':
        case '"':
            os << '\\' << c;
            break;
        default: os << c;
        }
    }
    os << '"';
}
template<typename OS>
OS& operator<<(OS& os, const Fun& fun) {
    os << fun.type << '.' << fun.method << '(';
    auto it = fun.args.begin(), end = fun.args.end();
    if (it != end) { print_quoted_string(os, *it); ++it; }
    for (; it != end; ++it) {
        os << ", ";
        print_quoted_string(os, *it);
    }
    return os << ')';
}
template<typename OS>
OS& operator<<(OS& os, const GetCorresponding& to_object) {
    return os << "(GET " << to_object.type << " <- " << to_object.expr << ')';
}
template<typename OS>
OS& operator<<(OS& os, const BinaryOp<And>& op) {
    return os << '(' << op.lhs << " AND " << op.rhs << ')';
}
template<typename OS>
OS& operator<<(OS& os, const BinaryOp<Or>& op) {
    return os << '(' << op.lhs << " OR " << op.rhs << ')';
}
template<typename OS>
OS& operator<<(OS& os, const BinaryOp<Diff>& op) {
    return os << '(' << op.lhs << " - " << op.rhs << ')';
}

}// namespace ast

ast::Expr parse(const std::string& s);
std::string make_request(const type::Type_e requested_type,
                         const std::string& request,
                         const std::vector<std::string>& forbidden_uris,
                         const type::OdtLevel_e odt_level,
                         const boost::optional<boost::posix_time::ptime>& since,
                         const boost::optional<boost::posix_time::ptime>& until,
                         const type::Data& data);

}} // namespace navitia::ptref

BOOST_FUSION_ADAPT_STRUCT(navitia::ptref::ast::All, )
BOOST_FUSION_ADAPT_STRUCT(navitia::ptref::ast::Empty, )
BOOST_FUSION_ADAPT_STRUCT(
    navitia::ptref::ast::Fun,
    (std::string, type)
    (std::string, method)
    (std::vector<std::string>, args)
)
BOOST_FUSION_ADAPT_STRUCT(
    navitia::ptref::ast::GetCorresponding,
    (std::string, type)
    (navitia::ptref::ast::Expr, expr)
)
BOOST_FUSION_ADAPT_STRUCT(
    navitia::ptref::ast::BinaryOp<navitia::ptref::ast::Diff>,
    (navitia::ptref::ast::Expr, lhs)
    (navitia::ptref::ast::Expr, rhs)
)
BOOST_FUSION_ADAPT_STRUCT(
    navitia::ptref::ast::BinaryOp<navitia::ptref::ast::And>,
    (navitia::ptref::ast::Expr, lhs)
    (navitia::ptref::ast::Expr, rhs)
)
BOOST_FUSION_ADAPT_STRUCT(
    navitia::ptref::ast::BinaryOp<navitia::ptref::ast::Or>,
    (navitia::ptref::ast::Expr, lhs)
    (navitia::ptref::ast::Expr, rhs)
)
