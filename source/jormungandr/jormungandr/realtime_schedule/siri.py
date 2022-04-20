# encoding: utf-8

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
import pybreaker
import requests as requests
from jormungandr import cache, app
from jormungandr.realtime_schedule.realtime_proxy import RealtimeProxy, RealtimeProxyError, floor_datetime
from jormungandr.schedule import RealTimePassage
import xml.etree.ElementTree as et
import aniso8601
from datetime import datetime
from flask_restful.inputs import boolean
import six


def to_bool(b):
    """
    encapsulate flask_restful.inputs.boolean to prevent exception if format isn't valid

    >>> to_bool('true')
    True
    >>> to_bool('false')
    False
    >>> to_bool('f')
    False
    >>> to_bool('t')
    False
    >>> to_bool('bob')
    False
    """
    try:
        return boolean(b)
    except ValueError:
        return False


class Siri(RealtimeProxy):
    """
    Class managing calls to siri external service providing real-time next passages


    curl example to check/test that external service is working:
    curl -X POST '{server}' -d '<x:Envelope
    xmlns:x="http://schemas.xmlsoap.org/soap/envelope/" xmlns:wsd="http://wsdl.siri.org.uk" xmlns:siri="http://www.siri.org.uk/siri">
        <x:Header/>
        <x:Body>
            <GetStopMonitoring xmlns="http://wsdl.siri.org.uk" xmlns:siri="http://www.siri.org.uk/siri">
                <ServiceRequestInfo xmlns="">
                    <siri:RequestTimestamp>{datetime}</siri:RequestTimestamp>
                    <siri:RequestorRef>{requestor_ref}</siri:RequestorRef>
                    <siri:MessageIdentifier>IDontCare</siri:MessageIdentifier>
                </ServiceRequestInfo>
                <Request version="1.3" xmlns="">
                    <siri:RequestTimestamp>{datetime}</siri:RequestTimestamp>
                    <siri:MessageIdentifier>IDontCare</siri:MessageIdentifier>
                    <siri:MonitoringRef>{stop_code}</siri:MonitoringRef>
                    <siri:MinimumStopVisitsPerLine>{nb_desired}</siri:MinimumStopVisitsPerLine>
                </Request>
                <RequestExtension xmlns=""/>
            </GetStopMonitoring>
        </x:Body>
    </x:Envelope>'

    {datetime} is iso-formated: YYYY-mm-ddTHH:MM:ss.sss+HH:MM
    {requestor_ref} is a configuration parameter
    {stop_code} is the stop_point code value, which type is the 'id' of the connector (or 'destination_id_tag' if provided in conf)
    ex: for a connector "Siri_BOB", on stop_point_BOB, you should find in the Navitia stop_point response:
        "codes": [
           {
                "type": "Siri_BOB",
                "value": "Bobito:StopPoint:BOB:00021201:ITO"
            }, ...
    {nb_desired} is the requested number of next passages

    In practice it will look like:
    curl -X POST 'http://bobito.fr:8080/ProfilSiriKidfProducer-Bobito/SiriServices' -d '<x:Envelope
    xmlns:x="http://schemas.xmlsoap.org/soap/envelope/" xmlns:wsd="http://wsdl.siri.org.uk" xmlns:siri="http://www.siri.org.uk/siri">
        <x:Header/>
        <x:Body>
            <GetStopMonitoring xmlns="http://wsdl.siri.org.uk" xmlns:siri="http://www.siri.org.uk/siri">
                <ServiceRequestInfo xmlns="">
                    <siri:RequestTimestamp>2018-06-11T17:21:49.703+02:00</siri:RequestTimestamp>
                    <siri:RequestorRef>BobitoJVM</siri:RequestorRef>
                    <siri:MessageIdentifier>IDontCare</siri:MessageIdentifier>
                </ServiceRequestInfo>
                <Request version="1.3" xmlns="">
                    <siri:RequestTimestamp>2018-06-11T17:21:49.703+02:00</siri:RequestTimestamp>
                    <siri:MessageIdentifier>IDontCare</siri:MessageIdentifier>
                    <siri:MonitoringRef>Bobito:StopPoint:BOB:00021201:ITO</siri:MonitoringRef>
                    <siri:MaximumStopVisits>5</siri:MaximumStopVisits>
                </Request>
                <RequestExtension xmlns=""/>
            </GetStopMonitoring>
        </x:Body>
    </x:Envelope>'

    Then Navitia matches route-points in the response using {stop_code}, {route_code} and {line_code}.
    {stop_code}, {route_code} and {line_code} are provided using the same code key, named after
    the 'destination_id_tag' if provided on connector's init, or the 'id' otherwise.
    """

    def __init__(
        self,
        id,
        service_url,
        requestor_ref,
        object_id_tag=None,
        destination_id_tag=None,
        instance=None,
        timeout=10,
        **kwargs
    ):
        self.service_url = service_url
        self.requestor_ref = requestor_ref  # login for siri
        self.timeout = timeout  # timeout in seconds
        self.rt_system_id = id
        self.object_id_tag = object_id_tag if object_id_tag else id
        self.destination_id_tag = destination_id_tag
        self.instance = instance
        fail_max = kwargs.get(
            'circuit_breaker_max_fail', app.config.get(str('CIRCUIT_BREAKER_MAX_SIRI_FAIL'), 5)
        )
        reset_timeout = kwargs.get(
            'circuit_breaker_reset_timeout', app.config.get(str('CIRCUIT_BREAKER_SIRI_TIMEOUT_S'), 60)
        )
        self.breaker = pybreaker.CircuitBreaker(fail_max=fail_max, reset_timeout=reset_timeout)
        # A step is applied on from_datetime to discretize calls and allow caching them
        self.from_datetime_step = kwargs.get(
            'from_datetime_step', app.config['CACHE_CONFIGURATION'].get('TIMEOUT_SIRI', 60)
        )

    def __repr__(self):
        """
        used as the cache key. we use the rt_system_id to share the cache between servers in production
        """
        try:
            return self.rt_system_id.encode('utf-8', 'backslashreplace')
        except:
            return self.rt_system_id

    def _get_next_passage_for_route_point(self, route_point, count, from_dt, current_dt, duration=None):
        stop = route_point.fetch_stop_id(self.object_id_tag)
        request = self._make_request(monitoring_ref=stop, dt=from_dt, count=count)
        if not request:
            return None
        siri_response = self._call_siri(request)
        if not siri_response or siri_response.status_code != 200:
            raise RealtimeProxyError('invalid response')
        logging.getLogger(__name__).debug('siri for {}: {}'.format(stop, siri_response.text))

        ns = {'siri': 'http://www.siri.org.uk/siri'}
        tree = None
        try:
            tree = et.fromstring(siri_response.content)
        except et.ParseError:
            logging.getLogger(__name__).exception("invalid xml")
            raise RealtimeProxyError('invalid xml')

        self._validate_response_or_raise(tree, ns)

        return self._get_passages(tree, ns, route_point)

    def status(self):
        return {
            'id': six.text_type(self.rt_system_id),
            'timeout': self.timeout,
            'circuit_breaker': {
                'current_state': self.breaker.current_state,
                'fail_counter': self.breaker.fail_counter,
                'reset_timeout': self.breaker.reset_timeout,
            },
        }

    def _validate_response_or_raise(self, tree, ns):
        stop_monitoring_delivery = tree.find('.//siri:StopMonitoringDelivery', ns)
        if stop_monitoring_delivery is None:
            raise RealtimeProxyError('No StopMonitoringDelivery in response')

        status = stop_monitoring_delivery.find('.//siri:Status', ns)
        if status is not None and not to_bool(status.text):
            # Status is false: there is a problem, but we may have a valid response too...
            # Lets log whats happening
            error_condition = stop_monitoring_delivery.find('.//siri:ErrorCondition', ns)
            if error_condition is not None and list(error_condition):
                if error_condition.find('.//siri:NoInfoForTopicError', ns) is not None:
                    # There is no data, we might be at the end of the service
                    # OR the SIRI server doesn't update it's own data: there is no way to know
                    # let's say it's normal and not log nor return base_schedule data
                    return
                # Log the error returned by SIRI, there is a node for the normalized error code
                # and another node that holds the description
                code = " ".join([e.tag for e in list(error_condition) if 'Description' not in e.tag])
                description_node = error_condition.find('.//siri:Description', ns)
                description = description_node.text if description_node is not None else None
                logging.getLogger(__name__).warning('error in siri response: %s/%s', code, description)
            monitored_stops = stop_monitoring_delivery.findall('.//siri:MonitoredStopVisit', ns)
            if monitored_stops is None or len(monitored_stops) < 1:
                # we might want to ignore error that match siri:NoInfoForTopicError,
                # maybe it means that there is no next departure, maybe not...
                # There is no departures and status is false: this looks like a real error...
                # If description contains error message use it in exception (ex:[BAD_ID] MonitoringRef (01001713:TOC))
                message = description or 'response status = false'
                raise RealtimeProxyError(message)

    def _get_passages(self, tree, ns, route_point):

        stop = route_point.fetch_stop_id(self.object_id_tag)
        line = route_point.fetch_line_id(self.object_id_tag)
        route = route_point.fetch_route_id(self.object_id_tag)
        next_passages = []
        for visit in tree.findall('.//siri:MonitoredStopVisit', ns):
            cur_stop = visit.find('.//siri:StopPointRef', ns).text
            if stop != cur_stop:
                continue
            cur_line = visit.find('.//siri:LineRef', ns).text
            if line != cur_line:
                continue
            cur_route = visit.find('.//siri:DirectionName', ns).text
            if route != cur_route:
                continue
            # TODO? we should ignore MonitoredCall with a DepartureStatus set to "Cancelled"
            cur_destination = visit.find('.//siri:DestinationName', ns).text
            cur_dt = visit.find('.//siri:ExpectedDepartureTime', ns).text
            # TODO? fallback on siri:AimedDepartureTime if there is no ExpectedDepartureTime
            # In that case we may want to set realtime to False
            cur_dt = aniso8601.parse_datetime(cur_dt)
            next_passages.append(RealTimePassage(cur_dt, cur_destination))

        return next_passages

    @cache.memoize(app.config.get(str('CACHE_CONFIGURATION'), {}).get(str('TIMEOUT_SIRI'), 60))
    def _call_siri(self, request):
        encoded_request = request.encode('utf-8', 'backslashreplace')
        headers = {"Content-Type": "text/xml; charset=UTF-8", "Content-Length": str(len(encoded_request))}

        logging.getLogger(__name__).debug('siri RT service, post at {}: {}'.format(self.service_url, request))
        try:
            return self.breaker.call(
                requests.post,
                url=self.service_url,
                headers=headers,
                data=encoded_request,
                verify=False,
                timeout=self.timeout,
            )
        except pybreaker.CircuitBreakerError as e:
            logging.getLogger(__name__).error(
                'siri RT service dead, using base ' 'schedule (error: {}'.format(e)
            )
            raise RealtimeProxyError('circuit breaker open')
        except requests.Timeout as t:
            logging.getLogger(__name__).error(
                'siri RT service timeout, using base ' 'schedule (error: {}'.format(t)
            )
            raise RealtimeProxyError('timeout')
        except Exception as e:
            logging.getLogger(__name__).exception('siri RT error, using base schedule')
            raise RealtimeProxyError(str(e))

    def _make_request(self, dt, count, monitoring_ref):
        # we don't want to ask 1000 next departure to SIRI :)
        count = min(count or 5, 5)  # if no value defined we ask for 5 passages
        message_identifier = 'IDontCare'
        request = """<?xml version="1.0" encoding="UTF-8"?>
        <x:Envelope xmlns:x="http://schemas.xmlsoap.org/soap/envelope/"
                    xmlns:wsd="http://wsdl.siri.org.uk" xmlns:siri="http://www.siri.org.uk/siri">
          <x:Header/>
          <x:Body>
            <GetStopMonitoring xmlns="http://wsdl.siri.org.uk" xmlns:siri="http://www.siri.org.uk/siri">
              <ServiceRequestInfo xmlns="">
                <siri:RequestTimestamp>{dt}</siri:RequestTimestamp>
                <siri:RequestorRef>{RequestorRef}</siri:RequestorRef>
                <siri:MessageIdentifier>{MessageIdentifier}</siri:MessageIdentifier>
              </ServiceRequestInfo>
              <Request version="1.3" xmlns="">
                <siri:RequestTimestamp>{dt}</siri:RequestTimestamp>
                <siri:MessageIdentifier>{MessageIdentifier}</siri:MessageIdentifier>
                <siri:MonitoringRef>{MonitoringRef}</siri:MonitoringRef>
                <siri:MinimumStopVisitsPerLine>{count}</siri:MinimumStopVisitsPerLine>
              </Request>
              <RequestExtension xmlns=""/>
            </GetStopMonitoring>
          </x:Body>
        </x:Envelope>
        """.format(
            dt=floor_datetime(datetime.utcfromtimestamp(dt), self.from_datetime_step).isoformat(),
            count=count,
            RequestorRef=self.requestor_ref,
            MessageIdentifier=message_identifier,
            MonitoringRef=monitoring_ref,
        )
        return request

    def __eq__(self, other):
        return self.rt_system_id == other.rt_system_id
