# Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
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
from __future__ import absolute_import, print_function, unicode_literals, division
from jormungandr.scenarios import journey_filter
import navitiacommon.response_pb2 as response_pb2
import navitiacommon.type_pb2 as type_pb2
from jormungandr.instance import OlympicsForbiddenUris
from jormungandr.street_network.tests.streetnetwork_test_utils import make_pt_object
from jormungandr.olympic_site_params_manager import AttractivityVirtualFallback


def remove_excess_terminus_without_item_test():
    response = response_pb2.Response()
    journey = response.journeys.add()
    journey.sections.add()
    journey.sections.add()
    journey.sections.add()

    journey.sections[0].type = response_pb2.STREET_NETWORK
    journey.sections[0].duration = 796

    journey_filter.remove_excess_terminus(response)
    assert len(response.terminus) == 0


def remove_excess_terminus_without_uri_in_display_information_test():
    response = response_pb2.Response()
    journey = response.journeys.add()
    journey.sections.add()
    journey.sections.add()
    journey.sections.add()

    journey.sections[0].type = response_pb2.STREET_NETWORK
    journey.sections[0].duration = 796

    terminus = response.terminus.add()
    terminus.uri = "aaa"
    assert len(response.terminus) == 1

    journey_filter.remove_excess_terminus(response)
    assert len(response.terminus) == 0


def remove_excess_terminus_with_uri_in_display_information_test():
    response = response_pb2.Response()
    journey = response.journeys.add()
    journey.sections.add()
    journey.sections.add()
    journey.sections.add()

    journey.sections[0].type = response_pb2.PUBLIC_TRANSPORT
    journey.sections[0].duration = 796
    journey.sections[0].pt_display_informations.uris.stop_area = "aaa"

    terminus = response.terminus.add()
    terminus.uri = "aaa"
    assert len(response.terminus) == 1

    journey_filter.remove_excess_terminus(response)
    assert len(response.terminus) == 1


def remove_excess_terminus_with_2items_uri_in_display_information_test():
    response = response_pb2.Response()
    journey = response.journeys.add()
    journey.sections.add()
    journey.sections.add()
    journey.sections.add()

    journey.sections[0].type = response_pb2.PUBLIC_TRANSPORT
    journey.sections[0].duration = 796
    journey.sections[0].pt_display_informations.uris.stop_area = "aaa"

    terminus = response.terminus.add()
    terminus.uri = "aaa"
    terminus = response.terminus.add()
    terminus.uri = "bbb"
    assert len(response.terminus) == 2

    journey_filter.remove_excess_terminus(response)
    assert len(response.terminus) == 1


def filter_odt_journeys_clockwise_test():
    request = {"clockwise": True}

    response = response_pb2.Response()
    # a public transport journey
    pt_journey = response.journeys.add()
    pt_journey.arrival_date_time = 10
    pt_section = pt_journey.sections.add()
    pt_section.type = response_pb2.PUBLIC_TRANSPORT

    # an On Demand Transport journey
    # that arrives after the public transport journey
    odt_journey = response.journeys.add()
    odt_section = odt_journey.sections.add()
    odt_section.additional_informations.append(response_pb2.ODT_WITH_ZONE)
    odt_journey.arrival_date_time = 11

    journey_filter.filter_odt_journeys(response.journeys, request)

    assert odt_journey.tags[0] == "to_delete"


def filter_odt_journeys_counter_clockwise_test():
    request = {"clockwise": False}

    response = response_pb2.Response()
    # a public transport journey
    pt_journey = response.journeys.add()
    pt_journey.departure_date_time = 10
    pt_section = pt_journey.sections.add()
    pt_section.type = response_pb2.PUBLIC_TRANSPORT

    # an On Demand Transport journey
    # that depart before the public transport journey
    odt_journey = response.journeys.add()
    odt_section = odt_journey.sections.add()
    odt_section.additional_informations.append(response_pb2.ODT_WITH_ZONE)
    odt_journey.departure_date_time = 9

    journey_filter.filter_odt_journeys(response.journeys, request)

    assert odt_journey.tags[0] == "to_delete"


class FakeInstance:
    def __init__(self, name='test'):
        self.name = name
        self.olympics_forbidden_uris = None


