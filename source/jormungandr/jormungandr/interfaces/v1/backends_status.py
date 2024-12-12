from jormungandr import i_manager
from jormungandr.protobuf_to_dict import protobuf_to_dict
from jormungandr.interfaces.v1.StatedResource import StatedResource
from navitiacommon import type_pb2, request_pb2
from jormungandr import exceptions
from collections import defaultdict
import gevent, gevent.pool
from jormungandr import app


class BackendsStatus(StatedResource):
    def __init__(self, *args, **kwargs):
        super().__init__(self, *args, **kwargs)

    def get(self):
        regions = i_manager.get_regions()

        response = {
            'lokis': {},
            'krakens': {},
            'errors': [],
        }

        pool = gevent.pool.Pool(app.config.get('BACKENDS_STATUS_GREENLET_POOL_SIZE', 10))

        def do(instance_name, pt_planner_type, pt_planner, req):
            try:
                resp = pt_planner.send_and_receive(req, request_id='backend_status')
                status = protobuf_to_dict(resp, use_enum_labels=True)
                is_loaded = status.get('status', {}).get('loaded') is True
                if not is_loaded:
                    return instance_name, pt_planner_type, status, 'data is not loaded'
                return instance_name, pt_planner_type, status, None
            except exceptions.DeadSocketException as e:
                return (
                    instance_name,
                    pt_planner_type,
                    None,
                    'instance {} backend {} did not respond because: {}'.format(
                        instance_name, pt_planner_type, str(e)
                    ),
                )

        futures = []

        req = request_pb2.Request()
        req.requested_api = type_pb2.STATUS

        for key_region in regions:
            instance = i_manager.instances[key_region]
            for pt_planner_type, pt_planner in instance.get_all_pt_planners():
                futures.append(pool.spawn(do, instance.name, pt_planner_type, pt_planner, req))

        found_err = False
        status_code = 200

        for future in gevent.iwait(futures):
            instance_name, pt_planner_type, status, err = future.get()
            found_err |= err is not None
            if err:
                response['errors'].append(
                    {'backend_type': pt_planner_type, 'backend_name': instance_name, 'error': err}
                )

            key = 'krakens' if pt_planner_type == 'kraken' else 'lokis'
            status_ = status.get('status', {}) if status is not None else {}
            response[key][instance_name] = {
                'status': status_.get('status'),
                'backend_version': status_.get('navitia_version'),
                'start_date': status_.get('start_production_date'),
                'end_date': status_.get('end_production_date'),
                'loaded': status_.get('loaded'),
                'last_load_at': status_.get('last_load_at'),
                'last_load_status': status_.get('last_load_status'),
                'is_realtime_loaded': status_.get('is_realtime_loaded'),
                'last_rt_data_loaded': status_.get('last_rt_data_loaded'),
            }

        if found_err:
            status_code = 503

        return response, status_code
