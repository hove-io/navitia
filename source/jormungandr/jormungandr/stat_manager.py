# coding=utf-8
import navitiacommon.response_pb2 as response_pb2
from flask_restful import reqparse, abort
import flask_restful
from flask import current_app, request
from functools import wraps
from stat_persistor import stat_pb2
from navitiacommon import request_pb2
from datetime import datetime, timedelta
from datetime import datetime
import time
import sys
import kombu
f_datetime = "%Y%m%dT%H%M%S"

class StatManager(object):

    def __init__(self):
        self.connection = None
        self.producer = None
        self.broker_url = None
        self.exchange_name = None
        self.topic_name = None

    def init(self, broker_url, exchange_name, topic_name):
        self.broker_url = broker_url
        self.exchange_name = exchange_name
        self.topic_name = topic_name
        self._init_rabbitmq()

    def _init_rabbitmq(self):
        """
        connection to rabbitmq and initialize queues
        """
        self.connection = kombu.Connection(self.broker_url)
        exchange = kombu.Exchange(self.exchange_name, 'topic', durable = True)
        self.producer = self.connection.Producer(exchange=exchange)

    def manage_stat(self, start_time, func_call):
        """
        Function to fill stat objects (requests, parameters, journeys et sections) sand send them to Broker
        """
        end_time = time.time()
        response_duration = (end_time - start_time) #In seconds
        stat_request = stat_pb2.StatRequest()
        stat_request.request_duration = int(response_duration * 1000) #In milliseconds
        self.fill_request(stat_request, func_call)
        self.fill_parameters(stat_request, func_call)
        if 'journeys' in request.endpoint:
            self.fill_journeys(stat_request, func_call)
        self.publish_request(stat_request)

    def fill_request(self, stat_request, func_call):
        """
        Remplir requests
        """
        dt = datetime.now()
        stat_request.request_date = int(time.mktime(dt.timetuple()))
        stat_request.user_id = 1 #??
        stat_request.user_name = 'Toto' #??
        stat_request.application_id = 2 #??
        stat_request.application_name = 'navitia2' #??

        stat_request.api = request.endpoint
        stat_request.query_string = request.url
        stat_request.host = request.host_url
        stat_request.response_size = 400
        stat_request.client = request.remote_addr


    def fill_parameters(self, stat_request, func_call):
        stat_parameter = stat_request.parameters.add()
        stat_parameter.key = 'key1'
        stat_parameter.value = 'value1'

    def fill_journey(self, stat_journey, resp_journey):
        """"
        Fill journey and all sections:
        """
        req_date_time = resp_journey["requested_date_time"]
        dep_date_time = resp_journey["departure_date_time"]
        arr_date_time = resp_journey["arrival_date_time"]

        stat_journey.requested_date_time = int(time.mktime(datetime.strptime(req_date_time, f_datetime).timetuple()))
        stat_journey.departure_date_time = int(time.mktime(datetime.strptime(dep_date_time, f_datetime).timetuple()))
        stat_journey.arrival_date_time = int(time.mktime(datetime.strptime(arr_date_time, f_datetime).timetuple()))
        stat_journey.duration = resp_journey.duration
        stat_journey.nb_transfers = resp_journey.nb_transfers
        stat_journey.type = resp_journey.type

        self.fill_sections(stat_journey, resp_journey)

    def fill_journeys(self, stat_request, func_call):
        """
        Fill journeys and sections for each journey
        """
        for resp_journey in func_call[0]['journeys']:
        #for resp_journey in func_call.journeys:
            stat_journey = stat_request.journeys.add()
            self.fill_journey(stat_journey, resp_journey)
            """
            self.fill_sections(stat_journey, resp_journey)
            """

    def fill_section(self, stat_section, resp_section):
        stat_section.departure_date_time = resp_section.departure_date_time
        stat_section.arrival_date_time = resp_section.arrival_date_time
        stat_section.duration = resp_section.duration
        stat_section.type = resp_section.type
        stat_section.from_embedded_type = resp_section
        stat_section.from_id = resp_section.from_id
        #stat_section.from_coord = resp_section.from_coord
        stat_section.to_embedded_type = resp_section.to_embedded_type
        stat_section.to_id = resp_section.to_id
        #section.to_coord = resp_section.to_coord
        stat_section.vehicle_journey_id = resp_section.vehicle_journey_id
        stat_section.line_id = resp_section.line_id
        stat_section.route_id = resp_section.route_id
        stat_section.network_id = resp_section.network_id
        stat_section.physical_mode_id = resp_section.physical_mode_id
        stat_section.commercial_mode_id = resp_section.commercial_mode_id

    def fill_sections(self, stat_journey, resp_journey):

        for resp_section in resp_journey.sections:
            stat_section = stat_journey.sections.add()
            self.fill_section(stat_section, resp_section)

    def publish_request(self, stat_request):
        self.producer.publish(stat_request.SerializeToString(), routing_key=self.topic_name)




















