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

#include "utils/init.h"
#include "routing/tests/routing_api_test_data.h"
#include "mock_kraken.h"

static boost::gregorian::date_period period(std::string beg, std::string end) {
    boost::gregorian::date start_date = boost::gregorian::from_undelimited_string(beg);
    boost::gregorian::date end_date = boost::gregorian::from_undelimited_string(end);  // end is not in the period
    return {start_date, end_date};
}

struct data_set {
    data_set(std::string date,
             std::string publisher_name,
             std::string timezone_name,
             navitia::type::TimeZoneHandler::dst_periods timezones)
        : b(
            date,
            [](ed::builder& b) {
                // Add calendar
                navitia::type::Calendar* wednesday_cal{
                    new navitia::type::Calendar(b.data->meta->production_date.begin())};
                wednesday_cal->name = "the wednesday calendar";
                wednesday_cal->uri = "wednesday";
                wednesday_cal->active_periods.push_back(period("20140101", "20140911"));
                wednesday_cal->week_pattern = std::bitset<7>{"0010000"};
                for (int i = 1; i <= 3; ++i) {
                    navitia::type::ExceptionDate exd;
                    exd.date = boost::gregorian::date(2014, i, 10 + i);  // random date for the exceptions
                    exd.type = navitia::type::ExceptionDate::ExceptionType::sub;
                    wednesday_cal->exceptions.push_back(exd);
                }
                b.data->pt_data->calendars.push_back(wednesday_cal);

                navitia::type::Calendar* monday_cal{new navitia::type::Calendar(b.data->meta->production_date.begin())};
                monday_cal->name = "the monday calendar";
                monday_cal->uri = "monday";
                monday_cal->active_periods.push_back(period("20140105", "20140911"));
                monday_cal->week_pattern = std::bitset<7>{"1000000"};
                for (int i = 1; i <= 3; ++i) {
                    navitia::type::ExceptionDate exd;
                    exd.date = boost::gregorian::date(2014, i + 3, 5 + i);  // random date for the exceptions
                    exd.type = navitia::type::ExceptionDate::ExceptionType::sub;
                    monday_cal->exceptions.push_back(exd);
                }
                b.data->pt_data->calendars.push_back(monday_cal);

                // Commercial modes
                b.data->pt_data->get_or_create_commercial_mode("Commercial_mode:Car", "Car");

                // add lines
                b.sa("stop_area:stop1", 9, 9, false, true)("stop_area:stop1", 9, 9);
                b.sa("stop_area:stop2", 10, 10, false, true)("stop_area:stop2", 10, 10);
                b.sa("stop_area:stop3", 11, 11, false, true)("stop_area:stop3", 11, 11);
                b.vj_with_network("network:A", "line:A", "", "", true, "vj1", "", "", "physical_mode:Car")(
                    "stop_area:stop1", 10 * 3600 + 15 * 60, 10 * 3600 + 15 * 60)("stop_area:stop2", 11 * 3600 + 10 * 60,
                                                                                 11 * 3600 + 10 * 60);
                b.lines["line:A"]->calendar_list.push_back(wednesday_cal);
                b.lines["line:A"]->calendar_list.push_back(monday_cal);

                // we add a stop area with a strange name (with space and special char)
                b.sa("stop_with name bob \" , é", 20, 20);
                b.vj_with_network("network:B", "line:B")
                    .physical_mode("physical_mode:Car")("stop_point:stop_with name bob \" , é", "8:00"_t)(
                        "stop_area:stop1", "9:00"_t);

                // add a line with a unicode name
                b.vj_with_network("network:CaRoule", "line:Ça roule")
                    .name("vj_b")("stop_area:stop2", 10 * 3600 + 15 * 60, 10 * 3600 + 15 * 60)(
                        "stop_area:stop1", 11 * 3600 + 10 * 60, 11 * 3600 + 10 * 60);
                b.lines["line:Ça roule"]->commercial_mode = nullptr;  // remove commercial_mode to allow label testing

                // add a line with a vehicle_journey having a middle stop with skipped_stop
                b.vj_with_network("network:C", "line:C")
                    .physical_mode("physical_mode:Car")("stop_area:stop1", 10 * 3600 + 15 * 60, 10 * 3600 + 15 * 60)(
                        "stop_area:stop2", 11 * 3600 + 15 * 60, 11 * 3600 + 15 * 60,
                        std::numeric_limits<uint16_t>::max(), false, false, 0, 0,
                        true)("stop_area:stop2", 12 * 3600 + 15 * 60, 12 * 3600 + 15 * 60);

                // add a mock shape
                b.lines["line:A"]->shape = {{{1, 2}, {2, 2}, {4, 5}}, {{10, 20}, {20, 20}, {40, 50}}};

                for (auto r : b.lines["line:A"]->route_list) {
                    r->shape = {{{1, 2}, {2, 2}, {4, 5}}, {{10, 20}, {20, 20}, {40, 50}}};
                    r->destination = b.sas.find("stop_area:stop2")->second;
                }
                for (auto r : b.lines["line:B"]->route_list) {
                    r->destination = b.sas.find("stop_area:stop1")->second;
                }
                for (auto r : b.lines["line:Ça roule"]->route_list) {
                    r->destination = b.sas.find("stop_area:stop1")->second;
                }

                b.frequency_vj("line:freq", "10:00:00"_t, "11:00:00"_t, "00:10:00"_t, "network:freq", "1")
                    .name("vj:freq")("stop_area:stop2", "10:00:00"_t, "10:00:00"_t)("stop_area:stop1", "10:05:00"_t,
                                                                                    "10:05:00"_t);

                // Company added
                auto* cmp1 = new navitia::type::Company();
                cmp1->line_list.push_back(b.lines["line:A"]);
                b.data->pt_data->companies.push_back(cmp1);
                cmp1->idx = b.data->pt_data->companies.size();
                cmp1->name = "CMP1";
                cmp1->uri = "CMP1";
                b.lines["line:A"]->company_list.push_back(cmp1);
                auto* cmp2 = new navitia::type::Company();
                cmp2->line_list.push_back(b.lines["line:A"]);
                b.data->pt_data->companies.push_back(cmp2);
                cmp2->idx = b.data->pt_data->companies.size();
                cmp2->name = "CMP2";
                cmp2->uri = "CMP2";
                b.lines["line:A"]->company_list.push_back(cmp2);
                b.lines["line:A"]->text_color = "FFD700";

                b.data->pt_data->codes.add(b.data->pt_data->companies[0], "cmp1_code_key_0", "cmp1_code_value_0");
                b.data->pt_data->codes.add(b.data->pt_data->companies[0], "cmp1_code_key_1", "cmp1_code_value_1");
                b.data->pt_data->codes.add(b.data->pt_data->companies[0], "cmp1_code_key_2", "cmp1_code_value_2");
                b.data->pt_data->codes.add(b.data->pt_data->companies[1], "cmp2_code_key_0", "cmp2_code_value_0");
                b.data->pt_data->codes.add(b.lines["line:A"], "external_code", "A");
                b.data->pt_data->codes.add(b.lines["line:A"], "codeB", "B");
                b.data->pt_data->codes.add(b.lines["line:A"], "codeB", "Bise");
                b.data->pt_data->codes.add(b.lines["line:A"], "codeC", "C");
                b.data->pt_data->codes.add(b.sps.at("stop_area:stop1"), "code_uic", "bobette");
                b.data->pt_data->codes.add(b.sps.at("stop_area:stop1"), "external_code", "stop1_code");
                b.data->pt_data->codes.add(b.lines["line:A"]->network, "external_code", "A");
                b.data->pt_data->codes.add(b.lines["line:B"]->network, "external_code", "B");

                b.data->build_uri();

                navitia::type::VehicleJourney* vj = b.data->pt_data->vehicle_journeys_map["vehicle_journey:vj1"];
                vj->base_validity_pattern()->add(boost::gregorian::from_undelimited_string("20140101"),
                                                 boost::gregorian::from_undelimited_string("20140111"),
                                                 monday_cal->week_pattern);
                vj->company = cmp1;

                // we add some comments
                auto& comments = b.data->pt_data->comments;
                comments.add(b.data->pt_data->routes_map["line:A:0"],
                             nt::Comment("I'm a happy comment", "information"));
                comments.add(b.lines["line:A"], nt::Comment("I'm a happy comment", "information"));
                comments.add(b.sas["stop_area:stop1"], nt::Comment("comment on stop A", "information"));
                comments.add(b.sas["stop_area:stop1"], nt::Comment("the stop is sad", "information"));
                comments.add(b.data->pt_data->stop_points_map["stop_area:stop2"],
                             nt::Comment("hello bob", "information"));
                comments.add(b.data->pt_data->vehicle_journeys[0], nt::Comment("hello", "information"));
                comments.add(b.data->pt_data->vehicle_journeys[0]->stop_time_list.front(),
                             nt::Comment("stop time is blocked", "on_demand_transport"));
                // Disruption on stoparea
                using btp = boost::posix_time::time_period;
                b.impact(nt::RTLevel::RealTime, "Disruption 1")
                    .severity(nt::disruption::Effect::UNKNOWN_EFFECT)
                    .msg("Disruption on StopArea stop_area:stop1", nt::disruption::ChannelType::email)
                    .on(nt::Type_e::StopArea, "stop_area:stop1", *b.data->pt_data)
                    .application_periods(btp("20140101T000000"_dt, "20140120T235959"_dt))
                    .publish(btp("20140101T000000"_dt, "20140120T235959"_dt));
                // LineGroup added
                auto* lg = new navitia::type::LineGroup();
                lg->name = "A group";
                lg->uri = "group:A";
                lg->main_line = b.lines["line:A"];
                lg->line_list.push_back(b.lines["line:A"]);
                b.lines["line:A"]->line_group_list.push_back(lg);
                comments.add(lg, nt::Comment("I'm a happy comment", "information"));
                b.data->pt_data->line_groups.push_back(lg);

                b.impact(nt::RTLevel::RealTime, "Disruption On line:A")
                    .severity(nt::disruption::Effect::UNKNOWN_EFFECT)
                    .msg("Disruption on Line line:A", nt::disruption::ChannelType::email)
                    .on(nt::Type_e::Line, "line:A", *b.data->pt_data)
                    .application_periods(btp("20140101T000000"_dt, "20140120T235959"_dt))
                    .publish(btp("20140101T000000"_dt, "20140120T235959"_dt));

                // contributor "c1" contains dataset "d1"
                auto* contributor = new navitia::type::Contributor();
                contributor->idx = b.data->pt_data->contributors.size();
                contributor->uri = "c1";
                contributor->name = "name-c1";
                contributor->website = "ws-c1";
                contributor->license = "ls-c1";
                b.data->pt_data->contributors.push_back(contributor);

                auto* dataset = new navitia::type::Dataset();
                dataset->idx = b.data->pt_data->datasets.size();
                dataset->uri = "d1";
                dataset->name = "name-d1";
                dataset->desc = "desc-d1";
                dataset->system = "sys-d1";
                dataset->validation_period = period("20160101", "20161230");

                dataset->contributor = contributor;
                contributor->dataset_list.insert(dataset);
                b.data->pt_data->datasets.push_back(dataset);

                // Link between dataset and vehicle_journey
                vj = b.data->pt_data->vehicle_journeys.back();
                vj->dataset = dataset;
                dataset->vehiclejourney_list.insert(vj);
                b.data->complete();
            },
            true,
            publisher_name,
            timezone_name,
            timezones) {}

    ed::builder b;
};

int main(int argc, const char* const argv[]) {
    navitia::init_app();

    // Date and Time zone
    const std::string date = "20140101";
    const std::string publisher_name = "canal tp";
    const std::string timezone_name = "Europe/Paris";
    const auto begin = boost::gregorian::date_from_iso_string(date);
    const boost::gregorian::date_period production_date = {begin, begin + boost::gregorian::years(1)};
    navitia::type::TimeZoneHandler::dst_periods timezones = {{+3600, {production_date}}};

    data_set data(date, publisher_name, timezone_name, timezones);

    mock_kraken kraken(data.b, argc, argv);

    return 0;
}
