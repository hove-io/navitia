#!/usr/bin/env python
import pika
import sindri.task_pb2

conn = pika.BlockingConnection()
channel = conn.channel()

task = sindri.task_pb2.Task()
task.action = 1
task.message.uri = 'fooezzae'
task.message.object.object_uri = 'foodzba'
task.message.object.object_type = 1
task.message.localized_messages.add()
task.message.localized_messages[0].language = 'fr'
task.message.localized_messages[0].body = 'hello'

#for i in range(10000):
channel.basic_publish(exchange='navitia', routing_key='realtime.at', body=task.SerializeToString())

conn.close()

