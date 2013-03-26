#include "type/message.h"



#include <log4cplus/logger.h>

#include <boost/format.hpp>
#include <fstream>
#include <boost/iostreams/filtering_streambuf.hpp>

#include "third_party/eos_portable_archive/portable_iarchive.hpp"
#include "third_party/eos_portable_archive/portable_oarchive.hpp"
#include "lz4_filter/filter.h"
#include <boost/foreach.hpp>

namespace pt = boost::posix_time;
namespace bg = boost::gregorian;


namespace navitia { namespace type {
bool MessageHolder::load(const std::string & filename) {
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
    try {
        this->load_lz4(filename);
        last_load_at = pt::microsec_clock::universal_time();
        loaded = true;
    } catch(std::exception& ex) {
        LOG4CPLUS_ERROR(logger, boost::format("le chargement des données à échoué: %s") % ex.what());
        last_load = false;
    } catch(...) {
        LOG4CPLUS_ERROR(logger, "le chargement des données à échoué");
        last_load = false;
    }
    return this->last_load;
}

void MessageHolder::load_lz4(const std::string & filename) {
    std::ifstream ifs(filename.c_str(),  std::ios::in | std::ios::binary);
    boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
    in.push(LZ4Decompressor(2048*500),8192*500, 8192*500);
    in.push(ifs);
    eos::portable_iarchive ia(in);
    ia >> *this;
}

void MessageHolder::save(const std::string & filename){
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
    this->save_lz4(filename);
}

void MessageHolder::save_lz4(const std::string & filename) {
    std::ofstream ofs(filename.c_str(),std::ios::out|std::ios::binary|std::ios::trunc);
    boost::iostreams::filtering_streambuf<boost::iostreams::output> out;
    out.push(LZ4Compressor(2048*500), 1024*500, 1024*500);
    out.push(ofs);
    eos::portable_oarchive oa(out);
    oa << *this;
}

MessageHolder& MessageHolder::operator=(const navitia::type::MessageHolder&& other){
    this->messages = std::move(other.messages);
    this->generation_date = std::move(other.generation_date);
    return *this;
}

std::vector<Message> MessageHolder::find_messages(const std::string& uri, const boost::posix_time::ptime& now,
        const boost::posix_time::time_period& action_period) const{
    std::vector<Message> result;
    auto it = messages.find(uri);
    if(it != messages.end()){
        BOOST_FOREACH(auto m, it->second){
            if(m.is_valid(now, action_period)){
                result.push_back(m);
            }
        }
    }
    return result;

}

//TODO action_time devrait etre une time_duration
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

bool Message::is_applicable(const boost::posix_time::time_period& period) const{
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

bool Message::valid_hour_perturbation(const pt::time_period& period) const{
    pt::time_period daily_period(pt::ptime(period.begin().date(), application_daily_start_hour),
            pt::ptime(period.begin().date(), application_daily_end_hour));

    return period.intersects(daily_period);

}

bool Message::valid_day_of_week(const bg::date& date) const{

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
