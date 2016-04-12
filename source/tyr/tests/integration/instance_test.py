from tests.check_utils import api_get, api_post, api_delete, api_put, _dt
import json
import pytest
from navitiacommon import models
from tyr import app

@pytest.fixture
def create_instance():
    with app.app_context():
        instance = models.Instance('fr')
        models.db.session.add(instance)
        models.db.session.commit()
        return instance.id

def check_traveler_profile(profile, params):
    for key, param in params.iteritems():
        assert profile[key] == param

@pytest.fixture
def traveler_profile_params():
    return {"last_section_mode": ["walking", "bss", "car"],
              "walking_speed": 5.1,
              "max_car_duration_to_pt": 200,
              "wheelchair": False,
              "max_bss_duration_to_pt": 100,
              "max_walking_duration_to_pt": 300,
              "first_section_mode": ["walking"],
              "bss_speed": 6.2,
              "bike_speed": 8.8,
              "car_speed": 23.11,
              "max_bike_duration_to_pt": 500,
    }

def test_get_instances_empty():
    resp = api_get('/v0/instances/')
    assert resp == []


def test_get_instances(create_instance):
    resp = api_get('/v0/instances/')
    assert len(resp) == 1
    assert resp[0]['name'] == 'fr'
    assert resp[0]['id'] == create_instance

def test_update_instances(create_instance):
    params = {"min_tc_with_bss": 5,
              "journey_order": "arrival_time",
              "max_duration": 200,
              "max_bss_duration_to_pt": 10,
              "max_nb_transfers": 5,
              "bike_speed": 2.2,
              "walking_transfer_penalty": 20,
              "night_bus_filter_base_factor": 300,
              "walking_speed": 1.62,
              "max_duration_fallback_mode": "bike",
              "priority": 4,
              "car_speed": 55.55,
              "min_tc_with_car": 100,
              "min_tc_with_bike": 100,
              "min_bike": 40,
              "max_walking_duration_to_pt": 300,
              "min_car": 400,
              "max_bike_duration_to_pt": 600,
              "max_duration_criteria": "duration",
              "scenario": "new_default",
              "bss_speed": 2.1,
              "min_bss": 40,
              "night_bus_filter_max_factor": 1.5,
              "max_car_duration_to_pt": 800,
              "bss_provider": False}
    resp = api_put('/v0/instances/fr', data=json.dumps(params), content_type='application/json')

    for key, param in params.iteritems():
        assert resp[key] == param


def test_update_invalid_scenario(create_instance):
    params = {"scenario": "foo"}
    resp, status = api_put('/v0/instances/fr', data=json.dumps(params), check=False,
                           content_type='application/json')
    assert status == 400

def test_update_invalid_instance(create_instance):
    params = {"scenario": "foo"}
    resp, status = api_put('/v0/instances/us', data=json.dumps(params), check=False,
                           content_type='application/json')
    assert status == 404

def test_get_non_existant_profile(create_instance):
    """
    by default there is no traveler profile created for an instance
    """
    _, status = api_get('/v0/instances/fr/traveler_profiles/standard', check=False)
    assert status == 404

def test_create_empty_traveler_profile(create_instance):
    """
    we have created a profile with all default value, totally useless...
    """
    api_post('/v0/instances/fr/traveler_profiles/standard')
    api_get('/v0/instances/fr/traveler_profiles/standard')


def test_create_traveler_profile(create_instance, traveler_profile_params):
    resp = api_post('/v0/instances/fr/traveler_profiles/standard', data=json.dumps(traveler_profile_params),
                    content_type='application/json')

    check_traveler_profile(resp, traveler_profile_params)
    resp = api_get('/v0/instances/fr/traveler_profiles/standard')
    check_traveler_profile(resp[0], traveler_profile_params)

def test_update_traveler_profile(create_instance, traveler_profile_params):

    api_post('/v0/instances/fr/traveler_profiles/standard')

    resp = api_put('/v0/instances/fr/traveler_profiles/standard', data=json.dumps(traveler_profile_params),
                   content_type='application/json')
    check_traveler_profile(resp, traveler_profile_params)

    resp = api_get('/v0/instances/fr/traveler_profiles/standard')
    check_traveler_profile(resp[0], traveler_profile_params)

def test_delete_traveler_profile(create_instance):
    api_post('/v0/instances/fr/traveler_profiles/standard')

    _, status = api_delete('/v0/instances/fr/traveler_profiles/standard', check=False, no_json=True)
    assert status == 204

    _, status = api_get('/v0/instances/fr/traveler_profiles/standard', check=False)
    assert status == 404
