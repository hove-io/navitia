# coding=utf-8
#  Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.hove.com).
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

from jormungandr.street_network.asgard import Asgard
from jormungandr import app
import pytest


@pytest.fixture
def config_asgard_socket():
    app.config['ASGARD_ZMQ_SOCKET'] = "tcp://127.0.0.1:3000"
    yield
    del app.config["ASGARD_ZMQ_SOCKET"]


class FakeInstance(object):

    name = 'fake_instance'
    context = None
    asgard_language = "en-US"


def status_test():
    asgard = Asgard(
        instance=FakeInstance(),
        service_url=None,
        asgard_socket="asgard_socket",
        id=u"tata-é$~#@\"*!'`§èû",
        modes=["walking", "bike", "car"],
        timeout=77,
        socket_ttl=60,
    )
    assert asgard._zmq_socket == "asgard_socket"

    status = asgard.status()
    assert len(status) == 7
    assert status['id'] == u'tata-é$~#@"*!\'`§èû'
    assert status['class'] == "Asgard"
    assert status['modes'] == ["walking", "bike", "car"]
    assert status['timeout'] == 77
    assert status['circuit_breaker']['current_state'] == 'closed'
    assert status['circuit_breaker']['fail_counter'] == 0
    assert status['circuit_breaker']['reset_timeout'] == 60
    assert status['zmq_socket_ttl'] == 60
    assert status['language'] == 'en-US'


def asgard_socket_var_test(config_asgard_socket):

    asgard = Asgard(
        instance=FakeInstance(),
        service_url=None,
        asgard_socket="asgard_socket",
        modes=["walking", "bike", "car"],
        timeout=77,
        socket_ttl=60,
    )
    assert asgard._zmq_socket == "tcp://127.0.0.1:3000"
    assert asgard.sn_system_id == "asgard"
