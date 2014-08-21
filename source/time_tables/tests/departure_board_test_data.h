#pragma once

#include "time_tables/departure_boards.h"
#include "ed/build_helper.h"

/**
 * Data set for departure board
 */
struct calendar_fixture {
    ed::builder b;
    boost::gregorian::date beg;
    boost::gregorian::date end_of_year;

    navitia::type::VehicleJourney* vj_week;
    navitia::type::VehicleJourney* vj_week_bis;
    navitia::type::VehicleJourney* vj_weekend;
    navitia::type::VehicleJourney* vj_all;
    navitia::type::VehicleJourney* vj_wednesday;
    calendar_fixture() : b("20120614") {
        //2 vj during the week
        b.vj("network:R", "line:A", "1", "", true, "week")("stop1", 10 * 3600, 10 * 3600 + 10 * 60)("stop2", 12 * 3600, 12 * 3600 + 10 * 60);
        b.vj("network:R", "line:A", "101", "", true, "week_bis")("stop1", 11 * 3600, 11 * 3600 + 10 * 60)("stop2", 14 * 3600, 14 * 3600 + 10 * 60);
        //NOTE: we give a first random validity pattern because the builder try to factorize them

        //only one on the week end
        b.vj("network:R", "line:A", "10101", "", true, "weekend")("stop1", 20 * 3600, 20 * 3600 + 10 * 60)("stop2", 21 * 3600, 21 * 3600 + 10 * 60);

        // and one everytime
        b.vj("network:R", "line:A", "1100101", "", true, "all")("stop1", 15 * 3600, 15 * 3600 + 10 * 60)("stop2", 16 * 3600, 16 * 3600 + 10 * 60);

        // and wednesday that will not be matched to any cal
        b.vj("network:R", "line:A", "110010011", "", true, "wednesday")("stop1", 17 * 3600, 17 * 3600 + 10 * 60)("stop2", 18 * 3600, 18 * 3600 + 10 * 60);

        b.data->build_uri();
        beg = b.data->meta->production_date.begin();
        end_of_year = beg + boost::gregorian::years(1) + boost::gregorian::days(1);

        vj_week = b.data->pt_data->vehicle_journeys_map["week"];
        vj_week->validity_pattern->add(beg, end_of_year, std::bitset<7>{"1111100"});
        vj_week_bis = b.data->pt_data->vehicle_journeys_map["week_bis"];
        vj_week_bis->validity_pattern->add(beg, end_of_year, std::bitset<7>{"1111100"});
        vj_weekend = b.data->pt_data->vehicle_journeys_map["weekend"];
        vj_weekend->validity_pattern->add(beg, end_of_year, std::bitset<7>{"0000011"});
        vj_all = b.data->pt_data->vehicle_journeys_map["all"];
        vj_all->validity_pattern->add(beg, end_of_year, std::bitset<7>{"1111111"});
        vj_wednesday = b.data->pt_data->vehicle_journeys_map["wednesday"];
        vj_wednesday->validity_pattern->add(beg, end_of_year, std::bitset<7>{"0010000"});

        //we now add 2 similar calendars
        auto week_cal = new navitia::type::Calendar(b.data->meta->production_date.begin());
        week_cal->uri = "week_cal";
        week_cal->active_periods.push_back({beg, end_of_year});
        week_cal->week_pattern = std::bitset<7>{"1111100"};
        b.data->pt_data->calendars.push_back(week_cal);

        auto weekend_cal = new navitia::type::Calendar(b.data->meta->production_date.begin());
        weekend_cal->uri = "weekend_cal";
        weekend_cal->active_periods.push_back({beg, end_of_year});
        weekend_cal->week_pattern = std::bitset<7>{"0000011"};
        b.data->pt_data->calendars.push_back(weekend_cal);

        auto not_associated_cal = new navitia::type::Calendar(b.data->meta->production_date.begin());
        not_associated_cal->uri = "not_associated_cal";
        not_associated_cal->active_periods.push_back({beg, end_of_year});
        not_associated_cal->week_pattern = std::bitset<7>{"0010000"};
        b.data->pt_data->calendars.push_back(not_associated_cal); //not associated to the line

        //both calendars are associated to the line
        b.lines["line:A"]->calendar_list.push_back(week_cal);
        b.lines["line:A"]->calendar_list.push_back(weekend_cal);

        b.data->build_uri();
        b.data->pt_data->index();
        b.data->build_raptor();
        b.data->geo_ref->init();
        b.data->complete();
    }
};
