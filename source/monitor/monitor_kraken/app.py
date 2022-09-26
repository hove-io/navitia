# Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.hove.com).
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

from flask import Flask, request
import json
from ConfigParser import ConfigParser
import zmq
import os

from monitor_kraken import request_pb2
from monitor_kraken import response_pb2
from monitor_kraken import type_pb2

app = Flask(__name__)
# config used in monitor() for several kraken
app.config.from_object('monitor_kraken.default_settings')
app.config.from_envvar('MONITOR_CONFIG_FILE', silent=True)
context = zmq.Context()

# config used in status() health() and ready()
# for a single kraken
single_kraken_zmq_socket = os.environ.get("KRAKEN_GENERAL_zmq_socket")
single_kraken_zmq_timeout_in_ms = os.environ.get("MONITOR_KRAKEN_ZMQ_TIMEOUT", 10000)


def get_kraken_uri(instance_name):
    config_file = '{path}/{instance}/kraken.ini'.format(path=app.config['KRAKEN_DIR'], instance=instance_name)
    parser = ConfigParser()
    parser.read(config_file)
    try:
        uri = parser.get('GENERAL', 'zmq_socket')
    except:
        raise
    return uri


# Monitor several kraken instances through http://MONITOR_HOST/?instance=my_instance_name
# Where 'my_instance_name' must be a subdirectory of app.config['KRAKEN_DIR']
# that contains a 'kraken.ini' file
@app.route('/')
def monitor():
    instance = request.args.get('instance')
    if not instance:
        return json.dumps({'error': ['no instance given']}), 400

    try:
        uri = get_kraken_uri(instance)
    except:
        return json.dumps({'error': ['instance invalid']}), 500

    uri = uri.replace('*', 'localhost')
    sock = context.socket(zmq.REQ)
    # discard messages when socket closed
    sock.setsockopt(zmq.LINGER, 0)
    try:
        sock.connect(uri)
        req = request_pb2.Request()
        req.requested_api = type_pb2.STATUS
        sock.send(req.SerializeToString())
        if sock.poll(app.config['TIMEOUT']) < 1:
            return json.dumps({'status': 'timeout'}), 503

        pb = sock.recv()
        resp = response_pb2.Response()
        resp.ParseFromString(pb)

        response = {}
        return_code = 200
        if resp.error and resp.error.message:
            response['status'] = resp.error.message

        response['start_production_date'] = resp.status.start_production_date
        response['end_production_date'] = resp.status.end_production_date
        response['last_load'] = resp.status.last_load_at
        response['last_load_status'] = resp.status.last_load_status
        response['loaded'] = resp.status.loaded
        response['is_connected_to_rabbitmq'] = resp.status.is_connected_to_rabbitmq
        response['disruption_error'] = resp.status.disruption_error
        response['publication_date'] = resp.status.publication_date
        response['is_realtime_loaded'] = resp.status.is_realtime_loaded
        response['kraken_version'] = resp.status.navitia_version
        response['data_version'] = resp.status.data_version

        if resp.status.last_load_status == False and 'status' not in response:
            response['status'] = 'last load failed'

        if 'status' not in response:
            response['status'] = 'running'
        else:
            return_code = 503

        return json.dumps(response), return_code
    finally:
        sock.close()


# Status of a single kraken through http://MONITOR_HOST/status
# The kraken is accessed through the zmq socket defined in "KRAKEN_GENERAL_zmq_socket"
# Performs a zmq status request to kraken
# and dumps the response into json
@app.route('/status')
def status():
    if not single_kraken_zmq_socket:
        return json.dumps({'error': ['no zmq socket configured']}), 500
    proto_response = request_kraken_zmq_status(single_kraken_zmq_socket, single_kraken_zmq_timeout_in_ms)
    if proto_response == None:
        return json.dumps({'error': 'zmq timed out'}), 503

    json_response = {}
    if proto_response.error and proto_response.error.message:
        json_response['error'] = proto_response.error.message

    if proto_response.status:
        proto_status = proto_response.status
        json_response['publication_date'] = proto_status.publication_date
        json_response['start_production_date'] = proto_status.start_production_date
        json_response['end_production_date'] = proto_status.end_production_date
        json_response['data_version'] = proto_status.data_version
        json_response['navitia_version'] = proto_status.navitia_version
        json_response['data_sources'] = [source for source in proto_status.data_sources]
        json_response['last_load_at'] = proto_status.last_load_at
        json_response['last_load_status'] = proto_status.last_load_status
        json_response['loaded'] = proto_status.loaded
        json_response['nb_threads'] = proto_status.nb_threads
        json_response['is_connected_to_rabbitmq'] = proto_status.is_connected_to_rabbitmq
        json_response['status'] = proto_status.status
        json_response['last_rt_data_loaded'] = proto_status.last_rt_data_loaded
        json_response['is_realtime_loaded'] = proto_status.is_realtime_loaded
        json_response['dataset_created_at'] = proto_status.dataset_created_at
        json_response['rt_contributors'] = [source for source in proto_status.rt_contributors]
        json_response['disruption_error'] = proto_status.disruption_error

    if proto_response.metadatas:
        proto_metadatas = proto_response.metadatas
        json_response['timezone'] = proto_metadatas.timezone
        json_response['name'] = proto_metadatas.name

    return json.dumps(json_response), 200


# Health of a single kraken through http://MONITOR_HOST/health
# The kraken is accessed through the zmq socket defined in "KRAKEN_GENERAL_zmq_socket"
# Respond with a 200 when kraken gives a response on a zmq status request
@app.route('/health')
def health():
    if not single_kraken_zmq_socket:
        return json.dumps({'error': ['no zmq socket configured']}), 500
    proto_response = request_kraken_zmq_status(single_kraken_zmq_socket, single_kraken_zmq_timeout_in_ms)
    if proto_response == None:
        return json.dumps({'error': 'zmq timed out'}), 503
    return json.dumps("alive"), 200


# Readyness of a single kraken through http://MONITOR_HOST/ready
# The kraken is accessed through the zmq socket defined in "KRAKEN_GENERAL_zmq_socket"
# Respond with a 200 when :
#  - kraken gives a response on a zmq status request
#  - the "loaded" field of the status response is set to True
@app.route('/ready')
def ready():
    if not single_kraken_zmq_socket:
        return json.dumps({'error': ['no zmq socket configured']}), 500
    proto_response = request_kraken_zmq_status(single_kraken_zmq_socket, single_kraken_zmq_timeout_in_ms)
    if proto_response == None:
        return json.dumps({'error': 'zmq timed out'}), 503
    if proto_response.status == None:
        return json.dumps({'error': 'no status in zmq response'}), 500
    if proto_response.status.loaded == True:
        return json.dumps("ready"), 200
    return json.dumps("not ready"), 400


def request_kraken_zmq_status(zmq_socket, zmq_timeout_in_ms):
    sock = context.socket(zmq.REQ)
    # discard messages when socket closed
    sock.setsockopt(zmq.LINGER, 0)
    try:
        sock.connect(zmq_socket.replace('*', 'localhost'))
        req = request_pb2.Request()
        req.requested_api = type_pb2.STATUS
        sock.send(req.SerializeToString())
        if sock.poll(zmq_timeout_in_ms) < 1:
            return None

        response_bytes = sock.recv()
        proto_response = response_pb2.Response()
        proto_response.ParseFromString(response_bytes)
        return proto_response
    finally:
        sock.close()


if __name__ == '__main__':
    app.run(host=os.environ.get("MONITOR_HOST", "127.0.0.1"))
