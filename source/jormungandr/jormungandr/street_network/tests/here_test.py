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
from __future__ import absolute_import, print_function, unicode_literals, division
import pytest
import requests
import requests_mock
from mock import MagicMock

from jormungandr.exceptions import TechnicalError
from jormungandr.street_network.here import Here
from jormungandr.street_network.tests.streetnetwork_test_utils import make_pt_object
from jormungandr.street_network.street_network import StreetNetworkPathType
from jormungandr.utils import PeriodExtremity, str_to_time_stamp
from navitiacommon import type_pb2, response_pb2


@pytest.fixture()
def valid_matrix_post_resp():
    return {
        "matrixId": "ca6631e0-6e2a-48cd-a359-f3a4f21d0e54",
        "status": "accepted",
        "statusUrl": "https://aws-eu-west-1.matrix.router.hereapi.com/v8/matrix/ca6631e0-6e2a-48cd-a359-f3a4f21d0e54/status",
    }


@pytest.fixture()
def valid_matrix_get_resp():
    return {
        "matrixId": "ca6631e0-6e2a-48cd-a359-f3a4f21d0e54",
        "matrix": {"numOrigins": 2, "numDestinations": 2, "travelTimes": [7146, 7434, 12927, 14838]},
        "regionDefinition": {"type": "world"},
    }


