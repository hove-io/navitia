#!/usr/bin/env python
import kombu
from stat_persistor import stat_pb2
import datetime
import time

connection = kombu.Connection("amqp://guest:guest@localhost:5672//")
exchange_name = "navitia"
topic_name = "stat.sender"
exchange_object = kombu.Exchange(exchange_name, 'topic', durable = True)
producer = connection.Producer(exchange=exchange_object)

stat_request = stat_pb2.StatRequest()
dt = datetime.datetime.now()
stat_request.request_date = int(time.mktime(dt.timetuple()))
stat_request.user_id = 1
stat_request.user_name = 'Toto'
stat_request.application_id = 2
stat_request.application_name = 'navitia2'
stat_request.request_duration = 4500
stat_request.api = 'places'
stat_request.query_string = '/v1/coverage/centre/places?q=gare'
stat_request.host = 'http://navitia2-ws.ctp.dev.canaltp.fr'
stat_request.client = '127.0.0.1'
stat_request.response_size = 400
stat_request.region_id = 'paris'
producer.publish(stat_request.SerializeToString(), routing_key= topic_name)

connection.close()
