from jormungandr import utils
import datetime
import logging

from jormungandr.utils import can_connect_to_database

DEFAULT_PT_PLANNER = 'kraken'


class NoRequestedPtPlanner(Exception):
    pass


class PtPlannersManager(object):
    def __init__(
        self,
        configs,
        name,
        zmq_socket_type,
        zmq_context,
        default_socket_path,
        db_configs_getter=None,
        update_interval=60,
    ):

        self.pt_planners = {}
        self.db_configs_getter = db_configs_getter
        self.update_interval = update_interval
        self.last_update = datetime.datetime(1970, 1, 1)
        self.name = name
        self.zmq_socket_type = zmq_socket_type
        self.zmq_context = zmq_context
        self.default_socket_path = default_socket_path
        # For the sake of retro-compatibility, configs may be None
        configs = configs or {}
        self.logger = logging.getLogger(__name__)
        self.old_configs = {}
        self.init(configs)

    def init(self, configs):
        import copy

        configs = copy.deepcopy(configs)
        if DEFAULT_PT_PLANNER not in configs:
            configs[DEFAULT_PT_PLANNER] = {
                'class': 'jormungandr.pt_planners.kraken.Kraken',
                'args': {'zmq_socket': self.default_socket_path},
            }

        for k in configs:
            conf = configs[k]
            if not conf['args'].get('name'):
                conf['args']['name'] = self.name

            conf['args']['zmq_context'] = self.zmq_context
            conf['args']['zmq_socket_type'] = self.zmq_socket_type

        for k in configs:
            if self.old_configs.get(k) == configs[k]:
                continue

            # config has changed regarding the old one, we must close all
            # zmq sockets before opening new ones
            if k in self.pt_planners and self.pt_planners[k].is_zmq_socket():
                self.pt_planners[k].clean_up_zmq_sockets()

            self.pt_planners[k] = utils.create_object(configs[k])

    def update_from_db(self):
        if not can_connect_to_database():
            self.logger.debug('Database is not accessible')
            self.last_update = datetime.datetime.utcnow()
            return

        if not self.db_configs_getter:
            return

        if self.last_update + datetime.timedelta(seconds=self.update_interval) > datetime.datetime.utcnow():
            return

        configs = self.db_configs_getter()
        self.init(configs)
        self.last_update = datetime.datetime.utcnow()

    def get_all_pt_planners(self):
        return self.pt_planners.items()

    def get_pt_planner(self, pt_planner_id):
        self.update_from_db()

        pt_planner = self.pt_planners.get(pt_planner_id)
        if pt_planner:
            return pt_planner
        raise NoRequestedPtPlanner("no requested pt_planner: {}".format(pt_planner_id))
