#!/usr/bin/env python
# coding=utf-8
from apptest import __all__ as alltests
import jormungandr.main
from jormungandr.instance_manager import InstanceManager
from instance_save import InstanceSave
import importlib
import logging


if __name__ == "__main__":
    tester = jormungandr.app.app.test_client()
    InstanceManager().initialisation(start_ping=False)
    InstanceManager().stop()
    for name, instance in InstanceManager().instances.iteritems():
        InstanceManager().instances[name] = InstanceSave("fixtures", instance)
    tests_classes = importlib.import_module("apptest")
    for name_t in alltests:
        logging.info("Reading test : " + name_t)
        test = getattr(tests_classes, name_t)
        if hasattr(test, "urls"):
            for url in test.urls.itervalues():
                logging.info("Requesting : " + url)
                tester.get(url)
                logging.info("file saved")