@pytest.fixture()
def valid_here_routing_response():
    return {
        # it's not a real response, it's a truncated to reduce its size
        'routes': [
            {
                'id': "aae539ad-eab5-485b-ba3b-b215cdd22b35",
                'sections': [
                    {
                        'id': "b9faaec5-1aae-4dff-8395-2b712a49ef7a",
                        'type': "vehicle",
                        'actions': [
                            {
                                'action': "depart",
                                'duration': 20,
                                'length': 158,
                                'instruction': "Head southwest on Avenue de Bondues (M654). Go for 158 m.",
                                'offset': 0,
                            },
                            {
                                'action': "roundaboutExit",
                                'duration': 64,
                                'length': 898,
                                'instruction': "Take the 2nd exit from roundabout onto Avenue de l'Abbé Pierre (M108). Go for 898 m.",
                                'offset': 13,
                                'direction': "right",
                                'exit': 2,
                            },
                            {
                                'action': "roundaboutExit",
                                'duration': 63,
                                'length': 542,
                                'instruction': "Take the 2nd exit from roundabout onto Avenue de l'Abbé Pierre (D108). Go for 542 m.",
                                'offset': 44,
                                'direction': "right",
                                'exit': 2,
                            },
                            {
                                'action': "roundaboutExit",
                                'duration': 471,
                                'length': 10603,
                                'instruction': "Take the 3rd exit from roundabout onto Rocade Nord-Ouest (M652) toward A25/Armentières/St André/Lambersart. Go for 10.6 km.",
                                'offset': 77,
                                'direction': "right",
                                'exit': 3,
                            },
                            {
                                'action': "arrive",
                                'duration': 0,
                                'length': 0,
                                'instruction': "Arrive at Rue Jules Guesde (M952).",
                                'offset': 774,
                            },
                        ],
                        'departure': {
                            'time': "2022-02-02T11:18:37+01:00",
                            'place': {
                                'type': "place",
                                'location': {'lat': 50.6968111, 'lng': 3.0622133},
                                'originalLocation': {'lat': 50.696585, 'lng': 3.062401},
                            },
                        },
                        'arrival': {
                            'time': "2022-02-02T11:43:05+01:00",
                            'place': {
                                'type': "place",
                                'location': {'lat': 50.5883439, 'lng': 3.0081205},
                                'originalLocation': {'lat': 50.5883469, 'lng': 3.0081329},
                            },
                        },
                        'summary': {
                            'duration': 1468,
                            'length': 21919,
                            'baseDuration': 1344,
                            'typicalDuration': 1498,
                        },
                        'polyline': "BG2mp2gDq886FtEhMjD3IvCnGvCzF_E_JrEjIzFrJ_EjI7GzKnGrJrEnGjDrE_EnGwC_EoBnGTvH7BnGvC3DjDjDjDnB3DAjD8BjD4DvCoGToGA8GoBoG7LoL_OoL_JwH_OoLnzBwlB_JwHjX8Q3N0KzZkSjmBgZrxB0ez8B4mB7pBoa3N4IjI0F7G0FjSwCjD3DvC7BjDTjDA7BoBnBUvCwC7BkDnBkDT4D7G3D3SnLvRnL7LrJvMnLzKnL_JzKjIzK_JvM_JrO7L3S_JzPzP_YnQnazK7Q7G3InG7GzFzF_E3DnBTrEjDnGvCrJvC7B_EvC_EjD3D3D7BrEAjDUvCoBvCwCjDgF7BgFnBgFTwHAoGoBsE8B4DvCsJ3D8GnGkI7GkI7GkIzF4InGoLzF4N3DoL3DwMjD0K3D4IzF8GnGsE7G8B7GnB7G3DnGnG_EjI_E_JrJjXvC7L7B7G_E3S_J7kB7B_EzF7QrE3NrT74BvC7GzFzPrJna3D_J3wB3qEnG7QrYzmCnQ3wB7GzU3NjrBvWrlCjS_2Bzev8CzZjpCjS3rB7B3D_JvWnQ3hB3I7QnL_T7GvM3S7fnQzZ3NzUjN7Q3SzZzernBvlBjwBvvBr7B_iBnuBvWjhB7V_iBrd31B3SjmB_gC_mE_iBvoCnkB_qC_Yj1BrT3mBna_2BrE3IzKvWztB_-Cnaj1BnVrsBnBvCzjBrqCzU7pBrTnkBvRnfjNjXrJzPnVjhBjXjhBnVrd3X_d7LrOrYjcvMjNjmBjmBzrCnnC7GnG_sBzoB3XjXjmBnkBjXzZvW7a_J3NzF3IrO_TvRvgB7Q3hB_O_iB7LnanQ_iBvC_E_JzU3I7QvH3NvbjwB3S7fzKjSvR_djSvgBzKnVvHvR3NnfvMjmBvMztBjI7pBnG_nBrEnpBvCzZjD7uBvC7uBTrxBoB3pCwC7zBA7aTzjBT7pBvCzyBrEzyBzF_xBjDnVT3DnLrqCnBrJzKzrCzF_nBrJ7sC7G36BzFrsBvCnanGv0BjDna3DnarEnajDvRvCrJ_EjX_EjS7BnG_ErOvC3I3I7V_E7LzK_TzFzKjS3cztBzhCvM3S_EjIjI3N7G7L7BrE_EzKjIvWnGnQ3IjcjI7kB7G_nBzFvgBnBjIrEnpBnB3IrEzjB3I_qCjIrgCzFjmBzKn4B_E3X7BvH_ErYzKzjBzKzevH3SnGzP_O3cvH3N7LvR_E7GrJnLrJnL_J_JzUjSjX_O3XzK3NzFvH7BnGnBrJ7B7VzF7ajI7L_EjXvMjXnQzZnV3SvRrsBnpBvgB7f3hBvgB_djcnLrJ7VrTvM_J3DjD3X3SnQvM7LjI3DvC7GrEjXnQ7BnB_YzP3mB7VnajNvW7LzUjI7V3IjDnB3NzF7fzKnkB7LjmBvMnpB3NrT7G_TvHjN_EjNnGjN7GvM3IvMzKnL7L_JjNvqB79BrE7GjIvMvRrdvH7L_EjIzF3IrT7a_JnLnG7GnGnG7GzFrO3I3NrE7LnBvHUzKwCjIwC_J0FzPkIvHoLjIsOrE4InGwMrEsJ3D0KjDgKvC4I3DgP3DsOrE8QnGsYnL8pBzF0UrEsO_E0P3DgK3D0KrE0KzFoLzFsJnGsJzFwHnGoGnG0FnGsEjIsE3I4DrJwCjIUrJnB7G7B3IjDjIrE7G_EnGzF7GvH7G3IzF3I7GvMzFnL_EnLrE_J3DjI3DvHvCrE3D7G3DjI3D3I_EnL7GvMzK7QzK_O_JjN_OrTjNnQ_TrYzK_OrTrdjX3hB3IrOnLvRnV7f3NzUzKrOnQzU3NnQrJzKnL7Lna_YrJjIzUzP_YjSrT7L7LnGniCjhB3N7GnajN3SrJvH3DzU_JvWnLvR7QzP3NrOzKnLvH3wBnazK7GnGnGrEzFjDnG7BnGTvHT7LT3DnBrE7BvCvCvCvCTvCUjDoB7BgFnB8GU0F8BkDwCwCnG0PvHkSjD4I_E4NzF0PjDvCvCTvCAvCoBvCkD7BsET4DAgFUsEoBkD8BwCrEoLjD8G_E4I_E8G3I0KnLoL7G8G7QkNjNkIrdsOzU0KzPkInGoB_EA3IAvCTjDAjDoBjD8BvCwC7BkD7BoG_EgFnG0FzK4InpBofjIoG7uB8kBnuBokBnQ4NvCzFvCjDjDvCjDT3DU3D4DvC4D7B0FTwHoBwHnLgKjDwCvMkNzP0UnQgZnL0UvHoQ7L0e_E0K3I8arJ0oBrEkcnBoLToGnBkI7BgF3DwHnB8BvC0FnBoGA8G8BoGwCsEkDwMoBgKUwM7BwqB7BkXjDwWA8G7GokBzFkcjIsnB_EwWvCoLnBwHjNk_BnashEnV0pD_J4wBnVgkD_JsnBvCsJnGkXvHwb3Ss7BjI4X7GsTrE8LnQkrBrJ0U3DnB_EA3DoB_EwH7B0K8B0KkD0F4DkDT8BvCsJ7BwH7G4X_EgUvCgKvCgK_E0UrE0UrEoVvC8LT8B3D8QzF4X3IsnBnBoG7G8fjDoQvC4NvCgP7BsOnBwMnBkNnB4NvCofnBsTTgKA4IU4I3DTjDAjD8BjDkD7B0FnBkIU0F8B0FwC4DwCgKoBkIUkI7B8fnBoVT4SoB8QvCsnB7BwoCwC4kC4D41BoG41BkI4kC0FssBwCkX8BoaoG46B8L06CwCoVsEsiBgF8kBsE4S4D8fkDwW8BkNkDoaoB4c7BkSnBkI7BwH3D8LnVsYnLwMjIsJrYkcnV4XrO8QvCwCzF8G_JkNjIoL3IkN7BkD_EkI3I0P3IoQ7zBolDjI0P7GkNjIsO3IsOjIkN3IkNvC4DzFkIjI0KvHkI7GoGjIoGjIsErJkDjIoB3IArJnBjIvCjI3DjI_EnGrEzFrEnG_ErJvHjDvCvR_Or2B_sB7a7V3SzPzFrEjDvCnB3D3DjDzFrEvHzF7G_E7GrE7GjD7GvCjI7B7GTvHAvHUnGoBzFoB7GwCvH4DnG4DvH0FjIoG_EgF_EsEvCsEvH0F_JkIjcwW_JkIjIwH7G8G7GkI3D0F7G8L_E4IrnB8nCzFgKzFsJnGsJ7GsJ_E0F3IsJrJ4I3N0KjS4NnGsEnGsE7G4DjIkDvHoB7GArJnBrYrE_ET7BTvCT4DvMwC7GsO_doB3D8BnGoLvqBwMzyB0F7Q0FvRkIrY8QnzBoB7BwCAs-C84B4hBsT4DzF8GjI8G7GkIvHgKjIkcvWgKjIwHzF4DTwHzF4InGwHrEkClB",
                        'spans': [
                            {
                                'offset': 0,
                                'duration': 11,
                                'baseDuration': 9,
                                'dynamicSpeedInfo': {
                                    'trafficSpeed': 11.9444447,
                                    'baseSpeed': 13.8888893,
                                    'turnTime': 0,
                                },
                            },
                            {
                                'offset': 10,
                                'duration': 9,
                                'baseDuration': 9,
                                'dynamicSpeedInfo': {
                                    'trafficSpeed': 11.9444447,
                                    'baseSpeed': 12.2222223,
                                    'turnTime': 7,
                                },
                            },
                            {
                                'offset': 13,
                                'duration': 5,
                                'baseDuration': 6,
                                'dynamicSpeedInfo': {
                                    'trafficSpeed': 11.9444447,
                                    'baseSpeed': 9.4444447,
                                    'turnTime': 0,
                                },
                            },
                            {
                                'offset': 21,
                                'duration': 5,
                                'baseDuration': 6,
                                'dynamicSpeedInfo': {
                                    'trafficSpeed': 14.166667,
                                    'baseSpeed': 12.2222223,
                                    'turnTime': 2,
                                },
                            },
                            {
                                'offset': 27,
                                'duration': 2,
                                'baseDuration': 1,
                                'dynamicSpeedInfo': {
                                    'trafficSpeed': 14.166667,
                                    'baseSpeed': 17.5,
                                    'turnTime': 0,
                                },
                            },
                            {
                                'offset': 769,
                                'duration': 4,
                                'baseDuration': 4,
                                'dynamicSpeedInfo': {
                                    'trafficSpeed': 13.8888893,
                                    'baseSpeed': 13.8888893,
                                    'turnTime': 0,
                                },
                            },
                        ],
                        'notices': [
                            {
                                'title': "mlDuration not supported for this route.",
                                'code': "mlDurationUnavailable",
                                'severity': "info",
                            }
                        ],
                        'language': "en-us",
                        'transport': {'mode': "car"},
                    }
                ],
            }
        ]
    }