def filter_olympic_site_without_olympics_forbidden_uris_test():
    # STREET_NETWORK -> PUBLIC_TRANSPORT + TRANSFER + PUBLIC_TRANSPORT + STREET_NETWORK
    response = response_pb2.Response()
    journey = response.journeys.add()
    for index, stype in enumerate(
        [
            response_pb2.STREET_NETWORK,
            response_pb2.PUBLIC_TRANSPORT,
            response_pb2.TRANSFER,
            response_pb2.PUBLIC_TRANSPORT,
            response_pb2.STREET_NETWORK,
        ]
    ):
        section = journey.sections.add()
        section.type = stype
        section.duration = index
    instance = FakeInstance()
    origin = make_pt_object(type_pb2.POI, lon=1, lat=2, uri='origin:poi')
    destination = make_pt_object(type_pb2.STOP_AREA, lon=3, lat=4, uri='destination:poi')
    journey_filter.filter_olympic_site([response], instance, {}, origin, destination)
    assert len(response.journeys[0].tags) == 0


def filter_olympic_site_with_wheelchair_true_test():
    # STREET_NETWORK -> PUBLIC_TRANSPORT + TRANSFER + PUBLIC_TRANSPORT + STREET_NETWORK
    response = response_pb2.Response()
    journey = response.journeys.add()
    response = response_pb2.Response()
    journey = response.journeys.add()
    for index, stype in enumerate(
        [
            response_pb2.STREET_NETWORK,
            response_pb2.PUBLIC_TRANSPORT,
            response_pb2.TRANSFER,
            response_pb2.PUBLIC_TRANSPORT,
            response_pb2.STREET_NETWORK,
        ]
    ):
        section = journey.sections.add()
        section.type = stype
        section.duration = index
    instance = FakeInstance()
    instance.olympics_forbidden_uris = OlympicsForbiddenUris(
        pt_object_olympics_forbidden_uris=["network:abc"],
        poi_property_key="sitejo",
        poi_property_value="123",
        min_pt_duration=50,
    )
    origin = make_pt_object(type_pb2.POI, lon=1, lat=2, uri='origin:poi')
    destination = make_pt_object(type_pb2.STOP_AREA, lon=3, lat=4, uri='destination:stop_area')
    journey_filter.filter_olympic_site([response], instance, {"wheelchair": True}, origin, destination)
    assert len(response.journeys[0].tags) == 0


def filter_olympic_site_without_wheelchair_test():
    # STREET_NETWORK -> PUBLIC_TRANSPORT + TRANSFER + PUBLIC_TRANSPORT + STREET_NETWORK
    response = response_pb2.Response()
    journey = response.journeys.add()
    response = response_pb2.Response()
    journey = response.journeys.add()
    for index, stype in enumerate(
        [
            response_pb2.STREET_NETWORK,
            response_pb2.PUBLIC_TRANSPORT,
            response_pb2.TRANSFER,
            response_pb2.PUBLIC_TRANSPORT,
            response_pb2.STREET_NETWORK,
        ]
    ):
        section = journey.sections.add()
        section.type = stype
        section.duration = index
    instance = FakeInstance()
    instance.olympics_forbidden_uris = OlympicsForbiddenUris(
        pt_object_olympics_forbidden_uris=["network:abc"],
        poi_property_key="sitejo",
        poi_property_value="123",
        min_pt_duration=50,
    )
    origin = make_pt_object(type_pb2.POI, lon=1, lat=2, uri='origin:poi')
    destination = make_pt_object(type_pb2.STOP_AREA, lon=3, lat=4, uri='destination:stop_area')
    journey_filter.filter_olympic_site([response], instance, {}, origin, destination)
    assert len(response.journeys[0].tags) == 0


