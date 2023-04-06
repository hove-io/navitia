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

#pragma once

#include "type/type.pb.h"

#include <boost/optional.hpp>
#include <boost/utility.hpp>
#include <prometheus/exposer.h>
#include <prometheus/counter.h>
#include <prometheus/gauge.h>

#include <memory>
#include <map>

// forward declare
namespace prometheus {
class Registry;
class Counter;
class Histogram;
}  // namespace prometheus

namespace navitia {
enum class RTAction { deletion = 0, chaos, kirin };

class InFlightGuard {
    prometheus::Gauge* gauge;

public:
    explicit InFlightGuard(prometheus::Gauge* gauge);
    InFlightGuard(InFlightGuard& other) = delete;
    InFlightGuard(InFlightGuard&& other) noexcept;
    void operator=(InFlightGuard& other) = delete;
    InFlightGuard& operator=(InFlightGuard&& other) noexcept;
    ~InFlightGuard();
};

class Metrics : boost::noncopyable {
protected:
    std::unique_ptr<prometheus::Exposer> exposer;
    std::shared_ptr<prometheus::Registry> registry;
    std::map<pbnavitia::API, prometheus::Histogram*> request_histogram;
    prometheus::Gauge* in_flight;
    prometheus::Histogram* data_loading_histogram;
    prometheus::Histogram* data_cloning_histogram;
    prometheus::Histogram* handle_rt_histogram;
    prometheus::Histogram* handle_disruption_histogram;
    prometheus::Histogram* delete_disruption_histogram;
    prometheus::Histogram* retrieve_rt_message_duration_histogram;
    prometheus::Histogram* retrieved_rt_message_count_histogram;
    prometheus::Histogram* applied_rt_entity_count_histogram;
    prometheus::Histogram* rt_message_age_histogram;
    prometheus::Gauge* next_st_cache_miss;

public:
    Metrics(const boost::optional<std::string>& endpoint, const std::string& coverage);
    void observe_api(pbnavitia::API api, double duration) const;
    InFlightGuard start_in_flight() const;

    void observe_data_loading(double duration) const;
    void observe_data_cloning(double duration) const;
    void observe_handle_rt(double duration) const;
    void observe_handle_disruption(double duration) const;
    void observe_delete_disruption(double duration) const;
    void observe_retrieve_rt_message_duration(double duration) const;
    void observe_retrieved_rt_message_count(size_t count) const;
    void observe_applied_rt_entity_count(size_t count) const;
    void observe_rt_message_age(double duration) const;
    void set_raptor_cache_miss(size_t nb_cache_miss) const;
};

}  // namespace navitia