def test_post_matrix(valid_matrix_post_resp):
    instance = MagicMock()
    instance.walking_speed = 1.12
    here = Here(instance=instance, service_base_url='bob.com', apiKey='toto')
    origin = make_pt_object(type_pb2.ADDRESS, 2.439938, 48.572841)
    destination = make_pt_object(type_pb2.ADDRESS, 2.440548, 48.57307)
    with requests_mock.Mocker() as req:
        req.post(requests_mock.ANY, json=valid_matrix_post_resp, status_code=202)
        post_resp = here.post_matrix_request(
            [origin],
            [destination, destination, destination],
            request={'datetime': str_to_time_stamp('20170621T174600')},
        )
        assert post_resp['status'] == 'accepted'
        assert post_resp['matrixId'] == 'ca6631e0-6e2a-48cd-a359-f3a4f21d0e54'


def test_get_matrix(valid_matrix_get_resp, valid_matrix_post_resp):
    instance = MagicMock()
    instance.walking_speed = 1.12
    here = Here(instance=instance, service_base_url='bob.com', apiKey='toto')
    origin = make_pt_object(type_pb2.ADDRESS, 2.439938, 48.572841)
    destination = make_pt_object(type_pb2.ADDRESS, 2.440548, 48.57307)
    with requests_mock.Mocker() as req:
        req.get(requests_mock.ANY, json=valid_matrix_get_resp, status_code=200)
        response = here.get_matrix_response(
            [origin], [destination, destination, destination, destination], post_resp=valid_matrix_post_resp
        )
        assert response.rows[0].routing_response[0].duration == 7146
        assert response.rows[0].routing_response[0].routing_status == response_pb2.reached
        assert response.rows[0].routing_response[1].duration == 7434
        assert response.rows[0].routing_response[1].routing_status == response_pb2.reached
        assert response.rows[0].routing_response[2].duration == 12927
        assert response.rows[0].routing_response[2].routing_status == response_pb2.reached
        assert response.rows[0].routing_response[3].duration == 14838
        assert response.rows[0].routing_response[3].routing_status == response_pb2.reached


