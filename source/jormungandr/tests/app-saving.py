#!/usr/bin/env python
# coding=utf-8
import os
#early definition of the env var
os.environ['JORMUNGANDR_CONFIG_FILE'] = os.path.dirname(os.path.realpath(__file__)) \
    + '/app_saving_settings.py'
from utils import *
from jormungandr import i_manager, app
from instance_save import InstanceSave
import importlib
import logging


class Serializer(MockInstance):

    def __init__(self, *args, **kwargs):
        i_manager.initialisation(start_ping=False)
        i_manager.stop()
        self.tester = app.test_client()

        # we need to override the kraken's instances
        logging.info("number of instances : " + str(len(i_manager.instances)))
        for name, instance in i_manager.instances.iteritems():
            i_manager.instances[name] = InstanceSave(instance)

    def serialize(self):
        logging.info("cleaning files")
        clean_dir(saving_directory)

        tests_classes = importlib.import_module("app_test")

        nb_test = 0
        for name_t in tests_classes.__all__:
            logging.info("Reading test : " + name_t)
            test = getattr(tests_classes, name_t)

            if hasattr(test, "urls"):

                for url_key, raw_url in test.urls.iteritems():
                    logging.info("=============== serializing test " +
                                 url_key + " ===============")
                    try:
                        url = self.transform_url(raw_url)
                        nb_test += 1
                        logging.info("Requesting : " + url)
                        response = self.tester.get(url)
                        # url can be in error (we might want to check invalid url)
                        # but we log if it's the case
                        if response.status_code != 200:
                            logging.warn("url in error : " + str(response.status_code))
                    except AssertionError as error:
                        logging.warn("url in error : assertion failed : " + error.message)

        logging.info("{} tests serialized ".format(nb_test))


def clean_dir(dir):
    for files in os.listdir(dir):
        file_path = os.path.join(dir, files)
        try:
            if os.path.isfile(file_path):
                os.unlink(file_path)
        except Exception as e:
            logging.error("error: " + e)

if __name__ == "__main__":
    ser = Serializer()
    ser.serialize()
