# Copyright (c) 2001-2014, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.kisio.com).
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

from __future__ import absolute_import, print_function, unicode_literals, division

import unittest
from copy import deepcopy
import os

# set default config file if not defined in other tests
from datetime import timedelta
import mock
import retrying

if not 'JORMUNGANDR_CONFIG_FILE' in os.environ:
    os.environ[str('JORMUNGANDR_CONFIG_FILE')] = str(
        os.path.dirname(os.path.realpath(__file__)) + '/integration_tests_settings.py'
    )

import subprocess
from .check_utils import *
import jormungandr
from jormungandr import app, i_manager, utils
from navitiacommon.models import User
from navitiacommon.type_pb2 import Severity
from jormungandr.instance import Instance
from jormungandr.parking_space_availability import (
    AbstractParkingPlacesProvider,
    Stands,
    StandsStatus,
    ParkingPlaces,
)
from jormungandr.equipments.sytral import SytralProvider
from jormungandr.ptref import FeedPublisher

import uuid


krakens_dir = os.environ[str('KRAKEN_BUILD_DIR')] + '/tests/mock-kraken'


class FakeModel(object):
    def __init__(
        self,
        priority,
        is_free,
        is_open_data,
        max_nb_journeys,
        scenario='new_default',
        equipment_details_providers=[],
        poi_dataset=None,
    ):
        self.priority = priority
        self.is_free = is_free
        self.is_open_data = is_open_data
        self.scenario = scenario
        self.equipment_details_providers = equipment_details_providers
        self.poi_dataset = poi_dataset
        self.max_nb_journeys = max_nb_journeys