def filter_olympic_site_origin_and_destination_test():
    # STREET_NETWORK -> PUBLIC_TRANSPORT + TRANSFER + PUBLIC_TRANSPORT + STREET_NETWORK
    response = response_pb2.Response()
    journey = response.journeys.add()
    response = response_pb2.Response()
    journey = response.journeys.add()
    for index, stype in enumerate(
        [
            response_pb2.STREET_NETWORK,
            response_pb2.PUBLIC_TRANSPORT,
            response_pb2.TRANSFER,
            response_pb2.PUBLIC_TRANSPORT,
            response_pb2.STREET_NETWORK,
        ]
    ):
        section = journey.sections.add()
        section.type = stype
        section.duration = index
    instance = FakeInstance()
    instance.olympics_forbidden_uris = OlympicsForbiddenUris(
        pt_object_olympics_forbidden_uris=["network:abc"],
        poi_property_key="sitejo",
        poi_property_value="123",
        min_pt_duration=50,
    )
    origin = make_pt_object(type_pb2.POI, lon=1, lat=2, uri='origin:poi')
    property = origin.poi.properties.add()
    property.type = "sitejo"
    property.value = "123"

    destination = make_pt_object(type_pb2.POI, lon=3, lat=4, uri='destination:poi')
    property = destination.poi.properties.add()
    property.type = "sitejo"
    property.value = "123"

    journey_filter.filter_olympic_site([response], instance, {"wheelchair": False}, origin, destination)
    assert len(response.journeys[0].tags) == 0


def filter_olympic_site_origin_olympic_site_first_pt_section_metro_test():
    # STREET_NETWORK -> PUBLIC_TRANSPORT + TRANSFER + PUBLIC_TRANSPORT + STREET_NETWORK
    #   Walking     ->  physical_mode:Metro -> Transfer -> physical_mode:Tramway -> Walking
    response = response_pb2.Response()
    journey = response.journeys.add()
    for stype in (
        response_pb2.STREET_NETWORK,
        response_pb2.PUBLIC_TRANSPORT,
        response_pb2.TRANSFER,
        response_pb2.PUBLIC_TRANSPORT,
        response_pb2.STREET_NETWORK,
    ):
        section = journey.sections.add()
        section.type = stype
    journey.sections[1].uris.physical_mode = 'physical_mode:Metro'
    instance = FakeInstance()
    instance.olympics_forbidden_uris = OlympicsForbiddenUris(
        pt_object_olympics_forbidden_uris=["network:abc"],
        poi_property_key="sitejo",
        poi_property_value="123",
        min_pt_duration=50,
    )
    origin = make_pt_object(type_pb2.POI, lon=1, lat=2, uri='origin:poi')
    property = origin.poi.properties.add()
    property.type = "sitejo"
    property.value = "123"

    destination = make_pt_object(type_pb2.POI, lon=3, lat=4, uri='destination:poi')
    journey_filter.filter_olympic_site([response], instance, {"wheelchair": False}, origin, destination)
    assert len(response.journeys[0].tags) == 0


def filter_olympic_site_destination_olympic_site_last_pt_section_tramway_test():
    # STREET_NETWORK -> PUBLIC_TRANSPORT + TRANSFER + PUBLIC_TRANSPORT + STREET_NETWORK
    #   Walking     ->  physical_mode:Metro -> Transfer -> physical_mode:Tramway -> Walking
    response = response_pb2.Response()
    journey = response.journeys.add()
    for stype in (
        response_pb2.STREET_NETWORK,
        response_pb2.PUBLIC_TRANSPORT,
        response_pb2.TRANSFER,
        response_pb2.PUBLIC_TRANSPORT,
        response_pb2.STREET_NETWORK,
    ):
        section = journey.sections.add()
        section.type = stype
    journey.sections[3].uris.physical_mode = 'physical_mode:Tramway'
    instance = FakeInstance()
    instance.olympics_forbidden_uris = OlympicsForbiddenUris(
        pt_object_olympics_forbidden_uris=["network:abc"],
        poi_property_key="sitejo",
        poi_property_value="123",
        min_pt_duration=50,
    )
    origin = make_pt_object(type_pb2.POI, lon=1, lat=2, uri='origin:poi')
    destination = make_pt_object(type_pb2.POI, lon=3, lat=4, uri='destination:poi')
    property = destination.poi.properties.add()
    property.type = "sitejo"
    property.value = "123"
    journey_filter.filter_olympic_site([response], instance, {"wheelchair": False}, origin, destination)
    assert len(response.journeys[0].tags) == 0


