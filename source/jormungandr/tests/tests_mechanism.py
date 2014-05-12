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

import os
import subprocess
from check_utils import *

# set default config file if not defined in other tests
if not 'JORMUNGANDR_CONFIG_FILE' in os.environ:
    os.environ['JORMUNGANDR_CONFIG_FILE'] = os.path.dirname(os.path.realpath(__file__)) \
        + '/integration_tests_settings.py'
from jormungandr import app
from jormungandr.instance_manager import InstanceManager
from jormungandr.stat_manager import StatManager

krakens_dir = os.environ['KRAKEN_BUILD_DIR'] + '/tests'


def check_loaded(kraken):
    #TODO!
    #ping for 5s the kraken until data loaded
    return True


class AbstractTestFixture:
    """
    Mother class for all integration tests

    load the list of kraken defined with the @dataset decorator

    at the end of the tests, kill all kraken

    (the setup() and teardown() methods are called once at the initialization and destruction of the integration tests)
    """
    @classmethod
    def launch_all_krakens(cls):
        krakens_exe = cls.data_sets
        for kraken_name in krakens_exe:
            exe = os.path.join(krakens_dir, kraken_name)
            logging.debug("spawning " + exe)
            fdr, fdw = os.pipe()
            kraken = subprocess.Popen(exe, stderr=fdw, stdout=fdw, close_fds=True)

            cls.krakens_pool[kraken_name] = kraken

        logging.debug("{} kraken spawned".format(len(cls.krakens_pool)))

        # we want to wait for all data to be loaded
        all_good = True
        for name, kraken_process in cls.krakens_pool.iteritems():
            if not check_loaded(name):
                all_good = False
                logging.error("error while loading the kraken {}, stoping".format(name))
                break

        # a bit of cleaning
        if not all_good:
            cls.kill_all_krakens()

            cls.krakens_pool.clear()
            assert False, "problem while loading the kraken, no need to continue"

    @classmethod
    def kill_all_krakens(cls):
        for name, kraken_process in cls.krakens_pool.iteritems():
            logging.debug("killing " + name)
            kraken_process.kill()

    @classmethod
    def create_dummy_ini(cls):
        conf_template = open(os.path.join(os.path.dirname(os.path.realpath(__file__)),
                                          'test_jormungandr.ini'))
        conf_template_str = conf_template.read()
        for name in cls.krakens_pool:
            f = open(os.path.join(krakens_dir, name) + '.ini', 'w')
            logging.debug("writing ini file {} for {}".format(f.name, name))
            f.write(conf_template_str.format(instance_name=name))
            f.close()

        #we set the env var that will be used to init jormun
        app.config['INI_FILES'] = [
            os.path.join(os.environ['KRAKEN_BUILD_DIR'], 'tests', k + '.ini')
            for k in cls.krakens_pool
        ]

    @classmethod
    def setup_class(cls):
        cls.krakens_pool = {}
        logging.info("Initing the tests {}, let's pop the krakens"
                     .format(cls.__name__))
        cls.launch_all_krakens()
        cls.create_dummy_ini()

        i_manager = InstanceManager()
        i_manager.initialisation(start_ping=False)

        #we block the stat manager not to send anything to rabbit mq
        def mock_publish(self, stat):
            pass
        StatManager.publish_request = mock_publish

    @classmethod
    def teardown_class(cls):
        logging.info("Tearing down the tests {}, time to hunt the krakens down"
                     .format(cls.__name__))
        cls.kill_all_krakens()

    def __init__(self, *args, **kwargs):
        self.tester = app.test_client()

    # ==============================
    # helpers
    # ==============================
    def query(self, url, display=False):
        """
        Query the requested url, test url status code to 200
        and if valid format response as dict

        display param dumps the json (used only for debug)
        """
        response = check_url(self.tester, url)

        assert response.data
        json_response = json.loads(response.data)

        if display:
            logging.info("loaded response : " + json.dumps(json_response, indent=2))

        assert json_response
        return json_response

    def query_region(self, url, display=False):
        """
            helper if the test in only one region,
            to shorten the test url query /v1/coverage/{region}/url
        """
        assert len(self.krakens_pool) == 1, "the helper can only work with one region"

        real_url = "/v1/coverage/{region}/{url}".format(region=self.krakens_pool.iterkeys().next(), url=url)

        return self.query(real_url, display)

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


def dataset(datasets):
    """
    decorator giving class attribute 'data_sets'

    each test should have this decorator to make clear the data set used for the tests
    """
    def deco(cls):
        cls.data_sets = datasets
        return cls
    return deco
