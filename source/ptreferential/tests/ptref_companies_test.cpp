
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

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ptref_companies_test
#include <boost/test/unit_test.hpp>
#include "ptreferential/ptreferential_utils.h"
#include "ptreferential/ptreferential.h"
#include "ptreferential/ptref_graph.h"
#include "ed/build_helper.h"
#include "tests/utils_test.h"
#include "utils/logger.h"

#include <boost/graph/strong_components.hpp>
#include <boost/graph/connected_components.hpp>
#include "type/pt_data.h"

using namespace navitia::ptref;

struct logger_initialized {
    logger_initialized() { navitia::init_logger(); }
};
BOOST_GLOBAL_FIXTURE(logger_initialized);

class Params {
public:
    navitia::type::Data data;
    navitia::type::Company* current_company;
    navitia::type::Network* current_network;
    navitia::type::Line* current_line;
    navitia::type::Route* current_route;

    void add_network(const std::string& network_name) {
        current_network = new navitia::type::Network();
        current_network->uri = network_name;
        current_network->name = network_name;
        current_network->idx = data.pt_data->networks.size();
        data.pt_data->networks.push_back(current_network);
    }

    void add_company(const std::string& company_name) {
        current_company = new navitia::type::Company();
        current_company->uri = company_name;
        current_company->name = company_name;
        current_company->idx = data.pt_data->companies.size();
        data.pt_data->companies.push_back(current_company);
    }
    void add_line(const std::string& line_name) {
        current_line = new navitia::type::Line();
        current_line->uri = line_name;
        current_line->name = line_name;
        current_line->idx = data.pt_data->lines.size();
        data.pt_data->lines.push_back(current_line);
    }

    void add_route(const std::string& route_name) {
        current_route = new navitia::type::Route();
        current_route->uri = route_name;
        current_route->name = route_name;
        current_route->idx = data.pt_data->routes.size();
        data.pt_data->routes.push_back(current_route);
    }

    Params() {
        add_network("N1");
        add_company("C1");
        add_line("C1-L1");
        current_company->line_list.push_back(current_line);
        current_line->company_list.push_back(current_company);
        current_line->network = current_network;
        current_network->line_list.push_back(current_line);

        add_line("C1-L2");
        current_company->line_list.push_back(current_line);
        current_line->company_list.push_back(current_company);
        current_line->network = current_network;
        current_network->line_list.push_back(current_line);

        add_network("N2");
        add_company("C2");
        add_line("C2-L1");
        add_route("C2-L1-R1");
        current_company->line_list.push_back(current_line);
        current_line->company_list.push_back(current_company);
        current_line->network = current_network;
        current_network->line_list.push_back(current_line);
        current_line->route_list.push_back(current_route);
        current_route->line = current_line;

        add_line("C2-L2");
        add_route("C2-L1-R2");
        current_company->line_list.push_back(current_line);
        current_line->company_list.push_back(current_company);
        current_line->network = current_network;
        current_network->line_list.push_back(current_line);
        current_line->route_list.push_back(current_route);
        current_route->line = current_line;

        add_line("C2-L3");
        current_company->line_list.push_back(current_line);
        current_line->company_list.push_back(current_company);
        current_line->network = current_network;
        current_network->line_list.push_back(current_line);

        add_network("N3");
        data.pt_data->build_uri();
    }
};

BOOST_FIXTURE_TEST_SUITE(companies_test, Params)

BOOST_AUTO_TEST_CASE(comanies_list) {
    auto indexes = make_query(navitia::type::Type_e::Company, "", data);
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Company>(indexes, data), std::set<std::string>({"C1", "C2"}));
}

BOOST_AUTO_TEST_CASE(comany_by_uri) {
    auto indexes = make_query(navitia::type::Type_e::Company, "company.uri=C1", data);
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Company>(indexes, data), std::set<std::string>({"C1"}));
}

BOOST_AUTO_TEST_CASE(lines_by_company) {
    auto indexes = make_query(navitia::type::Type_e::Line, "company.uri=C1", data);
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Line>(indexes, data), std::set<std::string>({"C1-L1", "C1-L2"}));

    indexes = make_query(navitia::type::Type_e::Line, "company.uri=C2", data);
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Line>(indexes, data), std::set<std::string>({"C2-L1", "C2-L2", "C2-L3"}));
}

BOOST_AUTO_TEST_CASE(comanies_by_line) {
    auto indexes = make_query(navitia::type::Type_e::Company, "line.uri=C1-L1", data);
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Company>(indexes, data), std::set<std::string>({"C1"}));
}

BOOST_AUTO_TEST_CASE(networks_by_company) {
    auto indexes = make_query(navitia::type::Type_e::Network, "company.uri=C1", data);
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Network>(indexes, data), std::set<std::string>({"N1"}));

    indexes = make_query(navitia::type::Type_e::Network, "company.uri=C2", data);
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Network>(indexes, data), std::set<std::string>({"N2"}));
}

BOOST_AUTO_TEST_CASE(companies_by_network) {
    auto indexes = make_query(navitia::type::Type_e::Company, "network.uri=N1", data);
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Company>(indexes, data), std::set<std::string>({"C1"}));

    indexes = make_query(navitia::type::Type_e::Company, "network.uri=N2", data);
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Company>(indexes, data), std::set<std::string>({"C2"}));

    BOOST_CHECK_THROW(make_query(navitia::type::Type_e::Company, "network.uri=N3", data), ptref_error);
}

BOOST_AUTO_TEST_CASE(routes_by_company) {
    BOOST_CHECK_THROW(make_query(navitia::type::Type_e::Route, "company.uri=C1", data), ptref_error);

    auto indexes = make_query(navitia::type::Type_e::Route, "company.uri=C2", data);
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Route>(indexes, data), std::set<std::string>({"C2-L1-R1", "C2-L1-R2"}));
}

BOOST_AUTO_TEST_CASE(companies_by_route) {
    auto indexes = make_query(navitia::type::Type_e::Company, "route.uri=C2-L1-R1", data);
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Company>(indexes, data), std::set<std::string>({"C2"}));

    indexes = make_query(navitia::type::Type_e::Company, "route.uri=C2-L1-R2", data);
    BOOST_CHECK_EQUAL_RANGE(get_uris<nt::Company>(indexes, data), std::set<std::string>({"C2"}));
}

BOOST_AUTO_TEST_SUITE_END()
