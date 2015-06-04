
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

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ptref_companies_test
#include <boost/test/unit_test.hpp>
#include "ptreferential/ptreferential.h"
#include "ptreferential/reflexion.h"
#include "ptreferential/ptref_graph.h"
#include "ed/build_helper.h"

#include <boost/graph/strong_components.hpp>
#include <boost/graph/connected_components.hpp>
#include "type/pt_data.h"


namespace navitia{namespace ptref {
template<typename T> std::vector<type::idx_t> get_indexes(Filter filter,  Type_e requested_type, const type::Data & d);
}}

using namespace navitia::ptref;

struct logger_initialized {
    logger_initialized()   { init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized )

class Params{
public:
    navitia::type::Data data;
    navitia::type::Company* current_company;
    navitia::type::Line* current_line;
    void add_company(const std::string& company_name){
        current_company = new navitia::type::Company();
        current_company->uri = company_name;
        current_company->name = company_name;
        current_company->idx = data.pt_data->companies.size();
        data.pt_data->companies.push_back(current_company);
    }
    void add_line(const std::string& ln_name){
        current_line = new navitia::type::Line();
        current_line->uri = ln_name;
        current_line->name = ln_name;
        current_line->idx = data.pt_data->lines.size();
        data.pt_data->lines.push_back(current_line);
    }


    Params(){
        add_company("C1");
        add_line("C1-L1");
        current_company->line_list.push_back(current_line);
        current_line->company_list.push_back(current_company);
        add_line("C1-L2");
        current_company->line_list.push_back(current_line);
        current_line->company_list.push_back(current_company);


        add_company("C2");
        add_line("C2-L1");
        current_company->line_list.push_back(current_line);
        current_line->company_list.push_back(current_company);
        add_line("C2-L2");
        current_company->line_list.push_back(current_line);
        current_line->company_list.push_back(current_company);

        add_line("C2-L3");
        current_company->line_list.push_back(current_line);
        current_line->company_list.push_back(current_company);

        data.pt_data->build_uri();
        }
};
BOOST_FIXTURE_TEST_SUITE(companies_test, Params)

BOOST_AUTO_TEST_CASE(comanies_list) {
    auto indexes = make_query(navitia::type::Type_e::Company, "", data);
    BOOST_CHECK_EQUAL(indexes.size(), 2);
}

BOOST_AUTO_TEST_CASE(comany_by_uri) {
    auto indexes = make_query(navitia::type::Type_e::Company, "company.uri=C1", data);
    BOOST_CHECK_EQUAL(indexes.size(), 1);
}

BOOST_AUTO_TEST_CASE(lines_by_company) {
    auto indexes = make_query(navitia::type::Type_e::Line, "company.uri=C1", std::vector<std::string>(), navitia::type::OdtLevel_e::all, data);
    BOOST_CHECK_EQUAL(indexes.size(), 2);

    indexes = make_query(navitia::type::Type_e::Line, "company.uri=C2", std::vector<std::string>(), navitia::type::OdtLevel_e::all, data);
    BOOST_CHECK_EQUAL(indexes.size(), 3);
}

BOOST_AUTO_TEST_CASE(comany_line) {
    auto indexes = make_query(navitia::type::Type_e::Company, "line.uri=C1-L1", data);
    BOOST_CHECK_EQUAL(indexes.size(), 1);
}



BOOST_AUTO_TEST_SUITE_END()
