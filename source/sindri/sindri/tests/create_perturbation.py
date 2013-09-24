#!/usr/bin/env python
import pika
import sindri.task_pb2

conn = pika.BlockingConnection()
channel = conn.channel()

task = sindri.task_pb2.Task()
task.action = 2
task.at_perturbation.uri = 'fooezzae'
task.at_perturbation.object.object_uri = 'foodzba'
task.at_perturbation.object.object_type = 1

#for i in range(10000):
channel.basic_publish(exchange='navitia', routing_key='realtime.at', body=task.SerializeToString())

conn.close()

