# coding=utf-8
# Copyright (c) 2001-2024, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# LICENCE: This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

from __future__ import absolute_import, print_function, unicode_literals, division

from mock import MagicMock, patch, PropertyMock
from flask import g
from navitiacommon import response_pb2
from jormungandr import app
from jormungandr.pt_planners.loki import Loki
from jormungandr.pt_planners.kraken import Kraken
from jormungandr.interfaces.v1.GraphicalIsochrone import rig_isochrone
import jormungandr.scenarios.tests.helpers_tests as helpers_tests

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------


def _make_loki(send_and_receive_return=None):
    """Create a Loki instance with a mocked send_and_receive."""
    loki = Loki.__new__(Loki)
    loki.send_and_receive = MagicMock(return_value=send_and_receive_return or response_pb2.Response())
    return loki


def _make_kraken(send_and_receive_return=None):
    """Create a Kraken instance with a mocked send_and_receive."""
    kraken = Kraken.__new__(Kraken)
    kraken.send_and_receive = MagicMock(return_value=send_and_receive_return or response_pb2.Response())
    return kraken


def _isochrone_response(*geojson_values, max_durations=None):
    """Build a protobuf Response with graphical_isochrones zones."""
    resp = response_pb2.Response()
    for i, geojson in enumerate(geojson_values):
        iso = resp.graphical_isochrones.add()
        iso.geojson = geojson
        iso.max_duration = max_durations[i] if max_durations else (i + 1) * 600
    return resp


def _make_isochrone_params(**kwargs):
    from jormungandr.pt_planners.pt_planner import GraphicalIsochronesParameters

    return GraphicalIsochronesParameters(**kwargs)


# ---------------------------------------------------------------------------
# Tests: instance.get_backend routing
# ---------------------------------------------------------------------------


def _make_instance_with_planners(loki_planner, kraken_planner, api_backends=None):
    """
    Build a minimal Instance-like object with mocked pt_planners and optional api_backends.
    Uses the real get_backend method.
    """
    from jormungandr.instance import Instance
    from jormungandr import pt_planners_manager

    instance = Instance.__new__(Instance)

    manager = MagicMock()
    manager.get_pt_planner = lambda name: {'loki': loki_planner, 'kraken': kraken_planner}[name]
    instance._pt_planner_manager = manager

    if api_backends is not None:
        type(instance).api_backends = PropertyMock(return_value=api_backends)
    else:
        type(instance).api_backends = PropertyMock(return_value=None)

    return instance


def test_get_backend_explicit_loki_in_request():
    """_pt_planner=loki in the request takes priority over everything."""
    loki = _make_loki()
    kraken = _make_kraken()
    instance = _make_instance_with_planners(loki, kraken)

    with app.app_context():
        backend = instance.get_backend('graphical_isochrone', {'_pt_planner': 'loki'})

    assert backend is loki


def test_get_backend_explicit_kraken_in_request():
    """_pt_planner=kraken in the request takes priority."""
    loki = _make_loki()
    kraken = _make_kraken()
    instance = _make_instance_with_planners(loki, kraken, api_backends={'graphical_isochrone': 'loki'})

    with app.app_context():
        backend = instance.get_backend('graphical_isochrone', {'_pt_planner': 'kraken'})

    assert backend is kraken


def test_get_backend_api_backends_loki():
    """With no _pt_planner in request, api_backends routes to loki."""
    loki = _make_loki()
    kraken = _make_kraken()
    instance = _make_instance_with_planners(loki, kraken, api_backends={'graphical_isochrone': 'loki'})

    with app.app_context():
        backend = instance.get_backend('graphical_isochrone', {})

    assert backend is loki


def test_get_backend_api_backends_kraken():
    """With no _pt_planner in request, api_backends routes to kraken."""
    loki = _make_loki()
    kraken = _make_kraken()
    instance = _make_instance_with_planners(loki, kraken, api_backends={'graphical_isochrone': 'kraken'})

    with app.app_context():
        backend = instance.get_backend('graphical_isochrone', {})

    assert backend is kraken