def test_matrix_timeout():
    instance = MagicMock()
    instance.walking_speed = 1.12
    here = Here(instance=instance, service_base_url='bob.com', apiKey='toto')
    origin = make_pt_object(type_pb2.ADDRESS, 2.439938, 48.572841)
    destination = make_pt_object(type_pb2.ADDRESS, 2.440548, 48.57307)
    with requests_mock.Mocker() as req:
        # a HERE timeout should raise a TechnicalError
        req.get(requests_mock.ANY, exc=requests.exceptions.Timeout)
        with pytest.raises(TechnicalError):
            here._get_street_network_routing_matrix(
                instance,
                [origin],
                [destination, destination, destination],
                mode='walking',
                max_duration=42,
                request={'datetime': str_to_time_stamp('20170621T174600')},
                request_id=None,
            )


def here_basic_routing_test(valid_here_routing_response):
    origin = make_pt_object(type_pb2.POI, 2.439938, 48.572841)
    destination = make_pt_object(type_pb2.STOP_AREA, 2.440548, 48.57307)

    # for a beginning fallback
    fallback_extremity = PeriodExtremity(str_to_time_stamp('20161010T152000'), True)
    response = Here._read_response(
        response=valid_here_routing_response,
        mode='walking',
        origin=origin,
        destination=destination,
        fallback_extremity=fallback_extremity,
        request={'datetime': str_to_time_stamp('20161010T152000')},
        direct_path_type=StreetNetworkPathType.BEGINNING_FALLBACK,
    )
    assert response.status_code == 200
    assert response.response_type == response_pb2.ITINERARY_FOUND
    assert len(response.journeys) == 1
    assert response.journeys[0].duration == 1468
    assert len(response.journeys[0].sections) == 1
    section = response.journeys[0].sections[0]
    assert section.type == response_pb2.STREET_NETWORK
    assert section.length == 21919
    assert section.duration == 1468
    assert section.destination == destination
    assert section.origin == origin
    assert section.begin_date_time == str_to_time_stamp('20161010T152000')
    assert section.end_date_time == section.begin_date_time + section.duration
    assert section.base_begin_date_time == str_to_time_stamp('20161010T152000') + (1468 - 1344)
    assert section.base_end_date_time == section.begin_date_time + section.duration
    # dynamic_speed
    dynamic_speeds = section.street_network.dynamic_speeds
    assert len(dynamic_speeds) == 6
    first_ds = dynamic_speeds[0]
    assert round(first_ds.base_speed, 2) == 13.89
    assert round(first_ds.traffic_speed, 2) == 11.94
    assert first_ds.geojson_offset == 0
    last_ds = dynamic_speeds[5]
    assert round(last_ds.base_speed, 2) == 13.89
    assert round(last_ds.traffic_speed, 2) == 13.89
    assert last_ds.geojson_offset == 769

    # for a direct path and clockiwe == True
    response = Here._read_response(
        response=valid_here_routing_response,
        mode='walking',
        origin=origin,
        destination=destination,
        fallback_extremity=fallback_extremity,
        request={'datetime': str_to_time_stamp('20161010T152000')},
        direct_path_type=StreetNetworkPathType.DIRECT,
    )
    assert response.status_code == 200
    assert response.response_type == response_pb2.ITINERARY_FOUND
    assert len(response.journeys) == 1
    assert len(response.journeys[0].sections) == 1
    section = response.journeys[0].sections[0]
    assert section.begin_date_time == str_to_time_stamp('20161010T152000')
    assert section.end_date_time == section.begin_date_time + section.duration
    assert section.base_begin_date_time == str_to_time_stamp('20161010T152000') + (1468 - 1344)
    assert section.base_end_date_time == section.begin_date_time + section.duration

    # for a ending fallback
    fallback_extremity = PeriodExtremity(str_to_time_stamp('20161010T152000'), True)
    response = Here._read_response(
        response=valid_here_routing_response,
        mode='walking',
        origin=origin,
        destination=destination,
        fallback_extremity=fallback_extremity,
        request={'datetime': str_to_time_stamp('20161010T152000')},
        direct_path_type=StreetNetworkPathType.ENDING_FALLBACK,
    )
    assert response.status_code == 200
    assert response.response_type == response_pb2.ITINERARY_FOUND
    assert len(response.journeys) == 1
    section = response.journeys[0].sections[0]
    assert section.begin_date_time == str_to_time_stamp('20161010T152000') - section.duration
    assert section.end_date_time == str_to_time_stamp('20161010T152000')
    assert section.base_begin_date_time == str_to_time_stamp('20161010T152000') - section.duration
    assert section.base_end_date_time == section.end_date_time - (1468 - 1344)

    # for a direct path and clockiwe == False
    fallback_extremity = PeriodExtremity(str_to_time_stamp('20161010T152000'), False)
    response = Here._read_response(
        response=valid_here_routing_response,
        mode='walking',
        origin=origin,
        destination=destination,
        fallback_extremity=fallback_extremity,
        request={'datetime': str_to_time_stamp('20161010T152000')},
        direct_path_type=StreetNetworkPathType.DIRECT,
    )
    assert response.status_code == 200
    assert response.response_type == response_pb2.ITINERARY_FOUND
    assert len(response.journeys) == 1
    assert len(response.journeys[0].sections) == 1
    section = response.journeys[0].sections[0]
    assert section.end_date_time == str_to_time_stamp('20161010T152000')
    assert section.begin_date_time == section.end_date_time - section.duration
    assert section.base_end_date_time == section.end_date_time
    assert section.base_begin_date_time == section.base_end_date_time - section.duration + (1468 - 1344)

    # for a beginning fallback and clockiwe == False
    fallback_extremity = PeriodExtremity(str_to_time_stamp('20161010T152000'), False)
    response = Here._read_response(
        response=valid_here_routing_response,
        mode='walking',
        origin=origin,
        destination=destination,
        fallback_extremity=fallback_extremity,
        request={'datetime': str_to_time_stamp('20161010T152000')},
        direct_path_type=StreetNetworkPathType.BEGINNING_FALLBACK,
    )
    assert response.status_code == 200
    assert response.response_type == response_pb2.ITINERARY_FOUND
    assert len(response.journeys) == 1
    assert len(response.journeys[0].sections) == 1
    section = response.journeys[0].sections[0]
    assert section.end_date_time == str_to_time_stamp('20161010T152000')
    assert section.begin_date_time == section.end_date_time - section.duration
    assert section.base_end_date_time == section.end_date_time
    assert section.base_begin_date_time == section.base_end_date_time - section.duration + (1468 - 1344)


def status_test():
    here = Here(
        instance=None,
        service_base_url='http://bob.com',
        id=u"tata-é$~#@\"*!'`§èû",
        modes=["walking", "bike", "car"],
        timeout=89,
    )
    status = here.status()
    assert len(status) == 8
    assert status['id'] == u'tata-é$~#@"*!\'`§èû'
    assert status['class'] == "Here"
    assert status['modes'] == ["walking", "bike", "car"]
    assert status['timeout'] == 89
    assert status['max_matrix_points'] == 100
    assert status['lapse_time_matrix_to_retry'] == 0.1
    assert status['language'] == "en-gb"
    assert len(status['circuit_breaker']) == 3
    assert status['circuit_breaker']['current_state'] == 'closed'
    assert status['circuit_breaker']['fail_counter'] == 0
    assert status['circuit_breaker']['reset_timeout'] == 60
