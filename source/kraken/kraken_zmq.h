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
#include "worker.h"
#include "maintenance_worker.h"
#include "kraken/data_manager.h"
#include "utils/logger.h"
#include "utils/zmq.h"
#include "kraken/configuration.h"
#include "type/meta_data.h"
#include "metrics.h"
#include "utils/deadline.h"
#include "type/datetime.h"

#include <log4cplus/ndc.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/optional/optional_io.hpp>

static void respond(zmq::socket_t& socket,
                    const std::vector<std::string>& client_id,
                    const pbnavitia::Response& response) {
    zmq::message_t reply(response.ByteSizeLong());
    try {
        response.SerializeToArray(reply.data(), response.ByteSizeLong());
    } catch (const google::protobuf::FatalException& e) {
        auto logger = log4cplus::Logger::getInstance("worker");
        LOG4CPLUS_ERROR(logger, "failure during serialization: " << e.what());
        pbnavitia::Response error_response;
        error_response.mutable_error()->set_id(pbnavitia::Error::internal_error);
        error_response.mutable_error()->set_message(e.what());
        reply.rebuild(error_response.ByteSizeLong());
        error_response.SerializeToArray(reply.data(), error_response.ByteSizeLong());
    }
    for (const auto& idx : client_id) {
        z_send(socket, idx, zmq::send_flags::sndmore);
    }
    socket.send(reply, zmq::send_flags::none);
}

static pbnavitia::Response create_error_response(std::string error_message, pbnavitia::Error_error_id error_id) {
    pbnavitia::Response response;
    auto* error = response.mutable_error();
    error->set_id(error_id);
    error->set_message(error_message);
    return response;
}

