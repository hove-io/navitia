# coding=utf-8
# Copyright (c) 2001-2017, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
# the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.kisio.com).
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
from __future__ import absolute_import, print_function, division
import datetime
from dateutil.parser import parse
import mock
import pytz
from jormungandr.realtime_schedule.siri import Siri
import validators
from jormungandr.realtime_schedule.tests.utils import MockRoutePoint, _timestamp
import xml.etree.ElementTree as et
import requests_mock


def make_request_test():
    siri = Siri(id='tata', service_url='http://bob.com/', requestor_ref='Stibada', from_datetime_step=30)

    request = siri._make_request(dt=_timestamp("12:00"), count=2, monitoring_ref='Tri:SP:toto:LOC')

    # it should be a valid xml
    root = et.fromstring(request)
    ns = {'siri': 'http://www.siri.org.uk/siri'}

    assert root.find('.//siri:MonitoringRef', ns).text == 'Tri:SP:toto:LOC'
    assert root.find('.//siri:MinimumStopVisitsPerLine', ns).text == '2'
    assert root.find('.//siri:RequestTimestamp', ns).text == '2016-02-07T12:00:00'
    assert root.find('.//siri:RequestorRef', ns).text == 'Stibada'

    request = siri._make_request(dt=_timestamp("12:00:12"), count=2, monitoring_ref='Tri:SP:toto:LOC')
    root = et.fromstring(request)
    assert root.find('.//siri:RequestTimestamp', ns).text == '2016-02-07T12:00:00'

    request = siri._make_request(dt=_timestamp("12:00:33"), count=2, monitoring_ref='Tri:SP:toto:LOC')
    root = et.fromstring(request)
    assert root.find('.//siri:RequestTimestamp', ns).text == '2016-02-07T12:00:30'