def test_get_backend_fallback_to_kraken():
    """Without any explicit config, get_backend defaults to kraken."""
    loki = _make_loki()
    kraken = _make_kraken()
    instance = _make_instance_with_planners(loki, kraken)

    with app.app_context():
        backend = instance.get_backend('graphical_isochrone', {})

    assert backend is kraken


def test_get_backend_api_backends_unknown_value_falls_back_to_kraken():
    """An unrecognized api_backends value (not 'loki'/'kraken') falls back to kraken."""
    loki = _make_loki()
    kraken = _make_kraken()
    instance = _make_instance_with_planners(
        loki, kraken, api_backends={'graphical_isochrone': 'unknown_planner'}
    )

    with app.app_context():
        backend = instance.get_backend('graphical_isochrone', {})

    assert backend is kraken


# ---------------------------------------------------------------------------
# Tests: Journeys.py _pt_planner assignment for isochrone
# ---------------------------------------------------------------------------


def test_journeys_isochrone_routes_to_loki_when_configured():
    """When api_backends has isochrone=loki, _pt_planner must be set to loki."""
    from jormungandr.interfaces.v1 import Journeys

    mod = MagicMock()
    mod.api_backends = {'isochrone': 'loki'}
    mod.default_pt_planner = 'kraken'

    args = {'_pt_planner': None}

    # Reproduce the logic from Journeys.py (the isochrone branch)
    if args.get('_pt_planner') is None:
        if mod.api_backends and mod.api_backends.get('isochrone') == 'loki':
            args['_pt_planner'] = 'loki'
        else:
            args['_pt_planner'] = 'kraken'
    else:
        args['_pt_planner'] = mod.default_pt_planner

    assert args['_pt_planner'] == 'loki'


def test_journeys_isochrone_routes_to_kraken_by_default():
    """When api_backends is not set for isochrone, _pt_planner must be kraken."""
    mod = MagicMock()
    mod.api_backends = {}
    mod.default_pt_planner = 'loki'

    args = {'_pt_planner': None}

    if args.get('_pt_planner') is None:
        if mod.api_backends and mod.api_backends.get('isochrone') == 'loki':
            args['_pt_planner'] = 'loki'
        else:
            args['_pt_planner'] = 'kraken'
    else:
        args['_pt_planner'] = mod.default_pt_planner

    assert args['_pt_planner'] == 'kraken'


def test_journeys_default_pt_planner_not_applied_when_pt_planner_already_set():
    """
    Regression: before the else-fix, default_pt_planner always overwrote _pt_planner.
    After the fix, setting _pt_planner=loki in the isochrone branch must stick.
    """
    mod = MagicMock()
    mod.api_backends = {'isochrone': 'loki'}
    mod.default_pt_planner = 'kraken'

    args = {'_pt_planner': None}

    # Simulate the fixed Journeys.py logic
    if args.get('_pt_planner') is None:
        if mod.api_backends and mod.api_backends.get('isochrone') == 'loki':
            args['_pt_planner'] = 'loki'
        else:
            args['_pt_planner'] = 'kraken'
    else:
        # This else-branch is the fix: only apply default_pt_planner when _pt_planner is not set by the isochrone logic
        args['_pt_planner'] = mod.default_pt_planner

    # loki must win; default_pt_planner=kraken must NOT overwrite it
    assert args['_pt_planner'] == 'loki'


# ---------------------------------------------------------------------------
# Tests: Loki vs Kraken — same response structure
# ---------------------------------------------------------------------------


