#include "type/message.h"

#include <log4cplus/logger.h>

#include <boost/format.hpp>

namespace pt = boost::posix_time;
namespace bg = boost::gregorian;


namespace navitia { namespace type {

MessageHolder& MessageHolder::operator=(const navitia::type::MessageHolder&& other){
    this->messages = std::move(other.messages);
    return *this;
}


bool Message::is_valid(const boost::posix_time::ptime& now, const boost::posix_time::time_period& action_period) const{
    if(now.is_not_a_date_time() && action_period.is_null()){
        return false;
    }

    bool to_return = is_publishable(now);

    if(!action_period.is_null()){
        to_return = to_return && is_applicable(action_period);
    }
    return to_return;
}

bool Message::is_publishable(const boost::posix_time::ptime& time) const{
    return publication_period.contains(time);
}

bool AtPerturbation::is_applicable(const boost::posix_time::time_period& period) const{
    bool days_intersects = false;

    //intersection de la period ou se déroule l'action et de la periode de validité de la perturbation
    pt::time_period common_period = period.intersection(application_period);

    //si la periode commune est null, l'impact n'est pas valide
    if(common_period.is_null()){
        return false;
    }

    //on doit travailler jour par jour
    //
    bg::date current_date = common_period.begin().date();

    while(!days_intersects && current_date <= common_period.end().date()){
        //on test uniquement le debut de la period, si la fin est valide, elle sera testé à la prochaine itération
        days_intersects = valid_day_of_week(current_date);

        //vérification des plages horaires journaliéres
        pt::time_period current_period = common_period.intersection(pt::time_period(pt::ptime(current_date, pt::seconds(0)), pt::hours(24)));
        days_intersects = days_intersects && valid_hour_perturbation(current_period);

        current_date += bg::days(1);
    }


    return days_intersects;

}

bool AtPerturbation::valid_hour_perturbation(const pt::time_period& period) const{
    pt::time_period daily_period(pt::ptime(period.begin().date(), application_daily_start_hour),
            pt::ptime(period.begin().date(), application_daily_end_hour));

    return period.intersects(daily_period);

}

bool AtPerturbation::valid_day_of_week(const bg::date& date) const{

    switch(date.day_of_week()){
        case bg::Monday: return this->active_days[0];
        case bg::Tuesday: return this->active_days[1];
        case bg::Wednesday: return this->active_days[2];
        case bg::Thursday: return this->active_days[3];
        case bg::Friday: return this->active_days[4];
        case bg::Saturday: return this->active_days[5];
        case bg::Sunday: return this->active_days[6];
    }
    return false; // Ne devrait pas arriver
}


}}//namespace
