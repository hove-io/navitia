# encoding: utf-8

# Copyright (c) 2001-2016, Canal TP and/or its affiliates. All rights reserved.
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
from flask import logging
import pybreaker
import requests as requests
from jormungandr import cache, app
from jormungandr.realtime_schedule.realtime_proxy import RealtimeProxy, RealtimeProxyError, floor_datetime
from jormungandr.schedule import RealTimePassage
import xml.etree.ElementTree as et
import aniso8601
from datetime import datetime


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
                    <siri:MaximumStopVisits>{nb_desired}</siri:MaximumStopVisits>
                </Request>
                <RequestExtension xmlns=""/>
            </GetStopMonitoring>
        </x:Body>
    </x:Envelope>'

    Then Navitia matches route-points in the response using {stop_code}, {route_code} and {line_code}.

    {datetime} is iso-formated: YYYY-mm-ddTHH:MM:ss.sss+HH:MM
    {stop_code}, {route_code} and {line_code} are provided using the same code key, named after
    the 'destination_id_tag' if provided on connector's init, or the 'id' otherwise.

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
    """

    def __init__(self, id, service_url, requestor_ref,
                 object_id_tag=None, destination_id_tag=None, instance=None, timeout=10, **kwargs):
        self.service_url = service_url
        self.requestor_ref = requestor_ref # login for siri
        self.timeout = timeout  #timeout in seconds
        self.rt_system_id = id
        self.object_id_tag = object_id_tag if object_id_tag else id
        self.destination_id_tag = destination_id_tag
        self.instance = instance
        self.breaker = pybreaker.CircuitBreaker(fail_max=app.config.get('CIRCUIT_BREAKER_MAX_SIRI_FAIL', 5),
                                                reset_timeout=app.config.get('CIRCUIT_BREAKER_SIRI_TIMEOUT_S', 60))
        self.step = kwargs.get('step', 30)

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
        return self._get_passages(siri_response.content, route_point)

    def status(self):
        return {
            'id': unicode(self.rt_system_id),
            'timeout': self.timeout,
            'circuit_breaker': {
                'current_state': self.breaker.current_state,
                'fail_counter': self.breaker.fail_counter,
                'reset_timeout': self.breaker.reset_timeout
            },
        }

    def _get_passages(self, xml, route_point):
        ns = {'siri': 'http://www.siri.org.uk/siri'}
        try:
            root = et.fromstring(xml)
        except et.ParseError as e:
            logging.getLogger(__name__).exception("invalid xml")
            raise RealtimeProxyError('invalid xml')

        stop = route_point.fetch_stop_id(self.object_id_tag)
        line = route_point.fetch_line_id(self.object_id_tag)
        route = route_point.fetch_route_id(self.object_id_tag)
        next_passages = []
        for visit in root.findall('.//siri:MonitoredStopVisit', ns):
            cur_stop = visit.find('.//siri:StopPointRef', ns).text
            if stop != cur_stop:
                continue
            cur_line = visit.find('.//siri:LineRef', ns).text
            if line != cur_line:
                continue
            cur_route = visit.find('.//siri:DirectionName', ns).text
            if route != cur_route:
                continue
            cur_destination = visit.find('.//siri:DestinationName', ns).text
            cur_dt = visit.find('.//siri:ExpectedDepartureTime', ns).text
            cur_dt = aniso8601.parse_datetime(cur_dt)
            next_passages.append(RealTimePassage(cur_dt, cur_destination))

        return next_passages

    @cache.memoize(app.config['CACHE_CONFIGURATION'].get('TIMEOUT_SIRI', 60))
    def _call_siri(self, request):
        encoded_request = request.encode('utf-8', 'backslashreplace')
        headers = {
            "Content-Type": "text/xml; charset=UTF-8",
            "Content-Length": len(encoded_request)
        }

        logging.getLogger(__name__).debug('siri RT service, post at {}: {}'.format(self.service_url, request))
        try:
            return self.breaker.call(requests.post,
                                     url=self.service_url,
                                     headers=headers,
                                     data=encoded_request,
                                     verify=False,
                                     timeout=self.timeout)
        except pybreaker.CircuitBreakerError as e:
            logging.getLogger(__name__).error('siri RT service dead, using base '
                                              'schedule (error: {}'.format(e))
            raise RealtimeProxyError('circuit breaker open')
        except requests.Timeout as t:
            logging.getLogger(__name__).error('siri RT service timeout, using base '
                                              'schedule (error: {}'.format(t))
            raise RealtimeProxyError('timeout')
        except Exception as e:
            logging.getLogger(__name__).exception('siri RT error, using base schedule')
            raise RealtimeProxyError(str(e))


    def _make_request(self, dt, count, monitoring_ref):
        #we don't want to ask 1000 next departure to SIRI :)
        count = min(count or 5, 5)# if no value defined we ask for 5 passages
        message_identifier='IDontCare'
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
                <siri:MaximumStopVisits>{count}</siri:MaximumStopVisits>
              </Request>
              <RequestExtension xmlns=""/>
            </GetStopMonitoring>
          </x:Body>
        </x:Envelope>
        """.format(dt=floor_datetime(datetime.utcfromtimestamp(dt), self.step).isoformat(),
                   count=count,
                   RequestorRef=self.requestor_ref,
                   MessageIdentifier=message_identifier,
                   MonitoringRef=monitoring_ref)
        return request




