"""
the setup() and teardown() methods are called once at the initialization of the integration tests
"""
import os
import stat
import subprocess
import logging


def is_exe(f):
    logging.info("check " + f)
    executable = stat.S_IEXEC | stat.S_IXGRP | stat.S_IXOTH
    if not os.path.isfile(f):
        return False

    st = os.stat(f)
    mode = st.st_mode
    return mode & executable


krakens_dir = '/home/antoine/dev/kraken/debug/tests'

krakens_pool = {}

def check_loaded(kraken):
    #TODO!
    #ping for 5s the kraken until data loaded
    return True

def launch_all_krakens():
    krakens_exe = [os.path.join(krakens_dir, f) for f in os.listdir(krakens_dir) if is_exe(os.path.join(krakens_dir, f))]
    for k in krakens_exe:
        logging.info("spawning " + k)
        fdr, fdw = os.pipe()
        kraken = subprocess.Popen(k, stderr=fdw, stdout=fdw, close_fds=True)

        krakens_pool[k] = kraken

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
        kill_all_krakens()

        krakens_pool.clear()
        assert False


def kill_all_krakens():
    for name, kraken_process in krakens_pool.iteritems():
        logging.info("killing " + name)
        kraken_process.kill()


class TestBidon:

    @classmethod
    def setup(cls):
        logging.info("hop on init!")
        launch_all_krakens()


    @classmethod
    def teardown(cls):
        logging.info("hop on kill!")
        kill_all_krakens()

    def test_bidon(self):
        logging.info("===========================================heho !")