class AbstractTestFixture(unittest.TestCase):
    """
    Mother class for all integration tests

    load the list of kraken defined with the @dataset decorator

    at the end of the tests, kill all kraken

    (the setup() and teardown() methods are called once at the initialization and destruction of the integration tests)
    """

    @classmethod
    def _get_zmq_socket_name(cls, kraken_name):
        return "ipc:///tmp/{uid}_{name}".format(uid=cls.uid, name=kraken_name)

    @classmethod
    def launch_all_krakens(cls):
        # Kraken doesn't handle amqp url :(
        rabbitmq = os.getenv('KRAKEN_RABBITMQ_HOST', 'localhost')
        for (kraken_name, conf) in cls.data_sets.items():
            exe = os.path.join(krakens_dir, kraken_name)
            assert os.path.exists(exe), "cannot find the kraken {}".format(exe)

            kraken_main_args = [
                "--GENERAL.zmq_socket=" + cls._get_zmq_socket_name(kraken_name),
                "--BROKER.host=" + rabbitmq,
                "--BROKER.queue=kraken_" + str(cls.uid),
                "--BROKER.queue_auto_delete=true",
            ]
            kraken_additional_args = conf.get('kraken_args', [])
            args = [exe] + kraken_main_args + kraken_additional_args

            logging.debug("spawning :" + " ".join(map(str, args)))

            kraken = subprocess.Popen(args, stderr=None, stdout=None, close_fds=False)
            cls.krakens_pool[kraken_name] = kraken

        logging.debug("{} kraken spawned".format(len(cls.krakens_pool)))

    @classmethod
    def kill_all_krakens(cls):
        for name, kraken_process in cls.krakens_pool.items():
            logging.debug("killing " + name)
            if cls.data_sets[name].get('check_killed', True):
                kraken_process.poll()
                if kraken_process.returncode is not None:
                    logging.error('kraken is dead, check errors, return code = %s', kraken_process.returncode)
                    assert False, 'kraken is dead, check errors, return code'
            kraken_process.terminate()
            kraken_process.communicate()  # read stdout and stderr to prevent zombie processes

    @classmethod
    def create_dummy_json(cls):
        for name in cls.krakens_pool:
            instance_config = {"key": name, "zmq_socket": cls._get_zmq_socket_name(name)}
            instance_config.update(cls.data_sets[name].get('instance_config', {}))
            with open(os.path.join(krakens_dir, name) + '.json', 'w') as f:
                logging.debug("writing ini file {} for {}".format(f.name, name))
                f.write(json.dumps(instance_config, indent=4))

        # we set the env var that will be used to init jormun
        return [
            os.path.join(os.environ['KRAKEN_BUILD_DIR'], 'tests/mock-kraken', k + '.json')
            for k in cls.krakens_pool
        ]

    @classmethod
    def global_jormun_setup(cls):
        """
        non instance dependent jormungandr setup
        """
        if cls.global_config.get('activate_bragi', False):
            logging.info("rigging the autocomplete for {}".format(cls.__name__))
            # if we need a global bragi, we rig jormungandr global_autocomplete
            cls.old_global_autocompletes = deepcopy(jormungandr.global_autocomplete)

            # we want to keep the same address for global_autocomplete as others might have references on it
            jormungandr.global_autocomplete.clear()
            jormungandr.global_autocomplete.update(
                {
                    'bragi': utils.create_object(
                        {
                            'class': 'jormungandr.autocomplete.geocodejson.GeocodeJson',
                            'args': {"host": "https://host_of_bragi"},
                        }
                    ),
                    'kraken': utils.create_object({'class': 'jormungandr.autocomplete.kraken.Kraken'}),
                }
            )

    @classmethod
    def global_jormun_teardown(cls):
        """
        cleanup the global config
        """
        if hasattr(cls, 'old_global_autocompletes'):
            logging.info("putting back the old global autoconfig for {}".format(cls.__name__))
            # if we changed the global_autocomplete variable, we put the old value back
            # we want to keep the same address for global_autocomplete as others might have references on it
            jormungandr.global_autocomplete.clear()
            jormungandr.global_autocomplete.update(cls.old_global_autocompletes)

    @classmethod
    def equipment_provider_manager(cls, key):
        """
        retrieve corresponding equipment provider manager
        """
        return i_manager.instances[key].equipment_provider_manager

    @classmethod
    def external_service_provider_manager(cls, key):
        """
        retrieve corresponding free_floating provider manager
        """
        return i_manager.instances[key].external_service_provider_manager

    @classmethod
    def setup_class(cls):
        cls.tester = app.test_client()
        cls.krakens_pool = {}
        cls.uid = uuid.uuid1()
        logging.info("Initing the tests {}, let's pop the krakens".format(cls.__name__))
        cls.global_jormun_setup()
        cls.launch_all_krakens()
        instances_config_files = cls.create_dummy_json()
        i_manager.configuration_files = instances_config_files
        i_manager.initialization()
        cls.mocks = []
        for name in cls.krakens_pool:
            priority = cls.data_sets[name].get('priority', 0)
            logging.info('instance %s has priority %s', name, priority)
            is_free = cls.data_sets[name].get('is_free', False)
            is_open_data = cls.data_sets[name].get('is_open_data', False)
            scenario = cls.data_sets[name].get('scenario', 'new_default')
            poi_dataset = cls.data_sets[name].get('poi_dataset', None)
            max_nb_journeys = cls.data_sets[name].get('max_nb_journeys', None)
            cls.mocks.append(
                mock.patch.object(
                    i_manager.instances[name],
                    'get_models',
                    return_value=FakeModel(
                        priority=priority,
                        is_free=is_free,
                        is_open_data=is_open_data,
                        scenario=scenario,
                        poi_dataset=poi_dataset,
                        max_nb_journeys=max_nb_journeys,
                    ),
                )
            )

        for m in cls.mocks:
            m.start()

        # we check that all instances are up
        for name in cls.krakens_pool:
            instance = i_manager.instances[name]
            try:
                retrying.Retrying(
                    stop_max_attempt_number=5,
                    wait_fixed=10,
                    retry_on_result=lambda x: not instance.is_initialized,
                ).call(instance.init)
            except retrying.RetryError:
                logging.exception('impossible to start kraken {}'.format(name))
                assert False, 'impossible to start a kraken'

        # we don't want to have anything to do with the jormun database either
        class bob:
            @classmethod
            def mock_get_token(cls, token, valid_until):
                # note, since get_from_token is a class method, we need to wrap it.
                # change that with a real mock framework
                pass

        User.get_from_token = bob.mock_get_token

        @property
        def mock_journey_order(self):
            return 'arrival_time'

        Instance.journey_order = mock_journey_order

    @classmethod
    def teardown_class(cls):
        logging.info("Tearing down the tests {}, time to hunt the krakens down".format(cls.__name__))
        cls.global_jormun_teardown()
        cls.kill_all_krakens()
        for m in cls.mocks:
            m.stop()

    # ==============================
    # helpers
    # ==============================
    def query(self, url, display=False, **kwargs):
        """
        Query the requested url, test url status code to 200
        and if valid format response as dict

        display param dumps the json (used only for debug)
        """
        response = check_url(self.tester, url, **kwargs)

        assert response.data
        json_response = json.loads(response.data)

        if display:
            logging.info("loaded response : " + json.dumps(json_response, indent=2))

        assert json_response
        return json_response

    def query_region(self, url, check=True, display=False):
        """
        helper if the test in only one region,
        to shorten the test url query /v1/coverage/{region}/url
        """
        assert len(self.krakens_pool) == 1, "the helper can only work with one region"
        str_url = "/v1/coverage"
        str_url += "/{region}/{url}"
        real_url = str_url.format(region=list(self.krakens_pool)[0], url=url)

        if check:
            return self.query(real_url, display)
        return self.query_no_assert(real_url, display)

    def query_no_assert(self, url, display=False):
        """
        query url without checking the status code
        used to test invalid url
        return tuple with response as dict and status
        """
        response = self.tester.get(url)

        json_response = json.loads(response.data)
        if display:
            logging.info("loaded response : " + json.dumps(json_response, indent=2))

        return json_response, response.status_code

    def check_journeys_links(self, response, query_dict, query):
        journeys_links = get_links_dict(response)
        clockwise = query_dict.get('datetime_represents', 'departure') == "departure"
        has_pt = any(s['type'] == 'public_transport' for j in response['journeys'] for s in j['sections'])
        tested_link = ["prev", "next", "first", "last"]

        has_disruption_no_service = False
        if 'impacts' in response:
            has_disruption_no_service = next(
                (True for impact in response.impacts if impact.severity.effect == Severity.NO_SERVICE), False
            )
        if has_disruption_no_service:
            tested_link.append("bypass_disruptions")

        for l in tested_link:
            if l in ["prev", "next"] and not has_pt:  # no prev and next if no pt
                continue

            assert l in journeys_links

            url = journeys_links[l]['href']

            additional_args = query_from_str(url)
            for k, v in additional_args.items():
                if k == 'datetime':
                    if l == 'next':
                        self.check_next_datetime_link(get_valid_datetime(v), response, clockwise)
                    elif l == 'prev':
                        self.check_previous_datetime_link(get_valid_datetime(v), response, clockwise)
                    elif l == 'first':
                        assert v.endswith('T000000')
                    elif l == 'last':
                        assert v.endswith('T235959')
                    continue
                if k == 'data_freshness':
                    if l == 'bypass_disruptions':
                        assert query_dict[k] == u'base_schedule'
                        assert v.endswith('realtime')
                if k == 'datetime_represents':
                    if l in ['prev', 'last']:
                        assert v == 'arrival'
                    else:
                        assert v == 'departure'
                    continue
                # assert query_dict[k] == v, except for bypass_disruptions link
                # we change query with  query_dict['data_freshness']='realtime'
                if k != 'data_freshness':
                    assert query_dict[k] == v

        if has_pt and '/coverage/' in query:
            types = [l['type'] for l in response['links']]
            assert 'stop_point' in types
            assert 'stop_area' in types
            assert 'route' in types
            assert 'physical_mode' in types
            assert 'commercial_mode' in types
            assert 'vehicle_journey' in types
            assert 'line' in types
            assert 'network' in types

    def is_valid_journey_response(self, response, query_str, check_journey_links=True):
        """
        check that the journey's response is valid

        this method is inside AbstractTestFixture because it can be overloaded by not scenario test Fixture
        """
        if isinstance(query_str, six.string_types):
            query_dict = query_from_str(query_str)
        else:
            query_dict = query_str

        journeys = get_not_null(response, "journeys")

        all_sections = unique_dict('id')
        assert len(journeys) > 0, "we must at least have one journey"
        for j in journeys:
            is_valid_journey(j, self.tester, query_dict)

            for s in j['sections']:
                all_sections[s['id']] = s

        # check the fare section
        # the fares must be structurally valid and all link to sections must be ok
        all_tickets = unique_dict('id')
        fares = response['tickets']
        for f in fares:
            is_valid_ticket(f, self.tester)
            all_tickets[f['id']] = f

        check_internal_links(response, self.tester)

        # check other links
        if check_journey_links:
            check_links(response, self.tester)

        # more checks on links, we want the prev/next/first/last,
        # to have forwarded all params, (and the time must be right)
        self.check_journeys_links(response, query_dict, query_str)

        # journeys[n].links
        for j in journeys:
            has_stop_point = any(
                (
                    o
                    for s in j.get('sections', [])
                    if 'from' in s
                    for o in (s['from'], s['to'])
                    if 'stop_point' in o
                )
            )
            if 'sections' not in j:
                assert len(j['links']) == 1  # isochone link
                assert j['links'][0]['rel'] == 'journeys'
                assert '/journeys?' in j['links'][0]['href']
                assert 'from=' in j['links'][0]['href']
                assert 'to=' in j['links'][0]['href']
            elif has_stop_point and 'public_transport' in (s['type'] for s in j['sections']):
                assert len(j['links']) == 2  # same_journey_schedules and this_journey link
                assert j['links'][0]['rel'] == 'same_journey_schedules'
                assert j['links'][0]['type'] == 'journeys'
                assert '/journeys?' in j['links'][0]['href']
                assert 'allowed_id%5B%5D=' in j['links'][0]['href']
                assert j['links'][0]['href'].count('first_section_mode%5B%5D=') == 1
                assert j['links'][0]['href'].count('last_section_mode%5B%5D=') == 1
                assert j['links'][0]['href'].count('direct_path=none') == 1
                assert j['links'][1]['rel'] == 'this_journey'
                assert j['links'][1]['href'].count('count=1') == 1
                assert j['links'][1]['href'].count('min_nb_journeys=1') == 1
            else:
                assert 'links' not in j

        feed_publishers = response.get("feed_publishers", [])
        for feed_publisher in feed_publishers:
            is_valid_feed_publisher(feed_publisher)

        if query_dict.get('debug', False):
            assert 'debug' in response
        else:
            assert 'debug' not in response

    def check_context(self, response):
        assert 'context' in response
        context = response['context']
        assert 'current_datetime' in context
        assert 'timezone' in context

    @staticmethod
    def check_next_datetime_link(dt, response, clockwise):
        if not response.get('journeys'):
            return
        """default next behaviour is 1s after the best or the soonest"""
        from jormungandr.scenarios.qualifier import min_from_criteria

        j_to_compare = min_from_criteria(
            generate_pt_journeys(response), new_default_pagination_journey_comparator(clockwise=clockwise)
        )

        j_departure = get_valid_datetime(j_to_compare['departure_date_time'])
        assert j_departure + timedelta(seconds=1) == dt

    @staticmethod
    def check_previous_datetime_link(dt, response, clockwise):
        if not response.get('journeys'):
            return
        """default previous behaviour is 1s before the best or the latest """
        from jormungandr.scenarios.qualifier import min_from_criteria

        j_to_compare = min_from_criteria(
            generate_pt_journeys(response), new_default_pagination_journey_comparator(clockwise=clockwise)
        )

        j_departure = get_valid_datetime(j_to_compare['arrival_date_time'])
        assert j_departure - timedelta(seconds=1) == dt