def mock_good_with_error_response():
    return """<?xml version="1.0"?>
<soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/">
  <soap:Body>
    <ns1:GetStopMonitoringResponse xmlns:ns1="http://wsdl.siri.org.uk">
      <ServiceDeliveryInfo xmlns:ns2="http://www.ifopt.org.uk/acsb" xmlns:ns3="http://datex2.eu/schema/1_0/1_0" xmlns:ns4="http://www.ifopt.org.uk/ifopt" xmlns:ns5="http://www.siri.org.uk/siri">
        <ns5:ResponseTimestamp>2017-03-02T11:27:09.886+01:00</ns5:ResponseTimestamp>
        <ns5:ProducerRef>Orleans</ns5:ProducerRef>
        <ns5:ResponseMessageIdentifier>Orleans:SM:RQ:331</ns5:ResponseMessageIdentifier>
        <ns5:RequestMessageRef>StopMonitoringClient:Test:0</ns5:RequestMessageRef>
      </ServiceDeliveryInfo>
      <Answer xmlns:ns2="http://www.ifopt.org.uk/acsb" xmlns:ns3="http://datex2.eu/schema/1_0/1_0" xmlns:ns4="http://www.ifopt.org.uk/ifopt" xmlns:ns5="http://www.siri.org.uk/siri">
        <ns5:StopMonitoringDelivery version="1.3">
          <ns5:ResponseTimestamp>2017-03-02T11:27:09.886+01:00</ns5:ResponseTimestamp>
          <ns5:RequestMessageRef>StopMonitoringClient:Test:0</ns5:RequestMessageRef>
          <ns5:Status>false</ns5:Status>
          <ns5:ErrorCondition>
            <ns5:ParametersIgnoredError/>
            <ns5:Description>...</ns5:Description>
          </ns5:ErrorCondition>
          <ns5:MonitoredStopVisit>
            <ns5:RecordedAtTime>2017-03-02T03:30:34.000+01:00</ns5:RecordedAtTime>
            <ns5:ItemIdentifier>Orleans:ItemId::268517466:LOC</ns5:ItemIdentifier>
            <ns5:MonitoringRef>Orleans:StopPoint:BP:TJDARC1:LOC</ns5:MonitoringRef>
            <ns5:MonitoredVehicleJourney>
              <ns5:LineRef>line_toto</ns5:LineRef>
              <ns5:FramedVehicleJourneyRef>
                <ns5:DataFrameRef>Orleans:Version:3:LOC</ns5:DataFrameRef>
                <ns5:DatedVehicleJourneyRef>Orleans:VehicleJourney::B_R_175_10_B09_5_11:06:00:LOC</ns5:DatedVehicleJourneyRef>
              </ns5:FramedVehicleJourneyRef>
              <ns5:JourneyPatternRef>Orleans:JourneyPattern::B_R_175:LOC</ns5:JourneyPatternRef>
              <ns5:PublishedLineName>G. POMPIDOU - CLOS DU HAMEAU</ns5:PublishedLineName>
              <ns5:DirectionName>route_tata</ns5:DirectionName>
              <ns5:VehicleFeatureRef/>
              <ns5:DestinationRef>stop_destination</ns5:DestinationRef>
              <ns5:DestinationName>Georges Pompidou</ns5:DestinationName>
              <ns5:Monitored>false</ns5:Monitored>
              <ns5:MonitoredCall>
                <ns5:StopPointRef>stop_tutu</ns5:StopPointRef>
                <ns5:Order>15</ns5:Order>
                <ns5:StopPointName>Jeanne d'Arc</ns5:StopPointName>
                <ns5:VehicleAtStop>false</ns5:VehicleAtStop>
                <ns5:PlatformTraversal>false</ns5:PlatformTraversal>
                <ns5:DestinationDisplay>GEORGES POMPIDOU</ns5:DestinationDisplay>
                <ns5:AimedArrivalTime>2016-03-29T13:30:00.000+00:00</ns5:AimedArrivalTime>
                <ns5:ExpectedArrivalTime>2016-03-29T13:37:00.000+00:00</ns5:ExpectedArrivalTime>
                <ns5:ArrivalStatus>noReport</ns5:ArrivalStatus>
                <ns5:ArrivalPlatformName/>
                <ns5:AimedDepartureTime>2016-03-29T13:30:00.000+00:00</ns5:AimedDepartureTime>
                <ns5:ExpectedDepartureTime>2016-03-29T13:37:00.000+00:00</ns5:ExpectedDepartureTime>
                <ns5:DepartureStatus>noReport</ns5:DepartureStatus>
                <ns5:DeparturePlatformName/>
              </ns5:MonitoredCall>
            </ns5:MonitoredVehicleJourney>
          </ns5:MonitoredStopVisit>
        </ns5:StopMonitoringDelivery>
      </Answer>
      <AnswerExtension xmlns:ns2="http://www.ifopt.org.uk/acsb" xmlns:ns3="http://datex2.eu/schema/1_0/1_0" xmlns:ns4="http://www.ifopt.org.uk/ifopt" xmlns:ns5="http://www.siri.org.uk/siri"/>
    </ns1:GetStopMonitoringResponse>
  </soap:Body>
</soap:Envelope>
    """


