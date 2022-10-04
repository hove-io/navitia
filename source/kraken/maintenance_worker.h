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

#include "type/data.h"
#include "kraken/data_manager.h"
#include "kraken/configuration.h"

#include <SimpleAmqpClient/SimpleAmqpClient.h>

#include <memory>

namespace navitia {

class Metrics;

class MaintenanceWorker {
private:
    DataManager<type::Data>& data_manager;
    log4cplus::Logger logger;
    const kraken::Configuration conf;

    const Metrics& metrics;

    // nom de la queue créer pour ce worker
    std::string queue_name_task;
    std::string queue_name_rt;

    boost::posix_time::ptime next_try_realtime_loading;

    AmqpClient::Channel::ptr_t channel;

    bool channel_opened = false;

    bool task_queue_created = false;
    bool realtime_queue_created = false;

    void listen_rabbitmq();

    void handle_task_in_batch(const std::vector<AmqpClient::Envelope::ptr_t>& envelopes);
    void handle_rt_in_batch(const std::vector<AmqpClient::Envelope::ptr_t>& envelopes);

    void load_realtime();

    /*!
     * This function will consume message in batch. It calls
     * AmqpClient::Channel::BasicConsumeMessage(const std::string&, Envelope::ptr_t&, int) to try
     * to get a message within a given timeout, if BasicConsumeMessage get a message with success,
     * the message will be push back and acked. This function will loop until it get max_nb messages or
     * the queue is "empty" (the emptiness is tested by the timeout, if the network doesn't work well,
     * it'd be better to set a larger timeout).
     *
     * Since BasicConsumeMessage is non-blocking, this function is non-blocking neither.
     * */
    std::vector<AmqpClient::Envelope::ptr_t> consume_in_batch(const std::string& consume_tag,
                                                              size_t max_nb,
                                                              size_t timeout_ms,
                                                              bool no_ack);

public:
    MaintenanceWorker(DataManager<type::Data>& data_manager, const kraken::Configuration conf, const Metrics& metrics);

    // try to read data.nav.lz4
    // and reload chaos disruption
    void load_data();
    bool is_data_loaded() const;

    void open_channel_to_rabbitmq();

    void create_task_queue();

    void listen_to_task_queue_until_data_loaded();

    void create_realtime_queue();

    void run();
};

}  // namespace navitia