class NewDefaultScenarioAbstractTestFixture(AbstractTestFixture):
    pass


def dataset(datasets, global_config={}):
    """
    decorator giving class attribute 'data_sets'

    each test should have this decorator to make clear the data set used for the tests

    each dataset can be either:
    just a string with the kraken name,
    or a pair with the kraken name and a list with additional arguments for the kraken
    """

    def deco(cls):
        cls.data_sets = datasets
        cls.global_config = global_config
        return cls

    return deco


def config(configs=None):
    import copy

    if not configs:
        configs = {"scenario": "default"}

    def deco(cls):
        cls.data_sets = {}
        for c in cls.__bases__:
            if hasattr(c, "data_sets"):
                for key in c.data_sets:
                    orig_config = copy.deepcopy(c.data_sets[key])
                    orig_config.update(configs)
                    cls.data_sets.update({key: orig_config})
        return cls

    return deco


class MockBssProvider(AbstractParkingPlacesProvider):
    def __init__(self, pois_supported, name='mock bss provider'):
        self.pois_supported = pois_supported
        self.name = name

    def support_poi(self, poi):
        return not self.pois_supported or poi['id'] in self.pois_supported

    def get_informations(self, poi):
        available_places = 13 if poi['id'] == 'poi:station_1' else 99
        available_bikes = 3 if poi['id'] == 'poi:station_1' else 98
        return Stands(
            available_places=available_places, available_bikes=available_bikes, status=StandsStatus.open
        )

    def status(self):
        return {}

    def feed_publisher(self):
        return FeedPublisher(id='mock_bss', name=self.name, license='the death license', url='bob.com')


