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

import requests as requests
import logging


class Siri(object):
    """
    Class managing the POC to test how to call and parse le service siri for Orleans
    """

    def __init__(self, service_url, requestor_ref, timeout=1):

        self.timeout = timeout
        self.service_url = service_url
        self.requestor_ref = requestor_ref

    def _call_siri(self, request):
        """
        :param request:
        :return:
        """
        '''
        #Call to the web service of ogv1
        request = '<?xml version="1.0" encoding="UTF-8"?>' \
        '<soap:Envelope xmlns:soap="http://www.w3.org/2003/05/soap-envelope" xmlns:sncf="http://www.sncf.com/">'\
        '<soap:Header/>'\
        '<soap:Body>'\
        '<sncf:GetEventList>'\
        '<sncf:lineRef>83</sncf:lineRef>'\
        '</sncf:GetEventList>'\
        '</soap:Body>'\
        '</soap:Envelope>'
        encoded_request = request.encode('UTF-8')

        headers = {"Content-Type": "text/xml; charset=UTF-8",
                   "Content-Length": len(encoded_request)}

        response = requests.post(url="http://ogv1-ws.fr-tn-priv.customer.canaltp.fr",
                                 headers=headers, data=encoded_request,
                                 verify=False)

        '''
        encoded_request = request.encode('UTF-8')

        headers = {"Content-Type": "text/xml; charset=UTF-8",
                   "Content-Length": len(encoded_request)}

        response = requests.post(url=self.service_url, headers=headers, data=encoded_request, verify=False, timeout=self.timeout)

        return response.text

    def _make_request(self, message_identifier='StopMonitoringClient:Test:0',
                      monitoring_ref='Orleans:StopPoint:BP:TPOSTA1:LOC',
                      stop_visit_types='all'):

        '''
        request = '<?xml version="1.0" encoding="UTF-8"?>' \
                  '<soapenv:Envelope xmlns:soapenv="http://schemas.xmlsoap.org/soap/envelope/">' \
                  '<soapenv:Header/>' \
                  '<soapenv:Body>' \
                  '<GetStopMonitoring xmlns="http://wsdl.siri.org.uk" xmlns:siri="http://www.siri.org.uk/siri">' \
                  '<ServiceRequestInfo xmlns="">' \
                  '<siri:RequestTimestamp>2017-02-02T11:10:12.788+01:00</siri:RequestTimestamp>' \
                  '<siri:RequestorRef>{RequestorRef}</siri:RequestorRef>' \
                  '<siri:MessageIdentifier>{MessageIdentifier}</siri:MessageIdentifier>' \
                  '</ServiceRequestInfo>' \
                  '<Request version="1.3" xmlns="">' \
                  '<siri:RequestTimestamp>2017-02-02T11:10:12.788+01:00</siri:RequestTimestamp>' \
                  '<siri:MessageIdentifier>{MessageIdentifier}</siri:MessageIdentifier>' \
                  '<siri:MonitoringRef>{MonitoringRef}</siri:MonitoringRef>' \
                  '<siri:StopVisitTypes>{StopVisitTypes}</siri:StopVisitTypes>' \
                  '</Request>' \
                  '<RequestExtension xmlns=""/>' \
                  '</GetStopMonitoring>' \
                  '</soapenv:Body>' \
                  '</soapenv:Envelope>'.format(RequestorRef=self.requestor_ref,
                                               MessageIdentifier=message_identifier,
                                               MonitoringRef=monitoring_ref,
                                               StopVisitTypes=stop_visit_types)

        '''
        request = '<?xml version="1.0" encoding="UTF-8"?>' \
                  '<x:Envelope xmlns:x="http://schemas.xmlsoap.org/soap/envelope/">' \
                  '<x:Header/>' \
                  '<x:Body>' \
                  '<GetStopMonitoring xmlns="http://wsdl.siri.org.uk" xmlns:siri="http://www.siri.org.uk/siri">' \
                  '<ServiceRequestInfo xmlns="">' \
                  '<siri:RequestTimestamp>2017-02-02T11:10:12.788+01:00</siri:RequestTimestamp>' \
                  '<siri:RequestorRef>{RequestorRef}</siri:RequestorRef>' \
                  '<siri:MessageIdentifier>{MessageIdentifier}</siri:MessageIdentifier>' \
                  '</ServiceRequestInfo>' \
                  '<Request version="1.3" xmlns="">' \
                  '<siri:RequestTimestamp>2017-02-02T11:10:12.788+01:00</siri:RequestTimestamp>' \
                  '<siri:MessageIdentifier>{MessageIdentifier}</siri:MessageIdentifier>' \
                  '<siri:MonitoringRef>{MonitoringRef}</siri:MonitoringRef>' \
                  '<siri:StopVisitTypes>{StopVisitTypes}</siri:StopVisitTypes>' \
                  '</Request>' \
                  '<RequestExtension xmlns=""/>' \
                  '</GetStopMonitoring>' \
                  '</x:Body>' \
                  '</x:Envelope>'.format(RequestorRef=self.requestor_ref,
                                         MessageIdentifier=message_identifier,
                                         MonitoringRef=monitoring_ref,
                                         StopVisitTypes=stop_visit_types)
        return request




