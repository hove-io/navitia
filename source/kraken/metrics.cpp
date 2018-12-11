/* Copyright © 2001-2018, Canal TP and/or its affiliates. All rights reserved.

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

#include "metrics.h"
#include "utils/functions.h"

#include <prometheus/exposer.h>
#include <prometheus/registry.h>
#include <prometheus/counter.h>
#include "utils/logger.h"

namespace navitia {

InFlightGuard::InFlightGuard(prometheus::Gauge* gauge): gauge(gauge){
    if(gauge != nullptr){
        gauge->Increment();
    }
}

InFlightGuard::~InFlightGuard() {
    if(gauge != nullptr){
        gauge->Decrement();
    }
}

InFlightGuard::InFlightGuard(InFlightGuard&& other){
    this->gauge = other.gauge;
    other.gauge = nullptr;
}

void InFlightGuard::operator=(InFlightGuard&& other){
    this->gauge = other.gauge;
    other.gauge = nullptr;
}


static prometheus::Histogram::BucketBoundaries create_exponential_buckets(double start, double factor, int count) {
    //boundaries need to be sorted!
    auto bucket_boundaries = prometheus::Histogram::BucketBoundaries{};
    double v = start;
    for (auto i = 0; i < count; i++) {
        bucket_boundaries.push_back(v);
        v = v * factor;
    }
    return bucket_boundaries;
}


Metrics::Metrics(const boost::optional<std::string>& endpoint, const std::string& coverage){
    if(endpoint == boost::none){
        return;
    }
    auto logger = log4cplus::Logger::getInstance("metrics");
    LOG4CPLUS_INFO(logger, "metrics available at http://"  << *endpoint << "/metrics");
    exposer = std::make_unique<prometheus::Exposer>(*endpoint);
    registry = std::make_shared<prometheus::Registry>();
    exposer->RegisterCollectable(registry);

    auto& histogram_family = prometheus::BuildHistogram()
                             .Name("kraken_request_duration_seconds")
                             .Help("duration of request in seconds")
                             .Labels({{"coverage", coverage}})
                             .Register(*registry);
    auto desc = pbnavitia::API_descriptor();
    for(int i = 0; i < desc->value_count(); ++i){
        auto value = desc->value(i);
        auto& histo = histogram_family.Add({{"api", value->name()}}, create_exponential_buckets(0.001, 1.5, 25));
        this->request_histogram[static_cast<pbnavitia::API>(value->number())] = &histo;
    }
    auto& in_flight_family = prometheus::BuildGauge()
                        .Name("kraken_request_in_flight")
                        .Help("Number of requests currently beeing processed")
                        .Labels({{"coverage", coverage}})
                        .Register(*registry);
    this->in_flight = &in_flight_family.Add({});

    this->data_loading_histogram = &prometheus::BuildHistogram()
                             .Name("kraken_data_loading_duration_seconds")
                             .Help("duration of loading data")
                             .Labels({{"coverage", coverage}})
                             .Register(*registry)
                             .Add({}, create_exponential_buckets(0.1, 1.5, 22));

    this->data_clonning_histogram = &prometheus::BuildHistogram()
                             .Name("kraken_data_clonning_duration_seconds")
                             .Help("duration of clonning data")
                             .Labels({{"coverage", coverage}})
                             .Register(*registry)
                             .Add({}, create_exponential_buckets(0.1, 1.5, 22));

    this->handle_rt_histogram = &prometheus::BuildHistogram()
                             .Name("kraken_handle_rt_duration_seconds")
                             .Help("duration for handling disruptions and realtime")
                             .Labels({{"coverage", coverage}})
                             .Register(*registry)
                             .Add({}, create_exponential_buckets(0.1, 1.5, 22));
}

InFlightGuard Metrics::start_in_flight() const{
    if(!registry) {
        return InFlightGuard(nullptr);
    }
    return InFlightGuard(this->in_flight);
}

void Metrics::observe_api(pbnavitia::API api, double duration) const{
    if(!registry) {
        return;
    }
    auto it = this->request_histogram.find(api);
    if(it != std::end(this->request_histogram)){
        it->second->Observe(duration);
    }else{
        auto logger = log4cplus::Logger::getInstance("metrics");
        LOG4CPLUS_WARN(logger, "api " << pbnavitia::API_Name(api) << " not found in metrics");
    }
}

void Metrics::observe_data_loading(double duration) const{
    if(!registry) {
        return;
    }
    this->data_loading_histogram->Observe(duration);
}

void Metrics::observe_data_clonning(double duration) const{
    if(!registry) {
        return;
    }
    this->data_clonning_histogram->Observe(duration);
}

void Metrics::observe_handle_rt(double duration) const{
    if(!registry) {
        return;
    }
    this->handle_rt_histogram->Observe(duration);
}


}//namespace navitia
