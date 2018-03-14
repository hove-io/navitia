
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

#include "metrics.h"
#include "utils/functions.h"

#include <prometheus/exposer.h>
#include <prometheus/registry.h>
#include <prometheus/counter.h>

namespace navitia {


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


Metrics::Metrics(const boost::optional<std::string>& endpoint){
    if(endpoint == boost::none){
        return;
    }
    exposer = std::make_unique<prometheus::Exposer>(*endpoint);
    registry = std::make_shared<prometheus::Registry>();
    exposer->RegisterCollectable(registry);

    auto& histogram_family = prometheus::BuildHistogram()
                             .Name("kraken_request_duration")
                             .Help("duration of request in seconds")
                             .Register(*registry);
    auto desc = pbnavitia::API_descriptor();
    for(int i = 0; i < desc->value_count(); ++i){
        auto value = desc->value(i);
        auto& histo = histogram_family.Add({{"api", value->name()}}, create_exponential_buckets(0.001, 25, 1.5));
        this->request_histogram[static_cast<pbnavitia::API>(value->number())] = &histo;
    }
}

void Metrics::observe_api(pbnavitia::API api, float duration) const{
    if(!registry) {
        return;
    }
    this->request_histogram.at(api)->Observe(duration);
}


}//namespace navitia
