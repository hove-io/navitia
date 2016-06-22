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

static boost::gregorian::date_period period(std::string beg, std::string end) {
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
        b.sa("stop_area:stop1", 9, 9, false, true)("stop_area:stop1", 9, 9);
        b.sa("stop_area:stop2", 10, 10, false, true)("stop_area:stop2", 10, 10);
        b.vj("line:A", "", "", true, "vj1", "", "physical_mode:Car")
                ("stop_area:stop1", 10 * 3600 + 15 * 60, 10 * 3600 + 15 * 60)
                ("stop_area:stop2", 11 * 3600 + 10 * 60, 11 * 3600 + 10 * 60);
        b.lines["line:A"]->calendar_list.push_back(wednesday_cal);
        b.lines["line:A"]->calendar_list.push_back(monday_cal);

        // we add a stop area with a strange name (with space and special char)
        b.sa("stop_with name bob \" , é", 20, 20);
        b.vj("line:B", "", "", true, "vj_b", "", "physical_mode:Car")
                ("stop_point:stop_with name bob \" , é", "8:00"_t)("stop_area:stop1", "9:00"_t);

        // add a line with a unicode name
        b.vj("line:Ça roule", "11111111", "", true, "vj_b")
                ("stop_area:stop2", 10 * 3600 + 15 * 60, 10 * 3600 + 15 * 60)
                ("stop_area:stop1", 11 * 3600 + 10 * 60, 11 * 3600 + 10 * 60);

        //add a mock shape
        b.lines["line:A"]->shape = {
                                    {{1,2}, {2,2}, {4,5}},
                                    {{10,20}, {20,20}, {40,50}}
                                   };

        // Commercial modes
        navitia::type::CommercialMode* cm = new navitia::type::CommercialMode();
        cm->uri = "Commercial_mode:Car";
        cm->name = "Car";
        b.data->pt_data->commercial_modes.push_back(cm);
        b.lines["line:A"]->commercial_mode = cm;

        for (auto r: b.lines["line:A"]->route_list) {
            r->shape = {
                {{1,2}, {2,2}, {4,5}},
                {{10,20}, {20,20}, {40,50}}
            };
            r->destination = b.sas.find("stop_area:stop2")->second;
        }
        for (auto r: b.lines["line:B"]->route_list) {
            r->destination = b.sas.find("stop_area:stop1")->second;
        }
        for (auto r: b.lines["line:Ça roule"]->route_list) {
            r->destination = b.sas.find("stop_area:stop1")->second;
        }
        b.data->pt_data->codes.add(b.lines["line:A"], "external_code", "A");
        b.data->pt_data->codes.add(b.lines["line:A"], "codeB", "B");
        b.data->pt_data->codes.add(b.lines["line:A"], "codeB", "Bise");
        b.data->pt_data->codes.add(b.lines["line:A"], "codeC", "C");
        b.data->pt_data->codes.add(b.sps.at("stop_area:stop1"), "code_uic", "bobette");

        b.data->build_uri();

        navitia::type::VehicleJourney* vj = b.data->pt_data->vehicle_journeys_map["vj1"];
        vj->base_validity_pattern()->add(boost::gregorian::from_undelimited_string("20140101"),
                                  boost::gregorian::from_undelimited_string("20140111"), monday_cal->week_pattern);

        //we add some comments
        auto& comments = b.data->pt_data->comments;
        comments.add(b.data->pt_data->routes_map["line:A:0"], "I'm a happy comment");
        comments.add(b.lines["line:A"], "I'm a happy comment");
        comments.add(b.sas["stop_area:stop1"], "comment on stop A");
        comments.add(b.sas["stop_area:stop1"], "the stop is sad");
        comments.add(b.data->pt_data->stop_points_map["stop_area:stop2"], "hello bob");
        comments.add(b.data->pt_data->vehicle_journeys[0], "hello");
        comments.add(b.data->pt_data->vehicle_journeys[0]->stop_time_list.front(),
                                      "stop time is blocked");
        // Disruption on stoparea
        using btp = boost::posix_time::time_period;
        b.impact(nt::RTLevel::RealTime, "Disruption 1")
                .severity(nt::disruption::Effect::UNKNOWN_EFFECT)
                .msg("Disruption on StopArea stop_area:stop1", nt::disruption::ChannelType::email)
                .on(nt::Type_e::StopArea, "stop_area:stop1")
                .application_periods(btp("20140101T000000"_dt, "20140120T235959"_dt))
                .publish(btp("20140101T000000"_dt, "20140120T235959"_dt));
       // Company added
        navitia::type::Company* cmp = new navitia::type::Company();
        cmp->line_list.push_back(b.lines["line:A"]);
        vj->company = cmp;
        b.data->pt_data->companies.push_back(cmp);
        cmp->idx = b.data->pt_data->companies.size();
        cmp->name = "CMP1";
        cmp->uri = "CMP1";
        b.lines["line:A"]->company_list.push_back(cmp);

        b.lines["line:A"]->text_color = "FFD700";
        // LineGroup added
        navitia::type::LineGroup* lg = new navitia::type::LineGroup();
        lg->name = "A group";
        lg->uri = "group:A";
        lg->main_line = b.lines["line:A"];
        lg->line_list.push_back(b.lines["line:A"]);
        b.lines["line:A"]->line_group_list.push_back(lg);
        comments.add(lg, "I'm a happy comment");
        b.data->pt_data->line_groups.push_back(lg);

        b.impact(nt::RTLevel::RealTime, "Disruption On line:A")
                .severity(nt::disruption::Effect::UNKNOWN_EFFECT)
                .msg("Disruption on Line line:A", nt::disruption::ChannelType::email)
                .on(nt::Type_e::Line, "line:A")
                .application_periods(btp("20140101T000000"_dt, "20140120T235959"_dt))
                .publish(btp("20140101T000000"_dt, "20140120T235959"_dt));

        //contributor "c1" contains dataset "d1"
        navitia::type::Contributor* contributor = new navitia::type::Contributor();
        contributor->idx = b.data->pt_data->contributors.size();
        contributor->uri = "c1";
        contributor->name = "name-c1";
        contributor->website = "ws-c1";
        contributor->license = "ls-c1";
        b.data->pt_data->contributors.push_back(contributor);

        navitia::type::Dataset* dataset = new navitia::type::Dataset();
        dataset->idx = b.data->pt_data->datasets.size();
        dataset->uri = "d1";
        dataset->name = "name-d1";
        dataset->desc = "desc-d1";
        dataset->system = "sys-d1";
        dataset->validation_period = period("20160101", "20161230");

        dataset->contributor = contributor;
        contributor->dataset_list.insert(dataset);
        b.data->pt_data->datasets.push_back(dataset);

        //Link between dataset and vehicle_journey
        vj = b.data->pt_data->vehicle_journeys.back();
        vj->dataset = dataset;
        dataset->vehiclejourney_list.insert(vj);

        b.data->complete();
        b.data->build_raptor();
    }

    ed::builder b;
};

int main(int argc, const char* const argv[]) {
    navitia::init_app();

    data_set data;

    mock_kraken kraken(data.b, "main_ptref_test", argc, argv);

    return 0;
}
