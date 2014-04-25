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

#include "utils/init.h"
#include "routing/tests/routing_api_test_data.h"
#include "mock_kraken.h"
#include "type/type.h"

boost::gregorian::date_period period(std::string beg, std::string end) {
    boost::gregorian::date start_date = boost::gregorian::from_undelimited_string(beg);
    boost::gregorian::date end_date = boost::gregorian::from_undelimited_string(end); //end is not in the period
    return {start_date, end_date};
}

struct data_set {

    data_set() : b("20140101") {
        //add calendar
        navitia::type::Calendar* wednesday_cal {new navitia::type::Calendar(b.data->meta->production_date.begin())};
        wednesday_cal->name = "the wednesday calendar";
        wednesday_cal->uri = "wednesday";
        wednesday_cal->active_periods.push_back(period("20140101", "20140911"));
        wednesday_cal->week_pattern = std::bitset<7>{"0010000"};
        for (int i = 1; i <= 3; ++i) {
            navitia::type::ExceptionDate exd;
            exd.date = boost::gregorian::date(2014, i, 10+i); //random date for the exceptions
            exd.type = navitia::type::ExceptionDate::ExceptionType::sub;
            wednesday_cal->exceptions.push_back(exd);
        }
        b.data->pt_data->calendars.push_back(wednesday_cal);

        navitia::type::Calendar* monday_cal {new navitia::type::Calendar(b.data->meta->production_date.begin())};
        monday_cal->name = "the monday calendar";
        monday_cal->uri = "monday";
        monday_cal->active_periods.push_back(period("20140105", "20140911"));
        monday_cal->week_pattern = std::bitset<7>{"1000000"};
        for (int i = 1; i <= 3; ++i) {
            navitia::type::ExceptionDate exd;
            exd.date = boost::gregorian::date(2014, i+3, 5+i); //random date for the exceptions
            exd.type = navitia::type::ExceptionDate::ExceptionType::sub;
            monday_cal->exceptions.push_back(exd);
        }
        b.data->pt_data->calendars.push_back(monday_cal);

        //add lines
        b.vj("network:R", "line:A", "", "", true, "vj1")
                ("stop_area:stop1", 10 * 3600 + 15 * 60, 10 * 3600 + 15 * 60)
                ("stop_area:stop2", 11 * 3600 + 10 * 60 ,11 * 3600 + 10 * 60);
        b.lines["line:A"]->calendar_list.push_back(wednesday_cal);
        b.lines["line:A"]->calendar_list.push_back(monday_cal);
        b.data->build_uri();

        navitia::type::VehicleJourney* vj = b.data->pt_data->vehicle_journeys_map["vj1"];
        vj->validity_pattern->add(boost::gregorian::from_undelimited_string("20140101"),
                                  boost::gregorian::from_undelimited_string("20140111"), monday_cal->week_pattern);

        b.data->complete();
    }

    ed::builder b;
};

int main() {
    navitia::init_app();

    data_set data;

    mock_kraken kraken(data.b, "main_ptref_test");

    return 0;
}
