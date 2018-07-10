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

#include "ptreferential_ng.h"
#include "ptreferential.h"

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

namespace navitia{
namespace ptref{

namespace {

template <typename Iterator>
struct PtRefGrammar: qi::grammar<Iterator, ast::Expr(), ascii::space_type>
{
    PtRefGrammar(): PtRefGrammar::base_type(expr) {
        using namespace qi::labels;
        using boost::spirit::_1;
        using boost::spirit::_2;
        namespace phx = boost::phoenix;

        pred = all | empty | fun | cmp | dwithin;
        all = qi::lit("all")[_val = phx::construct<ast::All>()];
        empty = qi::lit("empty")[_val = phx::construct<ast::Empty>()];
        fun = ident >> '.' >> ident >> '(' >> -(str % ',') >> ')';
        cmp = ident[phx::at_c<0>(_val) = _1]
            >> '.'
            >> ident[phx::at_c<1>(_val) = _1]
            >> '='
            >> str[phx::push_back(phx::at_c<2>(_val), _1)];
        dwithin = ident[phx::at_c<0>(_val) = _1]
            >> '.'
            >> "coord"
            >> qi::lit("DWITHIN")[phx::at_c<1>(_val) = "within"]
            >> '('
            >> str[phx::push_back(phx::at_c<2>(_val), ""),
                   phx::push_back(phx::at_c<2>(_val), _1)]
            >> ','
            >> str[phx::back(phx::at_c<2>(_val)) += ";",
                   phx::back(phx::at_c<2>(_val)) += _1]
            >> ','
            >> str[phx::at(phx::at_c<2>(_val), 0) = _1]
            >> ')';

        to_object = (qi::lit("get") | "GET") >> ident >> "<-" >> expr;
        expr_leaf = '(' >> expr >> ')' | pred | to_object;
        expr_diff = (expr_leaf >> '-' >> expr_diff)
            [_val = phx::construct<ast::BinaryOp<ast::Diff>>(_1, _2)]
            | expr_leaf[_val = _1];
        expr_and = (expr_diff >> (qi::lit("AND") | qi::lit("and")) >> expr_and)
            [_val = phx::construct<ast::BinaryOp<ast::And>>(_1, _2)]
            | expr_diff[_val = _1];
        expr_or = (expr_and >> (qi::lit("OR") | qi::lit("or")) >> expr_or)
            [_val = phx::construct<ast::BinaryOp<ast::Or>>(_1, _2)]
            | expr_and[_val = _1];
        expr = expr_or;

        ident = qi::lexeme[qi::alpha >> *(qi::alnum | qi::char_("_"))];
        str = qi::lexeme[+(qi::alnum | qi::char_("_.:;|-"))]
            | qi::lexeme['"' > *(qi::char_ - qi::char_("\"\\") | ('\\' > qi::char_)) > '"'];
    }

    // Pred
    qi::rule<Iterator, ast::Pred(), ascii::space_type> pred;
    qi::rule<Iterator, ast::All(), ascii::space_type> all;
    qi::rule<Iterator, ast::Empty(), ascii::space_type> empty;
    qi::rule<Iterator, ast::Fun(), ascii::space_type> fun;
    qi::rule<Iterator, ast::Fun(), ascii::space_type> cmp;
    qi::rule<Iterator, ast::Fun(), ascii::space_type> dwithin;

    // Expr
    qi::rule<Iterator, ast::ToObject(), ascii::space_type> to_object;
    qi::rule<Iterator, ast::Expr(), ascii::space_type> expr_leaf;
    qi::rule<Iterator, ast::Expr(), ascii::space_type> expr_and;
    qi::rule<Iterator, ast::Expr(), ascii::space_type> expr_or;
    qi::rule<Iterator, ast::Expr(), ascii::space_type> expr_diff;
    qi::rule<Iterator, ast::Expr(), ascii::space_type> expr;

    // lexemes
    qi::rule<Iterator, std::string(), ascii::space_type> ident;
    qi::rule<Iterator, std::string(), ascii::space_type> str;
};

} // anonymous namespace

ast::Expr parse(const std::string& request) {
    using boost::spirit::ascii::space;
    static const PtRefGrammar<std::string::const_iterator> grammar;

    ast::Expr expr;
    auto iter = request.begin(), end = request.end();
    if (phrase_parse(iter, end, grammar, space, expr)) {
        if(iter != end) {
            const std::string unparsed(iter, end);
            throw parsing_error(parsing_error::partial_error, "Filter: Unable to parse the whole string. Not parsed: >>" + unparsed + "<<");
        }
    } else {
        throw parsing_error(parsing_error::global_error, "Filter: unable to parse " + request);
    }
    return expr;
}

static std::string make_request(const type::Type_e requested_type,
                                const std::string& request,
                                const std::vector<std::string>& forbidden_uris,
                                const type::OdtLevel_e odt_level,
                                const boost::optional<boost::posix_time::ptime>& since,
                                const boost::optional<boost::posix_time::ptime>& until,
                                const type::Data& data) {
    std::string res = request.empty() ? std::string("all") : request;
    // TODO: implement the other args (generating warnings)
        
    return res;
}

type::Indexes make_query_ng(const type::Type_e requested_type,
                            const std::string& request,
                            const std::vector<std::string>& forbidden_uris,
                            const type::OdtLevel_e odt_level,
                            const boost::optional<boost::posix_time::ptime>& since,
                            const boost::optional<boost::posix_time::ptime>& until,
                            const type::Data& data) {
    auto logger = log4cplus::Logger::getInstance("logger");
    const auto request_ng = make_request(requested_type, request, forbidden_uris, odt_level, since, until, data);
    const auto expr = parse(request_ng);
    LOG4CPLUS_DEBUG(logger, "ptref_ng parsed: " << expr);
    return type::Indexes();
}

}} // namespace navitia::ptref