def filter_olympic_site_origin_olympic_site_first_pt_section_navette_jo_test():
    # STREET_NETWORK -> PUBLIC_TRANSPORT + TRANSFER + PUBLIC_TRANSPORT + STREET_NETWORK
    #   Walking     ->  physical_mode:Bus -> Transfer -> physical_mode:Tramway -> Walking
    response = response_pb2.Response()
    journey = response.journeys.add()
    for stype in (
        response_pb2.STREET_NETWORK,
        response_pb2.PUBLIC_TRANSPORT,
        response_pb2.TRANSFER,
        response_pb2.PUBLIC_TRANSPORT,
        response_pb2.STREET_NETWORK,
    ):
        section = journey.sections.add()
        section.type = stype
    journey.sections[1].uris.physical_mode = 'physical_mode:Bus'
    journey.sections[1].uris.network = 'network:abc'
    instance = FakeInstance()
    instance.olympics_forbidden_uris = OlympicsForbiddenUris(
        pt_object_olympics_forbidden_uris=["network:abc"],
        poi_property_key="sitejo",
        poi_property_value="123",
        min_pt_duration=50,
    )
    origin = make_pt_object(type_pb2.POI, lon=1, lat=2, uri='origin:poi')
    property = origin.poi.properties.add()
    property.type = "sitejo"
    property.value = "123"

    destination = make_pt_object(type_pb2.POI, lon=3, lat=4, uri='destination:poi')
    journey_filter.filter_olympic_site([response], instance, {"wheelchair": False}, origin, destination)
    assert len(response.journeys[0].tags) == 0


def filter_olympic_site_destination_olympic_site_last_pt_section_navette_jo_test():
    # STREET_NETWORK -> PUBLIC_TRANSPORT + TRANSFER + PUBLIC_TRANSPORT + STREET_NETWORK
    #   Walking     ->  physical_mode:Metro -> Transfer -> physical_mode:Bus -> Walking
    response = response_pb2.Response()
    journey = response.journeys.add()
    for stype in (
        response_pb2.STREET_NETWORK,
        response_pb2.PUBLIC_TRANSPORT,
        response_pb2.TRANSFER,
        response_pb2.PUBLIC_TRANSPORT,
        response_pb2.STREET_NETWORK,
    ):
        section = journey.sections.add()
        section.type = stype
    journey.sections[3].uris.physical_mode = 'physical_mode:Bus'
    journey.sections[3].uris.network = 'network:abc'
    instance = FakeInstance()
    instance.olympics_forbidden_uris = OlympicsForbiddenUris(
        pt_object_olympics_forbidden_uris=["network:abc"],
        poi_property_key="sitejo",
        poi_property_value="123",
        min_pt_duration=50,
    )
    origin = make_pt_object(type_pb2.POI, lon=1, lat=2, uri='origin:poi')
    destination = make_pt_object(type_pb2.POI, lon=3, lat=4, uri='destination:poi')
    property = destination.poi.properties.add()
    property.type = "sitejo"
    property.value = "123"
    journey_filter.filter_olympic_site([response], instance, {"wheelchair": False}, origin, destination)
    assert len(response.journeys[0].tags) == 0


def filter_olympic_site_origin_olympic_site_to_delete_tag_test():
    # STREET_NETWORK -> PUBLIC_TRANSPORT + TRANSFER + PUBLIC_TRANSPORT + STREET_NETWORK
    #   Walking     ->  physical_mode:Bus -> Transfer -> physical_mode:Tramway -> Walking
    # journey.sections[1].duration < min_pt_duration
    response = response_pb2.Response()
    journey = response.journeys.add()
    for stype in (
        response_pb2.STREET_NETWORK,
        response_pb2.PUBLIC_TRANSPORT,
        response_pb2.TRANSFER,
        response_pb2.PUBLIC_TRANSPORT,
        response_pb2.STREET_NETWORK,
    ):
        section = journey.sections.add()
        section.type = stype
    journey.sections[1].uris.physical_mode = 'physical_mode:Bus'
    journey.sections[1].duration = 40
    journey.nb_transfers = 2
    instance = FakeInstance()
    instance.olympics_forbidden_uris = OlympicsForbiddenUris(
        pt_object_olympics_forbidden_uris=["network:abc"],
        poi_property_key="sitejo",
        poi_property_value="123",
        min_pt_duration=50,
    )
    origin = make_pt_object(type_pb2.POI, lon=1, lat=2, uri='origin:poi')
    property = origin.poi.properties.add()
    property.type = "sitejo"
    property.value = "123"
    destination = make_pt_object(type_pb2.POI, lon=3, lat=4, uri='destination:poi')

    journey_filter.filter_olympic_site([response], instance, {"wheelchair": False}, origin, destination)
    assert len(response.journeys[0].tags) == 1
    assert response.journeys[0].tags[0] == "to_delete"


