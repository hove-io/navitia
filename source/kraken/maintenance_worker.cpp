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

#include "maintenance_worker.h"

#include "apply_disruption.h"
#include "make_disruption_from_chaos.h"
#include "metrics.h"
#include "realtime.h"
#include "type/pt_data.h"
#include "type/task.pb.h"
#include "type/kirin.pb.h"
#include "utils/get_hostname.h"

#include <SimpleAmqpClient/Envelope.h>
#include <boost/algorithm/string/join.hpp>
#include <boost/optional.hpp>
#include <boost/thread/thread.hpp>
#include <boost/range/adaptor/reversed.hpp>

#include <chrono>
#include <csignal>
#include <sys/stat.h>
#include <thread>
#include <utility>

namespace nt = navitia::type;
namespace pt = boost::posix_time;
namespace bg = boost::gregorian;

namespace navitia {

void MaintenanceWorker::run() {
    // try to load data from disk
    load_data();

    if (!is_data_loaded()) {
        // The data is not loaded, so we wait for a reload message on the task queue
        // that results in a successful data load.
        // We may be disconnected from rabbitmq during this wait,
        // and reconnection to rabbitmq is handled by this while(true) loop
        while (true) {
            try {
                open_channel_to_rabbitmq();
                create_task_queue();
                // will return when a reload message on the task queue
                // arrived and resulted in a successful data load
                listen_to_task_queue_until_data_loaded();
                // data is loaded, let's break from the while(true)  loop
                break;
            } catch (const std::runtime_error& ex) {
                LOG4CPLUS_ERROR(logger, "Connection to rabbitmq failed: " << ex.what());
                data_manager.get_data()->is_connected_to_rabbitmq = false;
                channel_opened = false;
                std::this_thread::sleep_for(std::chrono::seconds(conf.broker_reconnect_wait()));
            }
        }
    }

    // if rabbitmq was not connected, we try to connect to rabbitmq
    // When connection is successfull, we don't want to load_data(), we just listen to the queues
    while (true) {
        try {
            // will do nothing is channel is already opened
            open_channel_to_rabbitmq();
            // will do nothing if the queue already exists
            create_task_queue();

            create_realtime_queue();
            listen_rabbitmq();
        } catch (const std::runtime_error& ex) {
            LOG4CPLUS_ERROR(logger, "Connection to rabbitmq failed: " << ex.what());
            data_manager.get_data()->is_connected_to_rabbitmq = false;
            channel_opened = false;
            std::this_thread::sleep_for(std::chrono::seconds(conf.broker_reconnect_wait()));
        }
    }
}

bool MaintenanceWorker::is_data_loaded() const {
    const auto data = data_manager.get_data();
    return data->loaded;
}

void MaintenanceWorker::open_channel_to_rabbitmq() {
    if (channel_opened) {
        return;
    }
    std::string instance_name = conf.instance_name();
    // connection through URI, if URI is provided, it will be used in the first place as other other connection options
    // are neglected
    boost::optional<std::string> broker_uri = conf.broker_uri();
    // connection through classic method
    std::string exchange_name = conf.broker_exchange();
    std::string protocol = conf.broker_protocol();
    std::string host = conf.broker_host();
    int port = conf.broker_port();
    std::string username = conf.broker_username();
    std::string password = conf.broker_password();
    std::string vhost = conf.broker_vhost();
    std::string hostname = get_hostname();

    AmqpClient::Channel::OpenOpts open_opts;
    // connection through URI or classic method
    if (broker_uri) {
        open_opts = AmqpClient::Channel::OpenOpts::FromUri(*broker_uri);
    } else {
        open_opts.host = host;
        open_opts.port = port;
        open_opts.vhost = vhost;
        open_opts.auth = AmqpClient::Channel::OpenOpts::BasicAuth(username, password);
        if (protocol == "amqps") {
            open_opts.tls_params = AmqpClient::Channel::OpenOpts::TLSParams();
        }
    }
    if (open_opts.tls_params.is_initialized()) {
        (*open_opts.tls_params).verify_hostname = true;
        (*open_opts.tls_params).verify_peer = false;
    }
    LOG4CPLUS_DEBUG(logger, boost::format("trying to connect to rabbitmq: %s://%s@%s:%s/%s")
                                % (open_opts.tls_params.is_initialized() ? "amqps" : "amqp")
                                % boost::get<AmqpClient::Channel::OpenOpts::BasicAuth>(open_opts.auth).username
                                % open_opts.host % open_opts.port % open_opts.vhost);
    channel = AmqpClient::Channel::Open(open_opts);
    LOG4CPLUS_INFO(logger, "connected to rabbitmq");

    channel->DeclareExchange(exchange_name, "topic", false, true, false);

    data_manager.get_data()->is_connected_to_rabbitmq = true;
    channel_opened = true;
}

void MaintenanceWorker::load_data() {
    const std::string database = conf.databases_path();
    auto chaos_database = conf.chaos_database();
    auto chaos_batch_size = conf.chaos_batch_size();
    auto contributors = conf.rt_topics();
    LOG4CPLUS_INFO(logger, "Loading database from file: " + database);
    auto start = pt::microsec_clock::universal_time();
    bool data_loaded =
        this->data_manager.load(database, chaos_database, contributors, conf.raptor_cache_size(), chaos_batch_size);
    if (data_loaded) {
        auto data = data_manager.get_data();
        data->is_realtime_loaded = false;
        data->meta->instance_name = conf.instance_name();
    }
    auto duration = pt::microsec_clock::universal_time() - start;
    this->metrics.observe_data_loading(duration.total_seconds());
    LOG4CPLUS_INFO(logger, "Loading database duration: " << duration);
}

void MaintenanceWorker::create_task_queue() {
    if (task_queue_created) {
        return;
    }

    std::string instance_name = conf.instance_name();
    std::string hostname = get_hostname();
    std::string default_queue_name = "kraken_" + hostname + "_" + instance_name;
    std::string queue_name = conf.broker_queue(default_queue_name);
    queue_name_task = (boost::format("%s_task") % queue_name).str();

    // first we have to delete the queues, binding can change between two run, and it doesn't seem possible
    // to unbind a queue if we don't know at what topic it's subscribed
    // if the queue doesn't exist an exception is throw...
    try {
        channel->DeleteQueue(queue_name_task);
    } catch (const std::runtime_error&) {
    }
    std::string exchange_name = conf.broker_exchange();
    this->channel->DeclareExchange(exchange_name, "topic", false, true, false);

    // creation of task queue for this kraken
    bool passive = false;
    bool durable = true;
    bool exclusive = false;
    bool auto_delete_queue = conf.broker_queue_auto_delete();

    AmqpClient::Table args;
    args.insert(std::make_pair("x-expires", conf.broker_queue_expire() * 1000));

    channel->DeclareQueue(queue_name_task, passive, durable, exclusive, auto_delete_queue, args);
    LOG4CPLUS_INFO(logger, "binding queue for tasks: " << this->queue_name_task);

    // binding the queue to the exchange for all task for this instance
    channel->BindQueue(queue_name_task, exchange_name, instance_name + ".task.*");

    task_queue_created = true;
}

void MaintenanceWorker::listen_to_task_queue_until_data_loaded() {
    bool no_local = true;
    bool no_ack = false;
    bool exclusive = false;
    std::string task_tag = this->channel->BasicConsume(this->queue_name_task, "", no_local, no_ack, exclusive);
    size_t timeout_ms = conf.broker_timeout();
    while (!is_data_loaded()) {
        boost::this_thread::interruption_point();
        try {
            auto task_envelopes = consume_in_batch(task_tag, 1, timeout_ms, no_ack);
            handle_task_in_batch(task_envelopes);
        } catch (const navitia::recoverable_exception& e) {
            // on a recoverable an internal server error is returned
            LOG4CPLUS_ERROR(logger, "internal server error on rabbitmq message: " << e.what());
            LOG4CPLUS_ERROR(logger, "backtrace: " << e.backtrace());
        }

        // Since consume_in_batch is non blocking, we don't want that the worker loops for nothing, when the
        // queue is empty.
        std::this_thread::sleep_for(std::chrono::seconds(conf.broker_sleeptime()));
    }
}

void MaintenanceWorker::create_realtime_queue() {
    if (realtime_queue_created) {
        return;
    }

    std::string exchange_name = conf.broker_exchange();

    std::string instance_name = conf.instance_name();
    std::string hostname = get_hostname();
    std::string default_queue_name = "kraken_" + hostname + "_" + instance_name;
    std::string queue_name = conf.broker_queue(default_queue_name);
    queue_name_rt = (boost::format("%s_rt") % queue_name).str();

    // first we have to delete the queues, binding can change between two run, and it doesn't seem possible
    // to unbind a queue if we don't know at what topic it's subscribed
    // if the queue doesn't exist an exception is throw...
    try {
        channel->DeleteQueue(queue_name_rt);
    } catch (const std::runtime_error&) {
    }

    this->channel->DeclareExchange(exchange_name, "topic", false, true, false);

    // creation of queues for this kraken
    bool passive = false;
    bool durable = true;
    bool exclusive = false;
    bool auto_delete_queue = conf.broker_queue_auto_delete();

    AmqpClient::Table args;
    args.insert(std::make_pair("x-expires", conf.broker_queue_expire() * 1000));

    channel->DeclareQueue(this->queue_name_rt, passive, durable, exclusive, auto_delete_queue, args);
    LOG4CPLUS_INFO(logger, "queue for disruptions: " << this->queue_name_rt);
    // binding the queue to the exchange for all tasks for this instance
    LOG4CPLUS_INFO(logger, "subscribing to [" << boost::algorithm::join(conf.rt_topics(), ", ") << "]");
    for (const auto& topic : conf.rt_topics()) {
        channel->BindQueue(queue_name_rt, exchange_name, topic);
    }
    realtime_queue_created = true;
}

void MaintenanceWorker::load_realtime() {
    if (!conf.is_realtime_enabled()) {
        return;
    }
    if (!channel) {
        data_manager.get_data()->is_realtime_loaded = false;
        throw std::runtime_error("not connected to rabbitmq");
    }
    if (data_manager.get_data()->is_realtime_loaded) {
        // realtime data are already loaded, we don't have anything todo
        LOG4CPLUS_TRACE(logger, "realtime data already loaded, skipping init");
        return;
    }
    data_manager.get_data()->is_realtime_loaded = false;
    LOG4CPLUS_INFO(logger, "loading realtime data");
    //                                             name, passive, durable, exclusive, auto_delete
    std::string queue_name = channel->DeclareQueue("", false, false, true, true);
    LOG4CPLUS_DEBUG(logger, "queue used as callback for realtime data: " << queue_name);
    pbnavitia::Task task;
    task.set_action(pbnavitia::LOAD_REALTIME);
    auto* lr = task.mutable_load_realtime();
    lr->set_queue_name(queue_name);
    for (const auto& topic : conf.rt_topics()) {
        lr->add_contributors(topic);
    }
    auto data = data_manager.get_data();
    lr->set_begin_date(bg::to_iso_string(data->meta->production_date.begin()));
    lr->set_end_date(bg::to_iso_string(data->meta->production_date.end()));
    std::string stream;
    task.SerializeToString(&stream);
    auto message = AmqpClient::BasicMessage::Create(stream);
    // Add expiration timeout (in ms) to the message (TTL)
    message->Expiration(std::to_string(conf.kirin_timeout()));

    // we ask for realtime data
    channel->BasicPublish(conf.broker_exchange(), "task.load_realtime.INSTANCE", message);

    std::string consumer_tag = this->channel->BasicConsume(queue_name, "");
    AmqpClient::Envelope::ptr_t envelope{};
    // waiting for a full gtfs-rt
    if (channel->BasicConsumeMessage(consumer_tag, envelope, conf.kirin_timeout())) {
        this->handle_rt_in_batch({envelope});
        data_manager.get_data()->is_realtime_loaded = true;
    } else {
        LOG4CPLUS_WARN(logger, "no realtime data receive before timeout: going without it!");
    }
    // Finally delete the queue as we create a new one in each call to the function
    channel->DeleteQueue(queue_name);
}

void MaintenanceWorker::handle_task_in_batch(const std::vector<AmqpClient::Envelope::ptr_t>& envelopes) {
    for (auto& envelope : envelopes) {
        LOG4CPLUS_TRACE(logger, "Task received");
        pbnavitia::Task task;
        bool result = task.ParseFromString(envelope->Message()->Body());
        if (!result) {
            LOG4CPLUS_WARN(logger, "Protobuf is not valid!");
            return;
        }
        switch (task.action()) {
            case pbnavitia::RELOAD:
                this->load_data();
                this->load_realtime();
                // For now, we have only one type of task: reload_kraken. We don't want that this command
                // is executed several times in stride for nothing.
                return;
            default:
                LOG4CPLUS_TRACE(logger, "Task ignored");
        }
    }
}

static bool autocomplete_rebuilding_needed(const transit_realtime::FeedEntity& entity) {
    return ((entity.trip_update().GetExtension(kirin::effect)
             == transit_realtime::Alert_Effect::Alert_Effect_ADDITIONAL_SERVICE)
            || (entity.trip_update().GetExtension(kirin::effect) == transit_realtime::Alert_Effect::Alert_Effect_DETOUR)
            || (entity.trip_update().GetExtension(kirin::effect)
                == transit_realtime::Alert_Effect::Alert_Effect_MODIFIED_SERVICE));
}

void MaintenanceWorker::handle_rt_in_batch(const std::vector<AmqpClient::Envelope::ptr_t>& envelopes) {
    boost::shared_ptr<nt::Data> data{};
    pt::ptime begin = pt::microsec_clock::universal_time();
    bool autocomplete_rebuilding_activated = false;
    auto rt_action = RTAction::chaos;

    std::unordered_set<std::string> applied_visited_id;
    for (auto& envelope : boost::adaptors::reverse(envelopes)) {
        const auto routing_key = envelope->RoutingKey();
        LOG4CPLUS_DEBUG(logger, "realtime info received from " << routing_key);
        assert(envelope);
        transit_realtime::FeedMessage feed_message;
        if (!feed_message.ParseFromString(envelope->Message()->Body())) {
            LOG4CPLUS_WARN(logger, "protobuf not valid!");
            return;
        }
        LOG4CPLUS_TRACE(logger, "received entity: " << feed_message.DebugString());
        for (const auto& entity : feed_message.entity()) {
            auto res = applied_visited_id.insert(entity.id());
            if (!res.second) {
                continue;
            }
            if (!data) {
                pt::ptime copy_begin = pt::microsec_clock::universal_time();
                data = data_manager.get_data_clone();
                auto duration = pt::microsec_clock::universal_time() - copy_begin;
                this->metrics.observe_data_cloning(duration.total_seconds());
                LOG4CPLUS_INFO(logger, "data copied (cloned) in " << duration);
            }
            if (entity.is_deleted()) {
                LOG4CPLUS_DEBUG(logger, "deletion of disruption " << entity.id());
                rt_action = RTAction::deletion;
                delete_disruption(entity.id(), *data->pt_data, *data->meta);
            } else if (entity.HasExtension(chaos::disruption)) {
                LOG4CPLUS_DEBUG(logger, "add/update of disruption " << entity.id());
                rt_action = RTAction::chaos;
                make_and_apply_disruption(entity.GetExtension(chaos::disruption), *data->pt_data, *data->meta);
            } else if (entity.has_trip_update()) {
                LOG4CPLUS_DEBUG(logger, "RT trip update" << entity.id());
                rt_action = RTAction::kirin;
                handle_realtime(entity.id(), navitia::from_posix_timestamp(feed_message.header().timestamp()),
                                entity.trip_update(), *data, conf.is_realtime_add_enabled(),
                                conf.is_realtime_add_trip_enabled());
                autocomplete_rebuilding_activated = autocomplete_rebuilding_needed(entity);
            } else {
                LOG4CPLUS_WARN(logger, "unsupported gtfs rt feed");
            }
        }
    }
    if (data) {
        LOG4CPLUS_INFO(logger, "rebuilding relations");
        data->build_relations();
        if (autocomplete_rebuilding_activated) {
            LOG4CPLUS_INFO(logger, "rebuilding autocomplete");
            data->build_autocomplete_partial();
        }
        LOG4CPLUS_INFO(logger, "cleaning weak impacts");
        data->pt_data->clean_weak_impacts();
        LOG4CPLUS_INFO(logger, "rebuilding data raptor");
        data->build_raptor(conf.raptor_cache_size());
        data->build_proximity_list();
        data->warmup(*data_manager.get_data());
        data->set_last_rt_data_loaded(pt::microsec_clock::universal_time());
        data_manager.set_data(std::move(data));

        // Feed metrics
        auto duration = pt::microsec_clock::universal_time() - begin;
        if (rt_action == RTAction::deletion) {
            this->metrics.observe_delete_disruption(duration.total_milliseconds() / 1000.0);
            LOG4CPLUS_INFO(logger, "Data updated after deleting disruption, "
                                       << envelopes.size() << " disruption(s) applied in " << duration);
        } else if (rt_action == RTAction::chaos) {
            this->metrics.observe_handle_disruption(duration.total_milliseconds() / 1000.0);
            LOG4CPLUS_INFO(logger, "Data updated with disruptions from chaos, "
                                       << envelopes.size() << " disruption(s) applied in " << duration);
        } else if (rt_action == RTAction::kirin) {
            this->metrics.observe_handle_rt(duration.total_milliseconds() / 1000.0);
            LOG4CPLUS_INFO(logger, "Data updated with realtime from kirin, "
                                       << envelopes.size() << " disruption(s) applied in " << duration);
        }
    } else if (!envelopes.empty()) {
        // we didn't had to update Data because there is no change but we want to track that realtime data
        // is being processed as it should because "nothing has changed" isn't the same thing
        // than "I don't known what's happening"
        auto current_data = data_manager.get_data();
        current_data->set_last_rt_data_loaded(pt::microsec_clock::universal_time());
    }
}

std::vector<AmqpClient::Envelope::ptr_t> MaintenanceWorker::consume_in_batch(const std::string& consume_tag,
                                                                             size_t max_nb,
                                                                             size_t timeout_ms,
                                                                             bool no_ack) {
    assert(consume_tag != "");
    assert(max_nb);

    std::vector<AmqpClient::Envelope::ptr_t> envelopes;
    envelopes.reserve(max_nb);
    size_t consumed_nb = 0;
    auto begin = pt::microsec_clock::universal_time();

    auto retrieving_timeout = conf.retrieving_timeout();
    pt::time_duration duration = pt::milliseconds{0};
    while (consumed_nb < max_nb
           && (pt::microsec_clock::universal_time() - begin).total_milliseconds() < retrieving_timeout) {
        AmqpClient::Envelope::ptr_t envelope{};

        /* !
         * The emptiness is tested thanks to the timeout. We consider that the queue is empty when
         * BasicConsumeMessage() timeout.
         * */
        bool queue_is_empty = !channel->BasicConsumeMessage(consume_tag, envelope, timeout_ms);
        if (queue_is_empty) {
            break;
        }

        if (envelope) {
            envelopes.push_back(envelope);
            if (!no_ack) {
                channel->BasicAck(envelope);
            }
            ++consumed_nb;
        }
    }
    return envelopes;
}

void MaintenanceWorker::listen_rabbitmq() {
    if (!channel) {
        throw std::runtime_error("not connected to rabbitmq");
    }
    bool no_local = true;
    bool no_ack = false;
    bool exclusive = false;
    std::string task_tag = this->channel->BasicConsume(this->queue_name_task, "", no_local, no_ack, exclusive);
    std::string rt_tag = this->channel->BasicConsume(this->queue_name_rt, "", no_local, no_ack, exclusive);

    LOG4CPLUS_INFO(logger, "start event loop");

    while (true) {
        boost::this_thread::interruption_point();
        auto now = pt::microsec_clock::universal_time();
        // We don't want to try to load realtime data every second
        if (now > this->next_try_realtime_loading) {
            this->next_try_realtime_loading = now + pt::milliseconds(conf.kirin_retry_timeout());
            this->load_realtime();
        }
        size_t timeout_ms = conf.broker_timeout();

        // Arbitrary Number: we suppose that disruptions can be handled very quickly so that,
        // in theory, we can handle a batch of 5000 disruptions in one time very quickly too.
        size_t max_batch_nb = conf.broker_max_batch_nb();

        try {
            auto rt_envelopes = consume_in_batch(rt_tag, max_batch_nb, timeout_ms, no_ack);
            handle_rt_in_batch(rt_envelopes);

            auto task_envelopes = consume_in_batch(task_tag, 1, timeout_ms, no_ack);
            handle_task_in_batch(task_envelopes);
        } catch (const navitia::recoverable_exception& e) {
            // on a recoverable an internal server error is returned
            LOG4CPLUS_ERROR(logger, "internal server error on rabbitmq message: " << e.what());
            LOG4CPLUS_ERROR(logger, "backtrace: " << e.backtrace());
        }

        // Since consume_in_batch is non blocking, we don't want that the worker loops for nothing, when the
        // queue is empty.
        std::this_thread::sleep_for(std::chrono::seconds(conf.broker_sleeptime()));
    }
}

MaintenanceWorker::MaintenanceWorker(DataManager<type::Data>& data_manager,
                                     kraken::Configuration conf,
                                     const Metrics& metrics)
    : data_manager(data_manager),
      logger(log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("background"))),
      conf(std::move(conf)),
      metrics(metrics),
      next_try_realtime_loading(pt::microsec_clock::universal_time()) {}

}  // namespace navitia