def mock_good_response():
    return """<?xml version="1.0"?>
<soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/">
  <soap:Body>
    <ns1:GetStopMonitoringResponse xmlns:ns1="http://wsdl.siri.org.uk">
      <ServiceDeliveryInfo xmlns:ns2="http://www.ifopt.org.uk/acsb" xmlns:ns3="http://datex2.eu/schema/1_0/1_0" xmlns:ns4="http://www.ifopt.org.uk/ifopt" xmlns:ns5="http://www.siri.org.uk/siri">
        <ns5:ResponseTimestamp>2017-03-02T11:27:09.886+01:00</ns5:ResponseTimestamp>
        <ns5:ProducerRef>Orleans</ns5:ProducerRef>
        <ns5:ResponseMessageIdentifier>Orleans:SM:RQ:331</ns5:ResponseMessageIdentifier>
        <ns5:RequestMessageRef>StopMonitoringClient:Test:0</ns5:RequestMessageRef>
      </ServiceDeliveryInfo>
      <Answer xmlns:ns2="http://www.ifopt.org.uk/acsb" xmlns:ns3="http://datex2.eu/schema/1_0/1_0" xmlns:ns4="http://www.ifopt.org.uk/ifopt" xmlns:ns5="http://www.siri.org.uk/siri">
        <ns5:StopMonitoringDelivery version="1.3">
          <ns5:ResponseTimestamp>2017-03-02T11:27:09.886+01:00</ns5:ResponseTimestamp>
          <ns5:RequestMessageRef>StopMonitoringClient:Test:0</ns5:RequestMessageRef>
          <ns5:Status>true</ns5:Status>
          <ns5:MonitoredStopVisit>
            <ns5:RecordedAtTime>2017-03-02T03:30:34.000+01:00</ns5:RecordedAtTime>
            <ns5:ItemIdentifier>Orleans:ItemId::268517466:LOC</ns5:ItemIdentifier>
            <ns5:MonitoringRef>Orleans:StopPoint:BP:TJDARC1:LOC</ns5:MonitoringRef>
            <ns5:MonitoredVehicleJourney>
              <ns5:LineRef>line_toto</ns5:LineRef>
              <ns5:FramedVehicleJourneyRef>
                <ns5:DataFrameRef>Orleans:Version:3:LOC</ns5:DataFrameRef>
                <ns5:DatedVehicleJourneyRef>Orleans:VehicleJourney::B_R_175_10_B09_5_11:06:00:LOC</ns5:DatedVehicleJourneyRef>
              </ns5:FramedVehicleJourneyRef>
              <ns5:JourneyPatternRef>Orleans:JourneyPattern::B_R_175:LOC</ns5:JourneyPatternRef>
              <ns5:PublishedLineName>G. POMPIDOU - CLOS DU HAMEAU</ns5:PublishedLineName>
              <ns5:DirectionName>route_tata</ns5:DirectionName>
              <ns5:VehicleFeatureRef/>
              <ns5:DestinationRef>stop_destination</ns5:DestinationRef>
              <ns5:DestinationName>Georges Pompidou</ns5:DestinationName>
              <ns5:Monitored>false</ns5:Monitored>
              <ns5:MonitoredCall>
                <ns5:StopPointRef>stop_tutu</ns5:StopPointRef>
                <ns5:Order>15</ns5:Order>
                <ns5:StopPointName>Jeanne d'Arc</ns5:StopPointName>
                <ns5:VehicleAtStop>false</ns5:VehicleAtStop>
                <ns5:PlatformTraversal>false</ns5:PlatformTraversal>
                <ns5:DestinationDisplay>GEORGES POMPIDOU</ns5:DestinationDisplay>
                <ns5:AimedArrivalTime>2016-03-29T13:30:00.000+00:00</ns5:AimedArrivalTime>
                <ns5:ExpectedArrivalTime>2016-03-29T13:37:00.000+00:00</ns5:ExpectedArrivalTime>
                <ns5:ArrivalStatus>noReport</ns5:ArrivalStatus>
                <ns5:ArrivalPlatformName/>
                <ns5:AimedDepartureTime>2016-03-29T13:30:00.000+00:00</ns5:AimedDepartureTime>
                <ns5:ExpectedDepartureTime>2016-03-29T13:37:00.000+00:00</ns5:ExpectedDepartureTime>
                <ns5:DepartureStatus>noReport</ns5:DepartureStatus>
                <ns5:DeparturePlatformName/>
              </ns5:MonitoredCall>
            </ns5:MonitoredVehicleJourney>
          </ns5:MonitoredStopVisit>
        </ns5:StopMonitoringDelivery>
      </Answer>
      <AnswerExtension xmlns:ns2="http://www.ifopt.org.uk/acsb" xmlns:ns3="http://datex2.eu/schema/1_0/1_0" xmlns:ns4="http://www.ifopt.org.uk/ifopt" xmlns:ns5="http://www.siri.org.uk/siri"/>
    </ns1:GetStopMonitoringResponse>
  </soap:Body>
</soap:Envelope>
    """