namespace pt = boost::posix_time;
inline void doWork(zmq::context_t& context,
                   DataManager<navitia::type::Data>& data_manager,
                   navitia::kraken::Configuration conf,
                   const navitia::Metrics& metrics,
                   const std::string& hostname,
                   int worker_id) {
    auto logger = log4cplus::Logger::getInstance("worker");

    zmq::socket_t socket(context, ZMQ_REQ);
    socket.connect("inproc://workers");
    bool run = true;
    auto enable_deadline = conf.enable_request_deadline();
    // Here we create the worker
    navitia::Worker w(conf);
    z_send(socket, "READY");
    auto slow_request_duration = pt::milliseconds(conf.slow_request_duration());

    std::vector<std::string> frames{};

    while (run) {
        size_t more = 0;
        size_t more_size = sizeof(more);
        frames.clear();
        try {
            do {
                std::string frame = z_recv(socket);
                frames.push_back(frame);

                // Are there more frames coming?
                socket.getsockopt(ZMQ_RCVMORE, &more, &more_size);
            } while (more);
        } catch (const zmq::error_t&) {
            // on gére le cas du sighup durant un recv
            LOG4CPLUS_WARN(logger, "Zmq error occured while receiving. I'll ignore this message.");
            pbnavitia::Response response =
                create_error_response("zmq error occured while receiving", pbnavitia::Error::internal_error);
            respond(socket, {}, response);
            continue;
        }

        // we should obtain at least 3 frames
        if (frames.size() < 3) {
            LOG4CPLUS_WARN(logger, "Received a zmq message with less than 3 frames. I'll ignore it.");
            pbnavitia::Response response = create_error_response("Received a zmq message with less than 3 frames.",
                                                                 pbnavitia::Error::internal_error);
            respond(socket, {}, response);
            continue;
        }

        // the penultimate frame should be empty
        if (frames[frames.size() - 2] != "") {
            LOG4CPLUS_WARN(logger, "Received a zmq message with a non empty penultimate frame. I'll ignore it.");
            pbnavitia::Response response = create_error_response(
                "Received a zmq message with a non empty penultimate frame.", pbnavitia::Error::internal_error);
            respond(socket, {}, response);
            continue;
        }

        // the payload is the last frame
        // we pop it from the "frames" vector
        // the remaining elements in "frames"
        // is :
        //  - one or more frame with the client_id
        //  - then an empty frame
        std::string payload = frames.back();
        frames.pop_back();

        navitia::InFlightGuard in_flight_guard(metrics.start_in_flight());
        pbnavitia::Request pb_req;
        pt::ptime start = pt::microsec_clock::universal_time();
        pbnavitia::API api = pbnavitia::UNKNOWN_API;
        if (!pb_req.ParseFromArray(payload.data(), payload.size())) {
            LOG4CPLUS_WARN(logger, "receive invalid protobuf");
            pbnavitia::Response response =
                create_error_response("Receive invalid protobuf.", pbnavitia::Error::invalid_protobuf_request);
            respond(socket, frames, response);
            continue;
        }

        api = pb_req.requested_api();
        const std::string& request_id = pb_req.request_id();
        log4cplus::NDCContextCreator ndc(request_id);

        LOG4CPLUS_INFO(logger, "receive request : " << pbnavitia::API_Name(api));
        if (api != pbnavitia::METADATAS) {
            LOG4CPLUS_DEBUG(logger, "receive request: " << pb_req.DebugString());
        }

        auto deadline = navitia::Deadline();
        if (enable_deadline && pb_req.has_deadline()) {
            try {
                deadline.set(boost::posix_time::from_iso_string(pb_req.deadline()));
            } catch (const std::exception& e) {
                LOG4CPLUS_WARN(logger, "impossible to parse deadline " << pb_req.deadline() << " : " << e.what());
            }
        }

        LOG4CPLUS_DEBUG(logger, "deadline set to " << deadline.get());
        const auto data = data_manager.get_data();
        try {
            w.dispatch(pb_req, *data, deadline);
            if (api != pbnavitia::METADATAS) {
                LOG4CPLUS_TRACE(logger, "response: " << w.pb_creator.get_response().DebugString());
            }
        } catch (const navitia::DeadlineExpired& e) {
            LOG4CPLUS_ERROR(logger, "deadline expired, aborting request: " << e.what());
            w.pb_creator.fill_pb_error(pbnavitia::Error::deadline_expired, e.what());
            // we still respond so this thread become availlable again
        } catch (const navitia::recoverable_exception& e) {
            // on a recoverable an internal server error is returned
            LOG4CPLUS_ERROR(logger, "internal server error: " << e.what());
            LOG4CPLUS_ERROR(logger, "on query: " << pb_req.DebugString());
            LOG4CPLUS_ERROR(logger, "backtrace: " << e.backtrace());
            w.pb_creator.fill_pb_error(pbnavitia::Error::internal_error, e.what());
        }
        if (!data->loaded) {
            w.pb_creator.set_publication_date(boost::gregorian::not_a_date_time);
        } else {
            w.pb_creator.set_publication_date(data->meta->publication_date);
        }
        respond(socket, frames, w.pb_creator.get_response());
        auto end = pt::microsec_clock::universal_time();
        auto duration = end - start;
        metrics.observe_api(api, duration.total_milliseconds() / 1000.0);
        auto cache_miss = w.get_raptor_next_st_cache_miss();
        if (cache_miss) {
            metrics.set_raptor_cache_miss(*cache_miss);
        }
        if (duration >= slow_request_duration) {
            LOG4CPLUS_WARN(logger, "slow request! duration: " << duration.total_milliseconds()
                                                              << "ms request: " << pb_req.DebugString());
        } else if (api != pbnavitia::METADATAS) {
            LOG4CPLUS_DEBUG(logger, "processing time : " << duration.total_milliseconds());
        }

        auto start_timestamp = (start - navitia::posix_epoch).total_milliseconds();
        auto end_timestamp = (end - navitia::posix_epoch).total_milliseconds();
        LOG4CPLUS_INFO(logger, "Api : " << pbnavitia::API_Name(api) << ", hostname: " << hostname
                                        << ", worker : " << worker_id << ", request : " << request_id
                                        << ", start : " << start_timestamp << ", end : " << end_timestamp);
    }
}
