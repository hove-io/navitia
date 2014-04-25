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
#include "worker.h"
#include "maintenance_worker.h"
#include "kraken/data_manager.h"
#include "utils/logger.h"
#include <zmq.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace pt = boost::posix_time;
void doWork(zmq::context_t & context, DataManager<navitia::type::Data>& data_manager) {
    auto logger = log4cplus::Logger::getInstance("worker");
    try{
        zmq::socket_t socket (context, ZMQ_REP);
        socket.connect ("inproc://workers");
        bool run = true;
        navitia::Worker w(data_manager);
        while(run) {
            zmq::message_t request;
            try{
                // Wait for next request from client
                socket.recv(&request);
            }catch(zmq::error_t){
                //on gére le cas du sighup durant un recv
                continue;
            }

            pbnavitia::Request pb_req;
            pbnavitia::Response result;
            pt::ptime start = pt::microsec_clock::local_time();
            pbnavitia::API api = pbnavitia::UNKNOWN_API;
            if(pb_req.ParseFromArray(request.data(), request.size())){
                /*auto*/ api = pb_req.requested_api();
                if(api != pbnavitia::METADATAS){
                    LOG4CPLUS_DEBUG(logger, "receive request: "
                            << pb_req.DebugString());
                }
                result = w.dispatch(pb_req);
                if(api != pbnavitia::METADATAS){
                   LOG4CPLUS_TRACE(logger, "response: " << result.DebugString());
                }
            }else{
               LOG4CPLUS_WARN(logger, "receive invalid protobuf");
               result.mutable_error()->set_id(
                       pbnavitia::Error::invalid_protobuf_request);
            }
            zmq::message_t reply(result.ByteSize());
            result.SerializeToArray(reply.data(), result.ByteSize());
            socket.send(reply);

            if(api != pbnavitia::METADATAS){
                LOG4CPLUS_DEBUG(logger, "processing time : "
                        << (pt::microsec_clock::local_time() - start).total_milliseconds());
            }
        }
    }catch(const std::exception& e){
        LOG4CPLUS_ERROR(logger, "worker die: " << e.what());
    }
}
