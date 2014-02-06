#!/usr/bin/env python
import kombu
from navitiacommon import task_pb2


connection = kombu.Connection()
exchange = kombu.Exchange('navitia', 'topic', durable=True)
producer = connection.Producer(exchange=exchange)

task = task_pb2.Task()
task.action = 1
task.message.uri = 'fooezzae'
task.message.object.object_uri = 'foodzba'
task.message.object.object_type = 1
task.message.localized_messages.add()
task.message.localized_messages[0].language = 'fr'
task.message.localized_messages[0].body = 'hello'
task.message.message_status = 0

# for i in range(10000):
producer.publish(body=task.SerializeToString(), routing_key='realtime.at')

connection.close()