def filter_olympic_site_origin_olympic_site_without_to_delete_tag_test():
    # STREET_NETWORK -> PUBLIC_TRANSPORT + TRANSFER + PUBLIC_TRANSPORT + STREET_NETWORK
    #   Walking     ->  physical_mode:Bus -> Transfer -> physical_mode:Tramway -> Walking
    # journey.sections[1].duration > min_pt_duration
    response = response_pb2.Response()
    journey = response.journeys.add()
    for stype in (
        response_pb2.STREET_NETWORK,
        response_pb2.PUBLIC_TRANSPORT,
        response_pb2.TRANSFER,
        response_pb2.PUBLIC_TRANSPORT,
        response_pb2.STREET_NETWORK,
    ):
        section = journey.sections.add()
        section.type = stype
    journey.sections[1].uris.physical_mode = 'physical_mode:Bus'
    journey.sections[1].duration = 60

    instance = FakeInstance()
    instance.olympics_forbidden_uris = OlympicsForbiddenUris(
        pt_object_olympics_forbidden_uris=["network:abc"],
        poi_property_key="sitejo",
        poi_property_value="123",
        min_pt_duration=50,
    )
    origin = make_pt_object(type_pb2.POI, lon=1, lat=2, uri='origin:poi')
    property = origin.poi.properties.add()
    property.type = "sitejo"
    property.value = "123"

    destination = make_pt_object(type_pb2.POI, lon=3, lat=4, uri='destination:poi')
    journey_filter.filter_olympic_site([response], instance, {"wheelchair": False}, origin, destination)
    assert len(response.journeys[0].tags) == 0


def filter_olympic_site_destination_olympic_site_to_delete_tag_test():
    # STREET_NETWORK -> PUBLIC_TRANSPORT + TRANSFER + PUBLIC_TRANSPORT + STREET_NETWORK
    #   Walking     ->  physical_mode:Metro -> Transfer -> physical_mode:Bus -> Walking
    # journey.sections[3].duration < min_pt_duration
    response = response_pb2.Response()
    journey = response.journeys.add()
    for stype in (
        response_pb2.STREET_NETWORK,
        response_pb2.PUBLIC_TRANSPORT,
        response_pb2.TRANSFER,
        response_pb2.PUBLIC_TRANSPORT,
        response_pb2.STREET_NETWORK,
    ):
        section = journey.sections.add()
        section.type = stype
    journey.sections[3].uris.physical_mode = 'physical_mode:Bus'
    journey.sections[3].duration = 40
    journey.nb_transfers = 2
    instance = FakeInstance()
    instance.olympics_forbidden_uris = OlympicsForbiddenUris(
        pt_object_olympics_forbidden_uris=["network:abc"],
        poi_property_key="sitejo",
        poi_property_value="123",
        min_pt_duration=50,
    )
    origin = make_pt_object(type_pb2.POI, lon=1, lat=2, uri='origin:poi')
    destination = make_pt_object(type_pb2.POI, lon=3, lat=4, uri='destination:poi')
    property = destination.poi.properties.add()
    property.type = "sitejo"
    property.value = "123"
    journey_filter.filter_olympic_site([response], instance, {"wheelchair": False}, origin, destination)
    assert len(response.journeys[0].tags) == 1
    assert response.journeys[0].tags[0] == "to_delete"


def filter_olympic_site_destination_olympic_site_without_to_delete_tag_test():
    # STREET_NETWORK -> PUBLIC_TRANSPORT + TRANSFER + PUBLIC_TRANSPORT + STREET_NETWORK
    #   Walking     ->  physical_mode:Metro -> Transfer -> physical_mode:Bus -> Walking
    # journey.sections[3].duration > min_pt_duration
    response = response_pb2.Response()
    journey = response.journeys.add()
    for stype in (
        response_pb2.STREET_NETWORK,
        response_pb2.PUBLIC_TRANSPORT,
        response_pb2.TRANSFER,
        response_pb2.PUBLIC_TRANSPORT,
        response_pb2.STREET_NETWORK,
    ):
        section = journey.sections.add()
        section.type = stype
    journey.sections[3].uris.physical_mode = 'physical_mode:Bus'
    journey.sections[3].duration = 60
    instance = FakeInstance()
    instance.olympics_forbidden_uris = OlympicsForbiddenUris(
        pt_object_olympics_forbidden_uris=["network:abc"],
        poi_property_key="sitejo",
        poi_property_value="123",
        min_pt_duration=50,
    )
    origin = make_pt_object(type_pb2.POI, lon=1, lat=2, uri='origin:poi')
    destination = make_pt_object(type_pb2.POI, lon=3, lat=4, uri='destination:poi')
    property = destination.poi.properties.add()
    property.type = "sitejo"
    property.value = "123"
    journey_filter.filter_olympic_site([response], instance, {"wheelchair": False}, origin, destination)
    assert len(response.journeys[0].tags) == 0


