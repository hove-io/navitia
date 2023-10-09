# Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.hove.com).
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
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from __future__ import absolute_import, print_function, unicode_literals, division
import logging
import flask
import os
import timeit
import functools
from typing import Text, Callable
from contextlib import contextmanager
from jormungandr import app
from functools import wraps

try:
    from newrelic import agent
except ImportError:
    logger = logging.getLogger(__name__)
    logger.warning('New Relic is not available')
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


def background_task(name, group):  # type: (Text, Text) -> Callable
    """
    Create a newrelic background task if we aren't already in a Transaction
    This is usefull for function that might be run in a greenlet to not loose instrumentation
    """

    def wrap(func):
        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            if agent and agent.current_transaction() is None:
                with agent.BackgroundTask(agent.application(), name=name, group=group):
                    return func(*args, **kwargs)
            else:
                return func(*args, **kwargs)

        return wrapper

    return wrap


def get_common_event_params(service_name, call_name, status="ok"):
    return {
        "service": service_name,
        "call": call_name,
        "status": status,
        "az": app.config.get("DEPLOYMENT_AZ", "unknown"),
    }


def distributedEvent(call_name, group_name):
    """
    Custom event that we publish to New Relic for distributed scenario
    """

    def wrap(func):
        @functools.wraps(func)
        def wrapper(obj, service, *args, **kwargs):
            event_params = get_common_event_params(type(service).__name__, call_name)
            event_params.update({"group": group_name})

            start_time = timeit.default_timer()
            result = None
            try:
                result = func(obj, *args, **kwargs)
            except Exception as e:
                event_params["status"] = "failed"
                event_params.update({"exception": e})
                raise

            duration = timeit.default_timer() - start_time
            event_params.update({"duration": duration})

            # Send the custom event to newrelic !
            record_custom_event("distributed", event_params)

            return result

        return wrapper

    return wrap


@contextmanager
def record_streetnetwork_call(call_name, connector_name, mode, coverage_name):
    """
    Custom event that we publish to New Relic for external streetnetwork call such as:
    Asgard, Kraken, Geovelo, Here, etc
    """
    newrelic_service_name = "streetnetwork_call"
    event_params = get_common_event_params(newrelic_service_name, call_name)
    event_params.update({"connector": connector_name, "mode": mode, "coverage": coverage_name})

    start_time = timeit.default_timer()
    try:
        yield
    except Exception as e:
        event_params["status"] = "failed"
        event_params.update({"exception": e})
        raise

    duration = timeit.default_timer() - start_time
    event_params.update({"duration": duration})

    # Send the custom event to newrelic !
    record_custom_event(newrelic_service_name, event_params)


def statManagerEvent(call_name, group_name):
    """
    Custom event that we publish to New Relic for stat_manager
    """

    def wrap(func):
        @functools.wraps(func)
        def wrapper(obj, service, *args, **kwargs):
            event_params = get_common_event_params(type(service).__name__, call_name)
            event_params.update({"group": group_name})

            start_time = timeit.default_timer()
            try:
                return func(obj, *args, **kwargs)
            except Exception as e:
                event_params["status"] = "failed"
                event_params.update({"reason": str(e)})
                raise
            finally:
                duration = timeit.default_timer() - start_time
                event_params.update({"duration": duration})

                # Send the custom event to newrelic !
                record_custom_event("stat_manager", event_params)

        return wrapper

    return wrap


class TransientSocketEvent:
    """
    Custom event that we publish to New Relic for transient_socket
    """

    def __init__(self, call_name, group_name, enable=False):
        self.call_name = call_name
        self.group_name = group_name
        self.enable = enable

    def __call__(self, func):
        @wraps(func)
        def wrapper(obj, service, *args, **kwargs):
            if not self.enable:
                return func(obj, *args, **kwargs)

            event_params = get_common_event_params(type(service).__name__, self.call_name)
            event_params.update({"group": self.group_name})

            start_time = timeit.default_timer()
            try:
                return func(obj, *args, **kwargs)
            except Exception as e:
                event_params["status"] = "failed"
                event_params.update({"reason": str(e)})
                raise
            finally:
                duration = timeit.default_timer() - start_time
                event_params.update({"duration": duration})

                # Send the custom event to newrelic !
                record_custom_event("transient_socket", event_params)

        return wrapper
