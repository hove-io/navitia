# coding: utf-8
# Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
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

from __future__ import absolute_import, print_function, division
from tyr.helper import is_activate_autocomplete_version, load_instance_config, get_instances_name
from tyr import app
import pytest
import os


def test_is_activate_autocomplete_version_with_mimir2(enable_mimir2):
    with app.app_context():
        assert is_activate_autocomplete_version(2)
        assert not is_activate_autocomplete_version(7)


def test_is_activate_autocomplete_version_with_mimir2_mimir7(enable_mimir2_and_mimir):
    with app.app_context():
        assert is_activate_autocomplete_version(2)
        assert is_activate_autocomplete_version(7)


def test_is_activate_autocomplete_version_with_mimir7(enable_mimir7):
    with app.app_context():
        assert not is_activate_autocomplete_version(2)
        assert is_activate_autocomplete_version(7)


def test_is_activate_autocomplete_version_without_mimir(disable_mimir):
    with app.app_context():
        assert not is_activate_autocomplete_version(2)
        assert not is_activate_autocomplete_version(7)


def test_valid_config_instance_from_env_variables(valid_instance_env_variables):
    instance = load_instance_config("fr-se-lyon", force_create_paths=False)
    assert instance.aliases_file == "/ed/aliases"
    assert instance.backup_directory == "/ed/backup"
    assert instance.exchange == "exchange"
    assert instance.is_free == True
    assert instance.name == "fr-se-lyon"
    assert instance.pg_port == 492


def test_valid_config_instance_from_env_variables_upper_instance_name(valid_instance_env_variables):
    instance = load_instance_config("FR-SE-lyon", force_create_paths=False)
    assert instance.aliases_file == "/ed/aliases"
    assert instance.backup_directory == "/ed/backup"
    assert instance.exchange == "exchange"
    assert instance.is_free == True
    assert instance.name == "fr-se-lyon"
    assert instance.pg_port == 492


def test_valid_config_instance_from_env_variables_and_instance_not_in_env_variables(
    valid_instance_env_variables,
):
    with app.app_context():
        with pytest.raises(ValueError) as exc:
            load_instance_config("fr-auv", force_create_paths=False)
        assert 'File doesn\'t exists or is not a file' in str(exc.value)
        assert '/fr-auv.ini' in str(exc.value)


def test_invalid_config_instance_from_env_variables(invalid_instance_env_variables):

    with pytest.raises(ValueError) as exc:
        load_instance_config("fr-se-lyon", force_create_paths=False)

    assert "Config is not valid for instance fr-se-lyon" in str(exc.value)
    assert "'492' is not of type 'number'" in str(exc.value)


def test_get_instances_name(init_instances_dir, valid_instance_env_variables):
    with app.app_context():
        instances = get_instances_name()
        assert len(instances) == 2
        for name in ["fr", "fr-se-lyon"]:
            assert name in instances


def test_get_instances_name_same_instance(init_instances_dir, valid_instance_env_variables_fr):
    # the same instance in config file and env variables
    with app.app_context():
        instances = get_instances_name()
        assert len(instances) == 1
        assert "fr" in instances


def test_create_repositories(create_repositories_instance_env_variables):
    with app.app_context():
        instance = load_instance_config("auv")
        for path in [
            instance.source_directory,
            instance.backup_directory,
            instance.synonyms_file,
            instance.aliases_file,
        ]:
            assert path.startswith("/tmp/tyr_instance_auv_")
            assert os.path.exists(path)