def mock_error_response():
    return """<?xml version="1.0" encoding="ISO-8859-1"?>
<soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/">
  <soap:Body>
    <ns1:GetStopMonitoringResponse xmlns:ns1="http://wsdl.siri.org.uk">
      <ServiceDeliveryInfo xmlns:ns2="http://www.ifopt.org.uk/acsb" xmlns:ns3="http://datex2.eu/schema/1_0/1_0" xmlns:ns4="http://www.ifopt.org.uk/ifopt" xmlns:ns5="http://www.siri.org.uk/siri">
        <ns5:ResponseTimestamp>2019-01-02T07:57:03.919+01:00</ns5:ResponseTimestamp>
        <ns5:ProducerRef>Orleans</ns5:ProducerRef>
        <ns5:ResponseMessageIdentifier>Orleans:SM:RQ:423078</ns5:ResponseMessageIdentifier>
        <ns5:RequestMessageRef>IDontCare</ns5:RequestMessageRef>
      </ServiceDeliveryInfo>
      <Answer xmlns:ns2="http://www.ifopt.org.uk/acsb" xmlns:ns3="http://datex2.eu/schema/1_0/1_0" xmlns:ns4="http://www.ifopt.org.uk/ifopt" xmlns:ns5="http://www.siri.org.uk/siri">
        <ns5:StopMonitoringDelivery>
          <ns5:ResponseTimestamp>2019-01-02T07:57:03.919+01:00</ns5:ResponseTimestamp>
          <ns5:RequestMessageRef>IDontCare</ns5:RequestMessageRef>
          <ns5:Status>false</ns5:Status>
          <ns5:ErrorCondition>
            <ns5:OtherError/>
            <ns5:Description>[BAD_ID] MonitoringRef (00052200)</ns5:Description>
          </ns5:ErrorCondition>
        </ns5:StopMonitoringDelivery>
      </Answer>
      <AnswerExtension xmlns:ns2="http://www.ifopt.org.uk/acsb" xmlns:ns3="http://datex2.eu/schema/1_0/1_0" xmlns:ns4="http://www.ifopt.org.uk/ifopt" xmlns:ns5="http://www.siri.org.uk/siri"/>
    </ns1:GetStopMonitoringResponse>
  </soap:Body>
</soap:Envelope>
    """


def mock_no_data_response():
    return """<?xml version="1.0"?>
<soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/">
  <soap:Body>
    <ns1:GetStopMonitoringResponse xmlns:ns1="http://wsdl.siri.org.uk">
      <ServiceDeliveryInfo xmlns:ns2="http://www.ifopt.org.uk/acsb" xmlns:ns3="http://datex2.eu/schema/1_0/1_0" xmlns:ns4="http://www.ifopt.org.uk/ifopt" xmlns:ns5="http://www.siri.org.uk/siri">
        <ns5:ResponseTimestamp>2018-12-31T09:11:27.681+01:00</ns5:ResponseTimestamp>
        <ns5:ProducerRef>Orleans</ns5:ProducerRef>
        <ns5:ResponseMessageIdentifier>Orleans:SM:RQ:630908</ns5:ResponseMessageIdentifier>
        <ns5:RequestMessageRef>IDontCare</ns5:RequestMessageRef>
      </ServiceDeliveryInfo>
      <Answer xmlns:ns2="http://www.ifopt.org.uk/acsb" xmlns:ns3="http://datex2.eu/schema/1_0/1_0" xmlns:ns4="http://www.ifopt.org.uk/ifopt" xmlns:ns5="http://www.siri.org.uk/siri">
        <ns5:StopMonitoringDelivery>
          <ns5:ResponseTimestamp>2018-12-31T09:11:27.681+01:00</ns5:ResponseTimestamp>
          <ns5:RequestMessageRef>IDontCare</ns5:RequestMessageRef>
          <ns5:Status>false</ns5:Status>
          <ns5:ErrorCondition>
            <ns5:NoInfoForTopicError/>
            <ns5:Description>Pas de donn&#xE9;e</ns5:Description>
          </ns5:ErrorCondition>
        </ns5:StopMonitoringDelivery>
      </Answer>
      <AnswerExtension xmlns:ns2="http://www.ifopt.org.uk/acsb" xmlns:ns3="http://datex2.eu/schema/1_0/1_0" xmlns:ns4="http://www.ifopt.org.uk/ifopt" xmlns:ns5="http://www.siri.org.uk/siri"/>
    </ns1:GetStopMonitoringResponse>
  </soap:Body>
</soap:Envelope>
    """


