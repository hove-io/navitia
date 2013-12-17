#!/usr/bin/env python
import pika
import sindri.task_pb2
import datetime

conn = pika.BlockingConnection()
channel = conn.channel()

task = sindri.task_pb2.Task()
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
channel.basic_publish(exchange='navitia', routing_key='realtime.at',
                      body=task.SerializeToString())

conn.close()
