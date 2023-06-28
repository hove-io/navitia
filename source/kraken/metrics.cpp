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

#include "metrics.h"

#include "utils/functions.h"
#include "utils/logger.h"

#include <prometheus/counter.h>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>
#include <prometheus/histogram.h>

namespace navitia {

InFlightGuard::InFlightGuard(prometheus::Gauge* gauge) : gauge(gauge) {
    if (gauge != nullptr) {
        gauge->Increment();
    }
}

InFlightGuard::~InFlightGuard() {
    if (gauge != nullptr) {
        gauge->Decrement();
    }
}

InFlightGuard::InFlightGuard(InFlightGuard&& other) noexcept {
    this->gauge = other.gauge;
    other.gauge = nullptr;
}

InFlightGuard& InFlightGuard::operator=(InFlightGuard&& other) noexcept {
    this->gauge = other.gauge;
    other.gauge = nullptr;
    return *this;
}

static prometheus::Histogram::BucketBoundaries create_exponential_buckets(double start, double factor, int count) {
    // boundaries need to be sorted!
    auto bucket_boundaries = prometheus::Histogram::BucketBoundaries{};
    double v = start;
    for (auto i = 0; i < count; i++) {
        bucket_boundaries.push_back(v);
        v = v * factor;
    }
    return bucket_boundaries;
}

static prometheus::Histogram::BucketBoundaries create_fixed_duration_buckets() {
    // boundaries need to be sorted!
    auto bucket_boundaries = prometheus::Histogram::BucketBoundaries{0.005, 0.01, 0.02, 0.05, 0.1, 0.2, 0.3, 0.4,
                                                                     0.5,   0.7,  1,    1.5,  2,   5,   10};
    return bucket_boundaries;
}

Metrics::Metrics(const boost::optional<std::string>& endpoint, const std::string& coverage) {
    if (endpoint == boost::none) {
        return;
    }
    auto logger = log4cplus::Logger::getInstance("metrics");
    LOG4CPLUS_INFO(logger, "metrics available at http://" << *endpoint << "/metrics");
    exposer = std::make_unique<prometheus::Exposer>(*endpoint);
    registry = std::make_shared<prometheus::Registry>();
    exposer->RegisterCollectable(registry);

    auto& histogram_family = prometheus::BuildHistogram()
                                 .Name("kraken_request_duration_seconds")
                                 .Help("duration of request in seconds")
                                 .Labels({{"coverage", coverage}})
                                 .Register(*registry);
    auto desc = pbnavitia::API_descriptor();
    for (int i = 0; i < desc->value_count(); ++i) {
        auto value = desc->value(i);
        auto& histo = histogram_family.Add({{"api", value->name()}}, create_fixed_duration_buckets());
        this->request_histogram[static_cast<pbnavitia::API>(value->number())] = &histo;
    }
    auto& in_flight_family = prometheus::BuildGauge()
                                 .Name("kraken_request_in_flight")
                                 .Help("Number of requests currently being processed")
                                 .Labels({{"coverage", coverage}})
                                 .Register(*registry);
    this->in_flight = &in_flight_family.Add({});

    auto& cache_miss_family = prometheus::BuildGauge()
                                  .Name("kraken_next_stop_time_cache_miss")
                                  .Help("Number of cache miss for the next stop_time in raptor")
                                  .Labels({{"coverage", coverage}})
                                  .Register(*registry);
    next_st_cache_miss = &cache_miss_family.Add({});

    // For the followings with bucket boundaries = {0.5, 1, 2, 4, 8, 16, 32, 64, 128} in seconds
    this->data_loading_histogram = &prometheus::BuildHistogram()
                                        .Name("kraken_data_loading_duration_seconds")
                                        .Help("duration of loading data with or without realtime")
                                        .Labels({{"coverage", coverage}})
                                        .Register(*registry)
                                        .Add({}, create_exponential_buckets(0.5, 2, 10));

    this->data_cloning_histogram = &prometheus::BuildHistogram()
                                        .Name("kraken_data_cloning_duration_seconds")
                                        .Help("duration of cloning data")
                                        .Labels({{"coverage", coverage}})
                                        .Register(*registry)
                                        .Add({}, create_exponential_buckets(0.5, 2, 10));

    this->handle_rt_histogram = &prometheus::BuildHistogram()
                                     .Name("kraken_handle_rt_duration_seconds")
                                     .Help("duration for handling realtime batch (add(s)/delete(s) from chaos/kirin")
                                     .Labels({{"coverage", coverage}})
                                     .Register(*registry)
                                     .Add({}, create_exponential_buckets(0.5, 2, 10));

    this->retrieve_rt_message_duration_histogram = &prometheus::BuildHistogram()
                                                        .Name("kraken_retrieve_rt_message_duration_seconds")
                                                        .Help("duration of RT messages retrieval from RabbitMQ")
                                                        .Labels({{"coverage", coverage}})
                                                        .Register(*registry)
                                                        .Add({}, create_exponential_buckets(0.5, 2, 10));

    this->retrieved_rt_message_count_histogram = &prometheus::BuildHistogram()
                                                      .Name("kraken_retrieve_rt_message_count")
                                                      .Help("number of RT messages retrieved from RabbitMQ")
                                                      .Labels({{"coverage", coverage}})
                                                      .Register(*registry)
                                                      .Add({}, create_exponential_buckets(0.5, 2, 10));

    this->applied_rt_entity_count_histogram = &prometheus::BuildHistogram()
                                                   .Name("kraken_applied_rt_entity_count")
                                                   .Help("number of applied RT entity from a message batch")
                                                   .Labels({{"coverage", coverage}})
                                                   .Register(*registry)
                                                   .Add({}, create_exponential_buckets(0.5, 2, 10));

    this->rt_message_age_min_histogram = &prometheus::BuildHistogram()
                                              .Name("kraken_rt_message_age_min_seconds")
                                              .Help("Minimum age of RT message from a batch")
                                              .Labels({{"coverage", coverage}})
                                              .Register(*registry)
                                              .Add({}, create_exponential_buckets(0.5, 2, 10));

    this->rt_message_age_average_histogram = &prometheus::BuildHistogram()
                                                  .Name("kraken_rt_message_age_average_seconds")
                                                  .Help("Average age of RT message from a batch")
                                                  .Labels({{"coverage", coverage}})
                                                  .Register(*registry)
                                                  .Add({}, create_exponential_buckets(0.5, 2, 10));

    this->rt_message_age_max_histogram = &prometheus::BuildHistogram()
                                              .Name("kraken_rt_message_age_max_seconds")
                                              .Help("Maximum age of RT message from a batch")
                                              .Labels({{"coverage", coverage}})
                                              .Register(*registry)
                                              .Add({}, create_exponential_buckets(0.5, 2, 10));
}

InFlightGuard Metrics::start_in_flight() const {
    if (!registry) {
        return InFlightGuard(nullptr);
    }
    return InFlightGuard(this->in_flight);
}

void Metrics::observe_api(pbnavitia::API api, double duration) const {
    if (!registry) {
        return;
    }
    auto it = this->request_histogram.find(api);
    if (it != std::end(this->request_histogram)) {
        it->second->Observe(duration);
    } else {
        auto logger = log4cplus::Logger::getInstance("metrics");
        LOG4CPLUS_WARN(logger, "api " << pbnavitia::API_Name(api) << " not found in metrics");
    }
}

void Metrics::observe_data_loading(double duration) const {
    if (!registry) {
        return;
    }
    this->data_loading_histogram->Observe(duration);
}

void Metrics::observe_data_cloning(double duration) const {
    if (!registry) {
        return;
    }
    this->data_cloning_histogram->Observe(duration);
}

void Metrics::observe_handle_rt(double duration) const {
    if (!registry) {
        return;
    }
    this->handle_rt_histogram->Observe(duration);
}

void Metrics::observe_retrieve_rt_message_duration(double duration) const {
    if (!registry) {
        return;
    }
    this->retrieve_rt_message_duration_histogram->Observe(duration);
}

void Metrics::observe_retrieved_rt_message_count(size_t count) const {
    if (!registry) {
        return;
    }
    this->retrieved_rt_message_count_histogram->Observe(double(count));
}

void Metrics::observe_applied_rt_entity_count(size_t count) const {
    if (!registry) {
        return;
    }
    this->applied_rt_entity_count_histogram->Observe(double(count));
}

void Metrics::observe_rt_message_age_min(double duration) const {
    if (!registry) {
        return;
    }
    this->rt_message_age_min_histogram->Observe(duration);
}

void Metrics::observe_rt_message_age_average(double duration) const {
    if (!registry) {
        return;
    }
    this->rt_message_age_average_histogram->Observe(duration);
}

void Metrics::observe_rt_message_age_max(double duration) const {
    if (!registry) {
        return;
    }
    this->rt_message_age_max_histogram->Observe(duration);
}

void Metrics::set_raptor_cache_miss(size_t nb_cache_miss) const {
    if (!registry) {
        return;
    }
    next_st_cache_miss->Set(nb_cache_miss);
}

}  // namespace navitia
