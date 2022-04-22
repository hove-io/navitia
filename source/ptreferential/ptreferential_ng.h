/* Copyright © 2001-2022, Hove and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Hove (www.hove.com).
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
channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#pragma once

#include "utils/exception.h"
#include "type/type_interfaces.h"
#include "type/rt_level.h"
#include "type/data.h"

#include <boost/optional.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/fusion/adapted/struct.hpp>
#include <boost/variant/recursive_variant.hpp>
#include <utility>

namespace navitia {
namespace ptref {

type::Indexes make_query_ng(const type::Type_e requested_type,
                            const std::string& request,
                            const std::vector<std::string>& forbidden_uris,
                            const type::OdtLevel_e odt_level,
                            const boost::optional<boost::posix_time::ptime>& since,
                            const boost::optional<boost::posix_time::ptime>& until,
                            const type::RTLevel rt_level,
                            const type::Data& data,
                            const boost::optional<boost::posix_time::ptime>& current_datetime = boost::none);

namespace ast {

struct All {};

struct Empty {};

struct Fun {
    std::string type;
    std::string method;
    std::vector<std::string> args;
};

struct GetCorresponding;
struct And;
struct Or;
struct Diff;
template <typename OpTag>
struct BinaryOp;

struct Expr {
    typedef boost::variant<All,
                           Empty,
                           Fun,
                           boost::recursive_wrapper<GetCorresponding>,
                           boost::recursive_wrapper<BinaryOp<And>>,
                           boost::recursive_wrapper<BinaryOp<Or>>,
                           boost::recursive_wrapper<BinaryOp<Diff>>>
        type;

    Expr() : expr(All()) {}
    template <typename E>
    Expr(E expr) : expr(std::move(expr)) {}

    Expr& operator+=(Expr const& rhs);
    Expr& operator-=(Expr const& rhs);
    Expr& operator*=(Expr const& rhs);

    type expr;
};

struct GetCorresponding {
    std::string type;
    Expr expr;
};
template <typename OpTag>
struct BinaryOp {
    Expr lhs;
    Expr rhs;
    BinaryOp(Expr l, Expr r) : lhs(std::move(l)), rhs(std::move(r)) {}
};

void print_quoted_string(std::ostream& os, const std::string& s);
std::ostream& operator<<(std::ostream& os, const Expr& expr);
std::ostream& operator<<(std::ostream& os, const All&);
std::ostream& operator<<(std::ostream& os, const Empty&);
std::ostream& operator<<(std::ostream& os, const Fun& fun);
std::ostream& operator<<(std::ostream& os, const GetCorresponding& to_object);
std::ostream& operator<<(std::ostream& os, const BinaryOp<And>& op);
std::ostream& operator<<(std::ostream& os, const BinaryOp<Or>& op);
std::ostream& operator<<(std::ostream& os, const BinaryOp<Diff>& op);

}  // namespace ast

ast::Expr parse(const std::string& request);
std::string make_request(const type::Type_e requested_type,
                         const std::string& request,
                         const std::vector<std::string>& forbidden_uris,
                         const type::OdtLevel_e odt_level,
                         const boost::optional<boost::posix_time::ptime>& since,
                         const boost::optional<boost::posix_time::ptime>& until,
                         const type::RTLevel rt_level,
                         const type::Data& data,
                         const boost::optional<boost::posix_time::ptime>& current_datetime = boost::none);

}  // namespace ptref
}  // namespace navitia

BOOST_FUSION_ADAPT_STRUCT(navitia::ptref::ast::Fun,
                          (std::string, type)(std::string, method)(std::vector<std::string>, args))
BOOST_FUSION_ADAPT_STRUCT(navitia::ptref::ast::GetCorresponding, (std::string, type)(navitia::ptref::ast::Expr, expr))
BOOST_FUSION_ADAPT_STRUCT(navitia::ptref::ast::BinaryOp<navitia::ptref::ast::Diff>,
                          (navitia::ptref::ast::Expr, lhs)(navitia::ptref::ast::Expr, rhs))
BOOST_FUSION_ADAPT_STRUCT(navitia::ptref::ast::BinaryOp<navitia::ptref::ast::And>,
                          (navitia::ptref::ast::Expr, lhs)(navitia::ptref::ast::Expr, rhs))
BOOST_FUSION_ADAPT_STRUCT(navitia::ptref::ast::BinaryOp<navitia::ptref::ast::Or>,
                          (navitia::ptref::ast::Expr, lhs)(navitia::ptref::ast::Expr, rhs))
