"""
the setup() and teardown() methods are called once at the initialization of the integration tests
"""
import os
import stat
import subprocess
import logging

krakens_dir = '/home/antoine/dev/kraken/debug/tests'
os.environ['JORMUNGANDR_CONFIG_FILE'] = os.path.dirname(os.path.realpath(__file__)) \
    + '/integration_tests_settings.py'
from jormungandr import i_manager, app
from nose.tools import *
from check_utils import *


def is_exe(f):
    logging.info("check " + f)
    executable = stat.S_IEXEC | stat.S_IXGRP | stat.S_IXOTH
    if not os.path.isfile(f):
        return False

    st = os.stat(f)
    mode = st.st_mode
    return mode & executable


krakens_pool = {}

def check_loaded(kraken):
    #TODO!
    #ping for 5s the kraken until data loaded
    return True




class TestBidon:

    @classmethod
    def launch_all_krakens(cls):
        krakens_exe = [f for f in os.listdir(krakens_dir) if is_exe(os.path.join(krakens_dir, f))]
        for kraken_name in krakens_exe:
            exe = os.path.join(krakens_dir, kraken_name)
            logging.info("spawning " + exe)
            fdr, fdw = os.pipe()
            kraken = subprocess.Popen(exe, stderr=fdw, stdout=fdw, close_fds=True)

            krakens_pool[kraken_name] = kraken

        logging.info("{} kraken spawned".format(len(krakens_pool)))

        # we want to wait for all data to be loaded
        all_good = True
        for name, kraken_process in krakens_pool.iteritems():
            if not check_loaded(name):
                all_good = False
                logging.error("error while loading the kraken {}, stoping".format(name))
                break

        # a bit of cleaning
        if not all_good:
            cls.kill_all_krakens()

            krakens_pool.clear()
            assert False

    @classmethod
    def kill_all_krakens(cls):
        for name, kraken_process in krakens_pool.iteritems():
            logging.info("killing " + name)
            kraken_process.kill()

    @classmethod
    def create_dummy_init(cls):
        for name in krakens_pool:
            f = open(os.path.join(krakens_dir, name) + '.ini', 'w')
            logging.info("writing ini file {} for {}".format(f.name, name))
            f.write('[instance]\n\
key={instance_name}\n\
socket=ipc:///tmp/{instance_name}\n\
\n\
[functional]\n\
cheap_journey=True\n'.format(instance_name=name))
            f.close()


    @classmethod
    def setup(cls):
        logging.info("hop on init!")
        cls.launch_all_krakens()
        cls.create_dummy_init()


    @classmethod
    def teardown(cls):
        logging.info("hop on kill!")
        cls.kill_all_krakens()

    def __init__(self, *args, **kwargs):
        self.tester = app.test_client()

    def test_bidon(self):
        logging.info("===========================================heho !")
        response = check_and_get_as_dict(self.tester, "/v1/coverage")

        logging.info(response)
        response = check_and_get_as_dict(self.tester, "/v1/coverage/main_routing_test")