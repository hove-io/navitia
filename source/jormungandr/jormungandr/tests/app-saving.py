#!/usr/bin/env python
# coding=utf-8
from apptest import __all__ as alltests
import jormungandr
from instance_manager import InstanceManager
from instance_save import InstanceSave
import importlib


if __name__ == "__main__":
    tester = jormungandr.app.test_client()
    InstanceManager().stop()
    for name, instance in InstanceManager().instances.iteritems():
        InstanceManager().instances[name] = InstanceSave("fixtures", instance)

    tests_classes = importlib.import_module("apptest")
    for name_t in alltests:
        test = getattr(tests_classes, name_t)
        if hasattr(test, "urls"):
            for url in test.urls.itervalues():
                tester.get(url)
