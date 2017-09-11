# Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Canal TP (www.canaltp.fr).
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
# IRC #navitia on freenode
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
from retrying import RetryError
import six

if not 'JORMUNGANDR_CONFIG_FILE' in os.environ:
    os.environ['JORMUNGANDR_CONFIG_FILE'] = os.path.dirname(os.path.realpath(__file__)) \
        + '/integration_tests_settings.py'

import subprocess
from .check_utils import *
import jormungandr
from jormungandr import app, i_manager, utils
from jormungandr.stat_manager import StatManager
from navitiacommon.models import User
from jormungandr.instance import Instance

krakens_dir = os.environ['KRAKEN_BUILD_DIR'] + '/tests'


class FakeModel(object):
    def __init__(self, priority, is_free, is_open_data, scenario='default'):
        self.priority = priority
        self.is_free = is_free
        self.is_open_data = is_open_data
        self.scenario = scenario


class AbstractTestFixture(unittest.TestCase):
    """
    Mother class for all integration tests

    load the list of kraken defined with the @dataset decorator

    at the end of the tests, kill all kraken

    (the setup() and teardown() methods are called once at the initialization and destruction of the integration tests)
    """
    @classmethod
    def launch_all_krakens(cls):
        for (kraken_name, conf) in cls.data_sets.items():
            additional_args = conf.get('kraken_args', [])
            exe = os.path.join(krakens_dir, kraken_name)
            logging.debug("spawning " + exe)

            assert os.path.exists(exe), "cannot find the kraken {}".format(exe)

            args = [exe] + additional_args

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
            kraken_process.kill()

    @classmethod
    def create_dummy_json(cls):
        for name in cls.krakens_pool:
            instance_config = {
                "key": name,
                "zmq_socket": "ipc:///tmp/{instance_name}".format(instance_name=name)
            }
            instance_config.update(cls.data_sets[name].get('instance_config', {}))
            with open(os.path.join(krakens_dir, name) + '.json', 'w') as f:
                logging.debug("writing ini file {} for {}".format(f.name, name))
                f.write(json.dumps(instance_config, indent=4))

        #we set the env var that will be used to init jormun
        return [
            os.path.join(os.environ['KRAKEN_BUILD_DIR'], 'tests', k + '.json')
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
            jormungandr.global_autocomplete.update({
                'bragi': utils.create_object({
                    'class': 'jormungandr.autocomplete.geocodejson.GeocodeJson',
                    'args': {
                        "host": "https://host_of_bragi"
                    }
                }),
                'kraken': utils.create_object({'class': 'jormungandr.autocomplete.kraken.Kraken'})
            })

    @classmethod
    def global_jormun_teardown(cls):
        """
        cleanup the global config
        """
        if hasattr(cls, 'old_global_autocompletes'):
            logging.info("putting back the old global autoconfig for {}".format(cls.__name__))
            #if we changed the global_autocomplete variable, we put the old value back
            # we want to keep the same address for global_autocomplete as others might have references on it
            jormungandr.global_autocomplete.clear()
            jormungandr.global_autocomplete.update(cls.old_global_autocompletes)

    @classmethod
    def setup_class(cls):
        cls.tester = app.test_client()
        cls.krakens_pool = {}
        logging.info("Initing the tests {}, let's pop the krakens".format(cls.__name__))
        cls.global_jormun_setup()
        cls.launch_all_krakens()
        instances_config_files = cls.create_dummy_json()
        i_manager.configuration_files = instances_config_files
        i_manager.initialisation()
        cls.mocks = []
        for name in cls.krakens_pool:
            priority = cls.data_sets[name].get('priority', 0)
            logging.info('instance %s has priority %s', name, priority)
            is_free = cls.data_sets[name].get('is_free', False)
            is_open_data = cls.data_sets[name].get('is_open_data', False)
            scenario = cls.data_sets[name].get('scenario', 'default')
            cls.mocks.append(mock.patch.object(i_manager.instances[name],
                                               'get_models',
                                               return_value=FakeModel(priority, is_free, is_open_data, scenario)))

        for m in cls.mocks:
            m.start()

        # we check that all instances are up
        for name in cls.krakens_pool:
            instance = i_manager.instances[name]
            try:
                retrying.Retrying(stop_max_delay=5000,
                                  wait_fixed=10,
                                  retry_on_result=lambda x: not instance.is_initialized).call(instance.init)
            except RetryError:
                logging.exception('impossible to start kraken {}'.format(name))
                assert False, 'impossible to start a kraken'

        #we block the stat manager not to send anything to rabbit mq
        def mock_publish(self, stat, pbf):
            pass

        #we don't want to initialize rabbit for test.
        def mock_init():
            pass

        StatManager.publish_request = mock_publish
        StatManager._init_rabbitmq = mock_init

        #we don't want to have anything to do with the jormun database either
        class bob:
            @classmethod
            def mock_get_token(cls, token, valid_until):
                #note, since get_from_token is a class method, we need to wrap it.
                #change that with a real mock framework
                pass
        User.get_from_token = bob.mock_get_token

        @property
        def mock_journey_order(self):
            return 'arrival_time'
        Instance.journey_order = mock_journey_order

    @classmethod
    def teardown_class(cls):
        logging.info("Tearing down the tests {}, time to hunt the krakens down"
                     .format(cls.__name__))
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

    def query_region(self, url, check=True, display=False, version="v1"):
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
        for l in ["prev", "next", "first", "last"]:
            if l in ["prev", "next"] and not has_pt:# no prev and next if no pt
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
                if k == 'datetime_represents':
                    if l in ['prev', 'last']:
                        assert v == 'arrival'
                    else:
                        assert v == 'departure'
                    continue

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
            has_stop_point = any((o for s in j.get('sections', []) if 'from' in s
                                    for o in (s['from'], s['to']) if 'stop_point' in o))
            if 'sections' not in j:
                assert len(j['links']) == 1 # isochone link
                assert j['links'][0]['rel'] == 'journeys'
                assert '/journeys?' in j['links'][0]['href']
                assert 'from=' in j['links'][0]['href']
                assert 'to=' in j['links'][0]['href']
            elif has_stop_point and 'public_transport' in (s['type'] for s in j['sections']):
                assert len(j['links']) == 1 # same_journey_schedules link
                assert j['links'][0]['rel'] == 'same_journey_schedules'
                assert j['links'][0]['type'] == 'journeys'
                assert '/journeys?' in j['links'][0]['href']
                assert 'allowed_id%5B%5D=' in j['links'][0]['href']
                assert j['links'][0]['href'].count('first_section_mode%5B%5D=') == 1
                assert j['links'][0]['href'].count('last_section_mode%5B%5D=') == 1
                assert j['links'][0]['href'].count('direct_path=none') == 1
            else:
                assert 'links' not in j

        feed_publishers = response.get("feed_publishers", [])
        for feed_publisher in feed_publishers:
            is_valid_feed_publisher(feed_publisher)

        if query_dict.get('debug', False):
            assert 'debug' in response
        else:
            assert 'debug' not in response

    @staticmethod
    def check_next_datetime_link(dt, response, clockwise):
        if not response.get('journeys'):
            return
        """default next behaviour is 1 min after the best or the soonest"""
        j_to_compare = next((j for j in response.get('journeys', []) if j['type'] == 'best'), None) or\
             next((j for j in response.get('journeys', [])), None)

        j_departure = get_valid_datetime(j_to_compare['departure_date_time'])
        assert j_departure + timedelta(minutes=1) == dt

    @staticmethod
    def check_previous_datetime_link(dt, response, clockwise):
        if not response.get('journeys'):
            return
        """default previous behaviour is 1 min before the best or the latest """
        j_to_compare = next((j for j in response.get('journeys', []) if j['type'] == 'best'), None) or\
             next((j for j in response.get('journeys', [])), None)

        j_departure = get_valid_datetime(j_to_compare['arrival_date_time'])
        assert j_departure - timedelta(minutes=1) == dt


class NewDefaultScenarioAbstractTestFixture(AbstractTestFixture):
    @staticmethod
    def check_next_datetime_link(dt, response, clockwise):
        if not response.get('journeys'):
            return
        """default next behaviour is 1s after the best or the soonest"""
        from jormungandr.scenarios.qualifier import min_from_criteria
        j_to_compare = min_from_criteria(generate_pt_journeys(response),
                                         new_default_pagination_journey_comparator(clockwise=clockwise))

        j_departure = get_valid_datetime(j_to_compare['departure_date_time'])
        assert j_departure + timedelta(seconds=1) == dt

    @staticmethod
    def check_previous_datetime_link(dt, response, clockwise):
        if not response.get('journeys'):
            return
        """default previous behaviour is 1s before the best or the latest """
        from jormungandr.scenarios.qualifier import min_from_criteria
        j_to_compare = min_from_criteria(generate_pt_journeys(response),
                                         new_default_pagination_journey_comparator(clockwise=clockwise))

        j_departure = get_valid_datetime(j_to_compare['arrival_date_time'])
        assert j_departure - timedelta(seconds=1) == dt


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
