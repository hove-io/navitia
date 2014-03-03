from flask import Flask, request
import json
import datetime
from ConfigParser import ConfigParser
import zmq

from monitor_kraken import request_pb2
from monitor_kraken import response_pb2
from monitor_kraken import type_pb2

app = Flask(__name__)
app.config.from_object('monitor_kraken.default_settings')
app.config.from_envvar('MONITOR_CONFIG_FILE', silent=True)
context = zmq.Context()

@app.route('/')
def monitor():
    if 'instance' not in request.args:
        return json.dumps({'error': ['instance invalid']}), 400

    instance = request.args['instance']
    config_file = '{path}/{instance}/kraken.ini'.format(
                                            path=app.config['KRAKEN_DIR'],
                                            instance=instance)
    parser = ConfigParser()
    parser.read(config_file)
    try:
        uri = parser.get('GENERAL', 'zmq_socket')
    except:
        return json.dumps({'error': ['instance invalid']}), 500

    uri.replace('*', 'localhost')
    sock = context.socket(zmq.REQ)
    try:
        sock.connect(uri)
        req = request_pb2.Request()
        req.requested_api = type_pb2.STATUS
        sock.send(req.SerializeToString())
        if sock.poll(10000) < 1:
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
        response['is_connected_to_rabbitmq'] = resp.is_connected_to_rabbitmq

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
