#include "parse_request.h"
#include "ptreferential/ptreferential.h"

namespace navitia { namespace timetables {
request_parser::request_parser(const std::string &API, const std::string &request, const std::string &str_dt, const std::string &str_max_dt,
               const int nb_departures, const type::Data & data) {
    boost::posix_time::ptime ptime;
    ptime = boost::posix_time::from_iso_string(str_dt);
    date_time = routing::DateTime((ptime.date() - data.meta.production_date.begin()).days(), ptime.time_of_day().total_seconds());

    if((nb_departures == std::numeric_limits<int>::max()) && str_max_dt == "") {
        pb_response.set_error(API+" : Un des deux champs nb_departures ou max_datetime doit être renseigné");
    } else {
        if(str_max_dt != "") {
            ptime = boost::posix_time::from_iso_string(str_max_dt);
            max_datetime = routing::DateTime((ptime.date() - data.meta.production_date.begin()).days(), ptime.time_of_day().total_seconds());
        }

        if(request!= "") {
            try {
                route_points = navitia::ptref::make_query(type::Type_e::eRoutePoint, request, data);
            } catch(ptref::ptref_parsing_error parse_error) {
                switch(parse_error.type){
                case ptref::ptref_parsing_error::error_type::partial_error: pb_response.set_error(API+" / PTReferential : On n'a pas réussi à parser toute la requête. Non-interprété : >>" + parse_error.more + "<<"); break;
                case ptref::ptref_parsing_error::error_type::global_error: pb_response.set_error(API+" / PTReferential : Impossible de parser la requête");
                case ptref::ptref_parsing_error::error_type::unknown_object: pb_response.set_error(API+"Objet NAViTiA inconnu : " + parse_error.more);
                }
            }
        }
    }
}

request_parser::request_parser(const std::string &API, const std::string &request, const std::string &str_dt, const std::string &change_time, const type::Data & data) {
    boost::posix_time::ptime ptime, maxptime;
    if(str_dt != "") {
        try {
            ptime = boost::posix_time::from_iso_string(str_dt);
        } catch(...) {
            pb_response.set_error(API+" / Probleme lors du parsage de datetime");
        }
    }
    else
        ptime = boost::posix_time::second_clock::local_time();

    ptime = ptime - ptime.time_of_day();
    if(change_time!= "") {
        std::string ctime = change_time;
        if(ctime.substr(0,1) == "T") {
            ctime = "19700101" + ctime;
            if(ctime.size() == 3)
                ctime += "00";
        }


        boost::posix_time::ptime pchange_time;
        try {
            pchange_time = boost::posix_time::from_iso_string(ctime);
        } catch(...) {
            pb_response.set_error(API+" / Probleme lors du parsage de changetime");
        }

        ptime = ptime + pchange_time.time_of_day();
    }
    date_time = routing::DateTime((ptime.date() - data.meta.production_date.begin()).days(), ptime.time_of_day().total_seconds());

    maxptime = ptime + boost::posix_time::time_duration(24,0,0);
    max_datetime = routing::DateTime((maxptime.date() - data.meta.production_date.begin()).days(), maxptime.time_of_day().total_seconds());


    if(request!= "") {
        try {
            route_points = navitia::ptref::make_query(type::Type_e::eRoutePoint, request, data);
        } catch(ptref::ptref_parsing_error parse_error) {
            switch(parse_error.type){
                case ptref::ptref_parsing_error::error_type::partial_error: pb_response.set_error(API+" / PTReferential : On n'a pas réussi à parser toute la requête. Non-interprété : >>" + parse_error.more + "<<"); break;
                case ptref::ptref_parsing_error::error_type::global_error: pb_response.set_error(API+" / PTReferential : Impossible de parser la requête");
                case ptref::ptref_parsing_error::error_type::unknown_object: pb_response.set_error(API+"Objet NAViTiA inconnu : " + parse_error.more);
                }
            }
    }


}
}}
