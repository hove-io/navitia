#!/usr/bin/env python
import kombu
from navitiacommon import task_pb2
import datetime


connection = kombu.Connection()
exchange = kombu.Exchange('navitia', 'topic', durable=True)
producer = connection.Producer(exchange=exchange)

task = task_pb2.Task()
task.action = 2
task.at_perturbation.uri = 'testajn2'
#task.at_perturbation.object.object_uri = 'line:SIT10'
#task.at_perturbation.object.object_type = 1
task.at_perturbation.object.object_uri = 'stop_area:SIT150'
task.at_perturbation.object.object_type = 5
task.at_perturbation.start_application_date = 0
task.at_perturbation.end_application_date = 1390033727

task.at_perturbation.start_application_daily_hour = 0
task.at_perturbation.end_application_daily_hour = 86399

# for i in range(10000):
producer.publish(task.SerializeToString(), routing_key='realtime.at')

connection.close()
