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
app.config.from_object('monitor_kraken.default_settings')
app.config.from_envvar('MONITOR_CONFIG_FILE', silent=True)
context = zmq.Context()


def get_kraken_uri(instance_name):
    uri = os.environ.get("KRAKEN_GENERAL_zmq_socket")
    if uri:
        return uri
    config_file = '{path}/{instance}/kraken.ini'.format(path=app.config['KRAKEN_DIR'], instance=instance_name)
    parser = ConfigParser()
    parser.read(config_file)
    try:
        uri = parser.get('GENERAL', 'zmq_socket')
    except:
        raise
    return uri


@app.route('/')
def monitor():
    instance = request.args.get('instance')
    if not instance:
        return json.dumps({'error': ['instance invalid']}), 400

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


if __name__ == '__main__':
    app.run()