def filter_olympic_site_destination_olympic_site_journey_tagged_test():
    # STREET_NETWORK -> PUBLIC_TRANSPORT + TRANSFER + PUBLIC_TRANSPORT + STREET_NETWORK
    #   Walking     ->  physical_mode:Metro -> Transfer -> physical_mode:Bus -> Walking
    # journey.sections[3].duration > min_pt_duration
    response = response_pb2.Response()
    journey = response.journeys.add()
    for stype in (
        response_pb2.STREET_NETWORK,
        response_pb2.PUBLIC_TRANSPORT,
        response_pb2.TRANSFER,
        response_pb2.PUBLIC_TRANSPORT,
        response_pb2.STREET_NETWORK,
    ):
        section = journey.sections.add()
        section.type = stype
    journey.sections[3].uris.physical_mode = 'physical_mode:Bus'
    journey.sections[3].duration = 60
    journey.tags.append('to_delete')

    instance = FakeInstance()
    instance.olympics_forbidden_uris = OlympicsForbiddenUris(
        pt_object_olympics_forbidden_uris=["network:abc"],
        poi_property_key="sitejo",
        poi_property_value="123",
        min_pt_duration=50,
    )
    origin = make_pt_object(type_pb2.POI, lon=1, lat=2, uri='origin:poi')
    destination = make_pt_object(type_pb2.POI, lon=3, lat=4, uri='destination:poi')
    property = destination.poi.properties.add()
    property.type = "sitejo"
    property.value = "123"
    journey_filter.filter_olympic_site([response], instance, {"wheelchair": False}, origin, destination)
    assert len(response.journeys[0].tags) == 1
    assert response.journeys[0].tags[0] == "to_delete"


def filter_olympic_site_destination_olympic_site_test1_test():
    # STREET_NETWORK -> PUBLIC_TRANSPORT + TRANSFER + PUBLIC_TRANSPORT + STREET_NETWORK
    #   Walking     ->  physical_mode:Bus -> Transfer -> physical_mode:Bus -> Walking
    # First section : journey.sections[1].duration < min_pt_duration
    # Last section : journey.sections[3].duration > min_pt_duration
    response = response_pb2.Response()
    journey = response.journeys.add()
    for stype in (
        response_pb2.STREET_NETWORK,
        response_pb2.PUBLIC_TRANSPORT,
        response_pb2.TRANSFER,
        response_pb2.PUBLIC_TRANSPORT,
        response_pb2.STREET_NETWORK,
    ):
        section = journey.sections.add()
        section.type = stype
    journey.sections[1].uris.physical_mode = 'physical_mode:Bus'
    journey.sections[1].duration = 30
    journey.sections[3].uris.physical_mode = 'physical_mode:Bus'
    journey.sections[3].duration = 60

    instance = FakeInstance()
    instance.olympics_forbidden_uris = OlympicsForbiddenUris(
        pt_object_olympics_forbidden_uris=["network:abc"],
        poi_property_key="sitejo",
        poi_property_value="123",
        min_pt_duration=50,
    )
    origin = make_pt_object(type_pb2.POI, lon=1, lat=2, uri='origin:poi')
    property = origin.poi.properties.add()
    property.type = "sitejo"
    property.value = "123"
    destination = make_pt_object(type_pb2.POI, lon=3, lat=4, uri='destination:poi')
    property = destination.poi.properties.add()
    property.type = "sitejo"
    property.value = "123"
    journey_filter.filter_olympic_site([response], instance, {"wheelchair": False}, origin, destination)
    assert len(response.journeys[0].tags) == 0


