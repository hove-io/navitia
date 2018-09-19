# Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Canal TP (www.canaltp.fr).
# Help us simplify mobility and open public transport:
#     a non ending quest to the responsive locomotion way of traveling!
#
# LICENCE: This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#
# Stay tuned using
# twitter @navitia
# IRC #navitia on freenode
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from __future__ import absolute_import, print_function, unicode_literals, division
import logging
import flask
import os
import timeit

try:
    from newrelic import agent
except ImportError:
    logger = logging.getLogger(__name__)
    logger.exception('failure while importing newrelic')
    agent = None

def init(config):
    if not agent or not config:
        return
    if os.path.exists(config):
        agent.initialize(config)
    else:
        logging.getLogger(__name__).error('%s doesn\'t exist, newrelic disabled', config)


def record_exception():
    """
    record the exception currently handled to newrelic
    """
    if agent:
        agent.record_exception()  # will record the exception currently handled


def record_custom_parameter(name, value):
    """
    add a custom parameter to the current request
    """
    if agent:
        agent.add_custom_parameter(name, value)


def record_custom_event(event_type, params):
    """
    record an event
    Event doesn't share anything with request so we track the request id
    """
    if agent:
        try:
            if not params:
                params = {}
            params['navitia_request_id'] = flask.request.id
        except RuntimeError:
            pass  # we are outside of a flask context :(
        try:
            agent.record_custom_event(event_type, params, agent.application())
        except:
            logger = logging.getLogger(__name__)
            logger.exception('failure while reporting to newrelic')


def ignore():
    """
    the transaction won't be sent to newrelic
    """
    if agent:
        try:
            agent.suppress_transaction_trace()
        except:
            logger = logging.getLogger(__name__)
            logger.exception('failure while ignoring transaction')


class DistributedEvent:
    """
    Custom event that we publish to New Relic for distributed scenario
    """
    def __init__(self, service, call_name, group_name):
        self.event_params = {
            "service" : type(service).__name__,
            "call" : call_name,
            "group" : group_name,
            "status" : "ok",
        }


    def time_function(self, function, *args, **kwargs):
        start_time = timeit.default_timer()
        result = None
        try:
            result = function(*args, **kwargs)
        except Exception as e:
            self.event_params["status"] = "failed"
            self.event_params.update({"exception" : e})

        duration = timeit.default_timer() - start_time
        self.event_params.update({"duration" : duration})

        # Send the custom event to newrelic !
        record_custom_event("distributed", self.event_params)
        return result