def next_passage_for_route_point_test():
    """
    test the whole next_passage_for_route_point
    mock the http call to return a good response, we should get some next_passages
    """
    siri = Siri(id='tata', service_url='http://bob.com/', requestor_ref='Stibada')
    route_point = MockRoutePoint(route_id='route_tata', line_id='line_toto', stop_id='stop_tutu')

    with requests_mock.Mocker() as m:
        m.post('http://bob.com/', text=mock_good_response())
        passages = siri._get_next_passage_for_route_point(
            route_point, from_dt=_timestamp("12:00"), current_dt=_timestamp("12:00"), count=1
        )
        assert m.called
        assert len(passages) == 1
        assert passages[0].datetime == datetime.datetime(2016, 3, 29, 13, 37, tzinfo=pytz.UTC)


def next_passage_for_route_point_failure_test():
    """
    test the whole next_passage_for_route_point

    the siri's response is in error (status = 404), we should get 'None'
    """
    siri = Siri(id='tata', service_url='http://bob.com/', requestor_ref='Stibada')

    route_point = MockRoutePoint(route_id='route_tata', line_id='line_toto', stop_id='stop_tutu')

    with requests_mock.Mocker() as m:
        m.post('http://bob.com/', text=mock_good_response(), status_code=404)
        passages = siri.next_passage_for_route_point(route_point, from_dt=_timestamp("12:00"), count=2)
        assert m.called

        assert passages is None


def next_passage_for_route_point_error_test():
    """
    test the whole next_passage_for_route_point

    the siri's response contains an error, we should get 'None'
    """
    siri = Siri(id='tata', service_url='http://bob.com/', requestor_ref='Stibada')

    route_point = MockRoutePoint(route_id='route_tata', line_id='line_toto', stop_id='stop_tutu')

    with requests_mock.Mocker() as m:
        m.post('http://bob.com/', text=mock_error_response(), status_code=200)
        passages = siri.next_passage_for_route_point(route_point, from_dt=_timestamp("12:00"), count=2)
        assert m.called

        assert passages is None


def next_passage_for_route_point_no_data_test():
    """
    test the whole next_passage_for_route_point

    the siri's response contains an error of type NoInfoForTopicError: there is no next departure
    """
    siri = Siri(id='tata', service_url='http://bob.com/', requestor_ref='Stibada')

    route_point = MockRoutePoint(route_id='route_tata', line_id='line_toto', stop_id='stop_tutu')

    with requests_mock.Mocker() as m:
        m.post('http://bob.com/', text=mock_no_data_response(), status_code=200)
        passages = siri.next_passage_for_route_point(route_point, from_dt=_timestamp("12:00"), count=2)
        assert m.called

        assert passages == []


def next_passage_for_route_point_good_response_with_error_test():
    """
    mock the http call to return a good response that also contains an error, we should get some next_passages
    """
    siri = Siri(id='tata', service_url='http://bob.com/', requestor_ref='Stibada')
    route_point = MockRoutePoint(route_id='route_tata', line_id='line_toto', stop_id='stop_tutu')

    with requests_mock.Mocker() as m:
        m.post('http://bob.com/', text=mock_good_with_error_response())
        passages = siri._get_next_passage_for_route_point(
            route_point, from_dt=_timestamp("12:00"), current_dt=_timestamp("12:00"), count=1
        )
        assert m.called
        assert len(passages) == 1
        assert passages[0].datetime == datetime.datetime(2016, 3, 29, 13, 37, tzinfo=pytz.UTC)


def status_test():
    siri = Siri(id=u"tata-é$~#@\"*!'`§èû", service_url='http://bob.com/', requestor_ref='Stibada')
    status = siri.status()
    assert status['id'] == u'tata-é$~#@"*!\'`§èû'