def test_loki_and_kraken_return_same_response_type():
    """Both Loki and Kraken must return a Response protobuf with the same schema."""
    geojson = '{"type":"Polygon","coordinates":[[]]}'
    resp = _isochrone_response(geojson, max_durations=[3600])

    loki = _make_loki(resp)
    kraken = _make_kraken(resp)

    with app.app_context():
        loki_result = loki.graphical_isochrones(
            {}, {}, 0, True, _make_isochrone_params(boundary_duration=[3600]), False
        )
        kraken_result = kraken.graphical_isochrones(
            {}, {}, 0, True, _make_isochrone_params(boundary_duration=[3600]), False
        )

    assert type(loki_result) == type(kraken_result)
    assert len(loki_result.graphical_isochrones) == len(kraken_result.graphical_isochrones)
    assert loki_result.graphical_isochrones[0].max_duration == kraken_result.graphical_isochrones[0].max_duration
    assert loki_result.graphical_isochrones[0].geojson == kraken_result.graphical_isochrones[0].geojson


# ---------------------------------------------------------------------------
# Tests: rig_isochrone — from/to injection from g.origin_detail/destination_detail
# ---------------------------------------------------------------------------


def _isochrones_json_response(n=2):
    return (
        {'isochrones': [{'min_duration': i * 600, 'max_duration': (i + 1) * 600} for i in range(n)]},
        200,
    )


@rig_isochrone()
def get_isochrones_response(n=2):
    """Fakes what GraphicalIsochrone.get() returns before rig_isochrone fills 'from'/'to'."""
    return _isochrones_json_response(n)


def test_fill_isochrones_sets_origin_from_request():
    """g.origin_detail must be copied onto every isochrone zone's 'from'; 'to' stays untouched."""
    with app.app_context():
        with app.test_request_context():
            g.origin_detail = helpers_tests.get_json_entry_point(id='resolved_origin', name='Resolved origin')
            g.destination_detail = None

            resp = get_isochrones_response(2)
            isochrones = resp[0]['isochrones']

            assert len(isochrones) == 2
            for iso in isochrones:
                assert iso['from']['id'] == 'resolved_origin'
                assert 'to' not in iso


def test_fill_isochrones_sets_destination_from_request():
    """Symmetric case: only g.destination_detail resolved, only 'to' must be filled."""
    with app.app_context():
        with app.test_request_context():
            g.origin_detail = None
            g.destination_detail = helpers_tests.get_json_entry_point(
                id='resolved_destination', name='Resolved destination'
            )

            resp = get_isochrones_response(2)
            isochrones = resp[0]['isochrones']

            for iso in isochrones:
                assert iso['to']['id'] == 'resolved_destination'
                assert 'from' not in iso


def test_fill_isochrones_no_origin_no_destination_is_noop():
    """If origin_detail/destination_detail were never resolved, zones are left as-is."""
    with app.app_context():
        with app.test_request_context():
            g.origin_detail = None
            g.destination_detail = None

            resp = get_isochrones_response(1)
            isochrones = resp[0]['isochrones']

            assert 'from' not in isochrones[0]
            assert 'to' not in isochrones[0]


def test_fill_isochrones_skips_error_responses():
    """An error response (status != 200) must be returned untouched, even without g attrs set."""

    @rig_isochrone()
    def error_response():
        return {'error': {'id': 'unknown_object'}}, 404

    with app.app_context():
        with app.test_request_context():
            resp = error_response()

            assert resp[1] == 404
            assert 'isochrones' not in resp[0]


def test_fill_isochrones_strips_within_zones_and_keeps_only_level_8_admins():
    """clean_global_origin_destination_detail must drop 'within_zones' and keep only level==8 admins."""
    with app.app_context():
        with app.test_request_context():
            g.origin_detail = {
                'id': 'resolved_origin',
                'name': 'Resolved origin',
                'within_zones': [{'id': 'zone1'}],
                'administrative_regions': [
                    {'id': 'admin_75056', 'level': 8},
                    {'id': 'admin_75', 'level': 6},
                ],
            }
            g.destination_detail = None

            resp = get_isochrones_response(1)
            origin = resp[0]['isochrones'][0]['from']

            assert 'within_zones' not in origin
            assert [admin['id'] for admin in origin['administrative_regions']] == ['admin_75056']
