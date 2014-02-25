#!/usr/bin/env python
# coding=utf-8
from utils import *
#from app_test import __all__ as alltests
from jormungandr import i_manager, app
from instance_save import InstanceSave
import importlib
import logging


if __name__ == "__main__":
    tester = app.test_client()
    i_manager.initialisation(start_ping=False)
    i_manager.stop()
    tests_classes = importlib.import_module("app_test")

    nb_test = 0
    for name_t in tests_classes.__all__:
        logging.info("Reading test : " + name_t)
        test = getattr(tests_classes, name_t)

        if hasattr(test, "urls"):
            test_instance = test()

            # we need to override the kraken's instances
            # (after the creation of test, since he also override the krakens)
            for name, instance in i_manager.instances.iteritems():
                i_manager.instances[name] = InstanceSave(saving_directory, instance)

            for url_key in test.urls:
                try:
                    logging.info("serializing test " + url_key)
                    url = test_instance.get_url(url_key)
                    nb_test += 1
                    logging.info("Requesting : " + url)
                    response = tester.get(url)
                    # url can be in error (we might want to check invalid url)
                    # but we log if it's the case
                    if response.status_code != 200:
                        logging.warn("url in error : " + response.status_code)
                    logging.info("file saved")
                except AssertionError as error:
                    logging.warn("url in error : assertion failed : " + error.message)

    logging.info("%d tests serialized " % nb_test)