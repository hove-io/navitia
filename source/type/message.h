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

#pragma once

#include <boost/date_time/posix_time/posix_time.hpp>


#include <boost/serialization/serialization.hpp>
#include <boost/date_time/gregorian/greg_serialize.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>
#include <boost/serialization/bitset.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>

#include <atomic>
#include <map>
#include <vector>
#include <string>
#include "utils/serialization_unique_ptr.h"
#include "utils/serialization_unique_ptr_container.h"

#include "type/type.h"

namespace navitia { namespace type {

namespace new_disruption {

enum class Effect {
  NO_SERVICE,
  REDUCED_SERVICE,
  SIGNIFICANT_DELAYS,
  DETOUR,
  ADDITIONAL_SERVICE,
  MODIFIED_SERVICE,
  OTHER_EFFECT,
  UNKNOWN_EFFECT,
  STOP_MOVED
};


struct Cause {
    std::string uri;
    std::string wording;
    boost::posix_time::ptime created_at;
    boost::posix_time::ptime updated_at;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar & uri & wording & created_at & updated_at;
    }
};

struct Severity {
    std::string uri;
    std::string wording;
    boost::posix_time::ptime created_at;
    boost::posix_time::ptime updated_at;
    std::string color;

    int priority;

    Effect effect;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar & uri & wording & created_at & updated_at & color & priority & effect;
    }
};

struct PtObject {
    Type_e object_type;
    std::string object_uri;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar & object_type & object_uri;
    }
};

struct Disruption;

struct Message {
    std::string text;

    boost::posix_time::ptime created_at;
    boost::posix_time::ptime updated_at;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar & text & created_at & updated_at;
    }
};

struct Impact {
    std::string uri;
    boost::posix_time::ptime created_at;
    boost::posix_time::ptime updated_at;

    // the application period define when the impact happen
    std::vector<boost::posix_time::time_period> application_periods;

    std::shared_ptr<Severity> severity;

    std::vector<PtObject> informed_entities;

    std::vector<Message> messages;

    //link to the parent disruption
    //Note: it is a raw pointer because an Impact is owned by it's disruption
    //(even if the impact is stored as a share_ptr in the disruption to allow for weak_ptr towards it)
    Disruption* disruption;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar & uri & created_at & updated_at & application_periods & severity & informed_entities & messages & disruption;
    }

    bool is_valid(const boost::posix_time::ptime& current_time, const boost::posix_time::time_period& action_period) const;
};

struct Tag {
    std::string uri;
    std::string name;
    boost::posix_time::ptime created_at;
    boost::posix_time::ptime updated_at;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar & uri & name & created_at & updated_at;
    }
};

struct Disruption {
    std::string uri;

    // it's the title of the disruption as shown in the backoffice
    std::string reference;

    // the publication period specify when an information can be displayed to
    // the customer, if a request is made before or after this period the
    // disruption must not be shown
    boost::posix_time::time_period publication_period {
        boost::posix_time::not_a_date_time, boost::posix_time::seconds(1)
    };//no default constructor for time_period, we must provide a value

    boost::posix_time::ptime created_at;
    boost::posix_time::ptime updated_at;

    std::shared_ptr<Cause> cause;

    //impacts are shared_ptr because there are weak_ptr pointing to them in the impacted objects
    std::vector<std::shared_ptr<Impact>> impacts;

    // the place where the disruption happen, the impacts can be in anothers places
    std::vector<PtObject> localization;

    //additional informations on the disruption
    std::vector<std::shared_ptr<Tag>> tags;

    std::string note;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar & uri & reference & publication_period
           & created_at & updated_at & cause & impacts & localization & tags & note;
    }
};

struct DisruptionHolder { //=> to be renamed as Disruptions
    std::vector<std::unique_ptr<Disruption>> disruptions;

    // causes, severities and tags are a pool (weak_ptr because the owner ship
    // is in the linked disruption or impact)
    std::map<std::string, std::weak_ptr<Cause>> causes; //to be wrapped
    std::map<std::string, std::weak_ptr<Severity>> severities; //to be wrapped too
    std::map<std::string, std::weak_ptr<Tag>> tags; //to be wrapped too

    template<class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar & disruptions & causes & severities & tags;
    }
};
}


/**
 * Old AT perturbation
 *
 * only used for adapated computation.
 *
 * will be soon (hope so) completly removed by Disruption
 */
struct AtPerturbation{
    std::string uri;

    Type_e object_type;
    std::string object_uri;

    boost::posix_time::time_period application_period;

    boost::posix_time::time_duration application_daily_start_hour;
    boost::posix_time::time_duration application_daily_end_hour;

    std::bitset<8> active_days;

    AtPerturbation(): object_type(Type_e::ValidityPattern),
        application_period(boost::posix_time::not_a_date_time, boost::posix_time::seconds(0)){}

    bool valid_day_of_week(const boost::gregorian::date& date) const;

    bool valid_hour_perturbation(const boost::posix_time::time_period& period) const;

    bool is_applicable(const boost::posix_time::time_period& time) const;

    bool operator<(const AtPerturbation& other) const {
        return (this->uri < other.uri);
    }
};


}}//namespace
