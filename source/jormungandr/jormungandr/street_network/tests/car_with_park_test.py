# coding=utf-8
#  Copyright (c) 2001-2017, Canal TP and/or its affiliates. All rights reserved.
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
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from jormungandr.street_network.car_with_park import CarWithPark
from jormungandr.street_network.kraken import Kraken
from jormungandr.street_network.here import Here


def test_feed_publisher():
    walking_service = Kraken(
        instance=None, service_url='https://bob.com', id=u"walking_service_id", modes=["walking"]
    )
    car_service = Here(
        instance=None, service_base_url='https://bobobo.com', id=u"car_service_id", modes=["car"], timeout=1
    )
    car_with_park = CarWithPark(instance=None, walking_service=walking_service, car_service=car_service)

    fp_car_service = car_service.feed_publisher()
    fp_car_with_park = car_with_park.feed_publisher()

    assert fp_car_service == fp_car_with_park
    assert fp_car_with_park.id == "here"
    assert fp_car_with_park.license == "Private"
    assert fp_car_with_park.name == "here"
    assert fp_car_with_park.url == "https://developer.here.com/terms-and-conditions"