def mock_bss_providers(pois_supported):
    providers = [MockBssProvider(pois_supported=pois_supported)]
    return mock.patch(
        'jormungandr.parking_space_availability.bss.BssProviderManager._get_providers',
        new_callable=mock.PropertyMock,
        return_value=lambda: providers,
    )


class MockCarParkProvider(AbstractParkingPlacesProvider):
    def __init__(self, pois_supported, name='mock car park provider'):
        self.pois_supported = pois_supported
        self.name = name

    def support_poi(self, poi):
        return not self.pois_supported or poi['id'] in self.pois_supported

    def get_informations(self, poi):
        return ParkingPlaces(available=41, occupied=42, available_PRM=43, occupied_PRM=44)

    def status(self):
        return {}

    def feed_publisher(self):
        return FeedPublisher(id='mock_car_park', name=self.name, license='license to kill', url='MI.6')


def mock_car_park_providers(pois_supported):
    providers = [MockCarParkProvider(pois_supported=pois_supported)]
    return mock.patch(
        'jormungandr.parking_space_availability.car.CarParkingProviderManager._get_providers',
        new_callable=mock.PropertyMock,
        return_value=lambda: providers,
    )


def mock_equipment_providers(equipment_provider_manager, data, code_types_list):
    equipment_provider_manager._equipment_providers = {
        "sytral": SytralProvider(url="fake.url", timeout=3, code_types=code_types_list)
    }
    equipment_provider_manager._equipment_providers["sytral"]._call_webservice = mock.MagicMock(
        return_value=data
    )