def filter_olympic_site_destination_olympic_site_test2_test():
    # STREET_NETWORK -> PUBLIC_TRANSPORT + TRANSFER + PUBLIC_TRANSPORT + STREET_NETWORK
    #   Walking     ->  physical_mode:Bus -> Transfer -> physical_mode:Bus -> Walking
    # First section : journey.sections[1].duration < min_pt_duration
    # Last section : journey.sections[3].duration < min_pt_duration
    response = response_pb2.Response()
    journey = response.journeys.add()
    for stype in (
        response_pb2.STREET_NETWORK,
        response_pb2.PUBLIC_TRANSPORT,
        response_pb2.TRANSFER,
        response_pb2.PUBLIC_TRANSPORT,
        response_pb2.STREET_NETWORK,
    ):
        section = journey.sections.add()
        section.type = stype
    journey.sections[1].uris.physical_mode = 'physical_mode:Bus'
    journey.sections[1].duration = 30
    journey.sections[3].uris.physical_mode = 'physical_mode:Bus'
    journey.sections[3].duration = 30
    journey.nb_transfers = 2

    instance = FakeInstance()
    instance.olympics_forbidden_uris = OlympicsForbiddenUris(
        pt_object_olympics_forbidden_uris=["network:abc"],
        poi_property_key="sitejo",
        poi_property_value="123",
        min_pt_duration=50,
    )
    origin = make_pt_object(type_pb2.POI, lon=1, lat=2, uri='origin:poi')
    property = origin.poi.properties.add()
    property.type = "sitejo"
    property.value = "123"
    destination = make_pt_object(type_pb2.POI, lon=3, lat=4, uri='destination:poi')
    property = destination.poi.properties.add()
    property.type = "sitejo"
    property.value = "123"
    journey_filter.filter_olympic_site([response], instance, {"wheelchair": False}, origin, destination)
    assert len(response.journeys[0].tags) == 1
    assert response.journeys[0].tags[0] == "to_delete"


def compute_journey_virtual_duration_test():
    journey = response_pb2.Journey()
    journey.departure_date_time = 0
    journey.arrival_date_time = 9600

    beginning_fallback_section = journey.sections.add()
    beginning_fallback_section.type = response_pb2.STREET_NETWORK
    beginning_fallback_section.begin_date_time = 0
    beginning_fallback_section.end_date_time = 3600
    beginning_fallback_section.origin.uri = "0;0"
    beginning_fallback_section.destination.uri = "gare de lyon"

    pt_section = journey.sections.add()
    pt_section.type = response_pb2.PUBLIC_TRANSPORT
    pt_section.begin_date_time = 3600
    pt_section.end_date_time = 7200
    pt_section.origin.uri = "gare de lyon"
    pt_section.destination.uri = "gare du nord"

    ending_fallback_section = journey.sections.add()
    ending_fallback_section.type = response_pb2.STREET_NETWORK
    ending_fallback_section.begin_date_time = 7200
    ending_fallback_section.end_date_time = 9600
    ending_fallback_section.origin.uri = "gare du nord"
    ending_fallback_section.destination.uri = "1;1"

    attractivities_virtual_fallbacks = {
        "arrival_scenario": {
            "gare de lyon": AttractivityVirtualFallback(0, 42),
            "gare du nord": AttractivityVirtualFallback(0, 84),
        }
    }
    virtual_duration, _ = journey_filter.compute_journey_virtual_duration_and_attractivity(
        journey, attractivities_virtual_fallbacks
    )
    assert virtual_duration == (9600 - 2400 + 84)

    attractivities_virtual_fallbacks = {
        "departure_scenario": {
            "gare de lyon": AttractivityVirtualFallback(0, 42),
            "gare du nord": AttractivityVirtualFallback(0, 84),
        }
    }
    virtual_duration, _ = journey_filter.compute_journey_virtual_duration_and_attractivity(
        journey, attractivities_virtual_fallbacks
    )
    assert virtual_duration == (9600 - 3600 + 42)

    # extremity stop_point not exist in attractivities_virtual_fallbacks
    attractivities_virtual_fallbacks = {
        "departure_scenario": {
            "gare du nord": AttractivityVirtualFallback(0, 84),
        }
    }
    virtual_duration, attractivity = journey_filter.compute_journey_virtual_duration_and_attractivity(
        journey, attractivities_virtual_fallbacks
    )
    assert attractivity == 0
    assert virtual_duration == (9600 - 3600 + 0)
