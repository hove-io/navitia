# encoding: utf-8
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

from jormungandr.equipments.equipment_provider_manager import EquipmentProviderManager
from navitiacommon.models import EquipmentsProvider

from mock import MagicMock
import datetime


def equipments_provider_manager_env_test():
    """
    Test that equipments providers are created from env when conditions are met
    """
    manager = EquipmentProviderManager(
        equipment_providers_configuration=[{'key': 'SytralRT', 'class': 'Sytral.class', 'args': 'Sytral.args'}]
    )
    manager._init_class = MagicMock(return_value="Provider")

    # Provider name from instance doesn't match config provider
    # No provider added in providers list
    manager.init_providers(['divia'])
    assert not manager._equipment_providers
    assert not manager._equipment_providers_legacy

    # Provider name from instance matches config provider
    # Sytral provider added in providers list
    manager.init_providers(['SytralRT'])
    assert not manager._equipment_providers
    assert len(manager._equipment_providers_legacy) == 1
    assert list(manager._equipment_providers_legacy)[0] == 'SytralRT'
    assert manager._get_providers() == {'SytralRT': 'Provider'}

    # Provider already created
    # No new provider added in providers list
    manager.init_providers(['SytralRT'])
    assert not manager._equipment_providers
    assert len(manager._equipment_providers_legacy) == 1


def providers_getter_ok():
    provider1 = EquipmentsProvider(id='sytral')
    provider1.klass = 'jormungandr.equipments.tests.EquipmentsProviderMock'
    provider1.args = {'url': 'sytral.url'}
    provider1.created_at = datetime.datetime.utcnow()

    provider2 = EquipmentsProvider(id='sytral2')
    provider2.klass = 'jormungandr.equipments.tests.EquipmentsProviderMock'
    provider2.args = {'url': 'sytral2.url'}
    return [provider1, provider2]


def provider_getter_update():
    provider = EquipmentsProvider(id='sytral')
    provider.klass = 'jormungandr.equipments.tests.EquipmentsProviderMock'
    provider.args = {'url': 'sytral.url.UPDATE'}
    provider.updated_at = datetime.datetime.utcnow()
    return [provider]


def providers_getter_wrong_class():
    provider = EquipmentsProvider(id='sytral')
    provider.klass = 'jormungandr/equipments/tests/EquipmentsProviderMock'
    provider.args = {'url': 'sytral.url'}
    return [provider]


def equipments_provider_manager_db_test():
    """
    Test that equipments providers are created from db when conditions are met
    """
    manager = EquipmentProviderManager([], providers_getter_ok, -1)

    # 2 providers defined in db are associated to the coverage
    # -> 2 providers created
    manager.init_providers(['sytral'])
    manager.update_config()
    assert not manager._equipment_providers_legacy
    assert len(manager._equipment_providers) == 2
    assert 'sytral' in manager._equipment_providers
    assert manager._equipment_providers['sytral'].url == 'sytral.url'
    assert 'sytral2' in manager._equipment_providers
    assert manager._equipment_providers['sytral2'].url == 'sytral2.url'

    manager_update = manager._last_update
    assert 'sytral' in manager._equipment_providers_last_update
    sytral_update = manager._equipment_providers_last_update['sytral']

    # Provider already existing is updated
    manager._providers_getter = provider_getter_update
    manager.update_config()
    assert manager._last_update > manager_update
    assert not manager._equipment_providers_legacy
    assert len(manager._equipment_providers) == 2
    assert 'sytral' in manager._equipment_providers
    assert manager._equipment_providers['sytral'].url == 'sytral.url.UPDATE'
    assert 'sytral' in manager._equipment_providers_last_update
    assert manager._equipment_providers_last_update['sytral'] > sytral_update

    # Long update interval so provider shouldn't be updated
    manager = EquipmentProviderManager([], providers_getter_ok, 600)
    manager.init_providers(['sytral'])
    manager.update_config()
    assert not manager._equipment_providers_legacy
    assert len(manager._equipment_providers) == 2
    assert 'sytral' in manager._equipment_providers
    assert manager._equipment_providers['sytral'].url == 'sytral.url'
    manager_update = manager._last_update

    manager._providers_getter = provider_getter_update
    manager.update_config()
    assert manager._last_update == manager_update
    assert not manager._equipment_providers_legacy
    assert len(manager._equipment_providers) == 2
    assert 'sytral' in manager._equipment_providers
    assert manager._equipment_providers['sytral'].url == 'sytral.url'


def wrong_equipments_provider_test():
    """
    Test that equipments providers with wrong parameters aren't created
    """

    # Provider has a class wrongly formatted
    manager = EquipmentProviderManager([], providers_getter_wrong_class, -1)
    manager.init_providers(['sytral'])
    assert not manager._equipment_providers_legacy
    assert not manager._equipment_providers

    # No providers available in db
    manager._providers_getter = []
    manager.update_config()
    assert not manager._equipment_providers_legacy
    assert not manager._equipment_providers


def equipments_provider_manager_env_vs_db_test():
    """
    Test that equipments providers from db have priority over providers from env
    """
    manager = EquipmentProviderManager(
        equipment_providers_configuration=[{'key': 'sytral', 'class': 'Sytral.class', 'args': 'Sytral.args'}]
    )
    manager._init_class = MagicMock(return_value="Provider")

    # Provider 'sytral' will be created in legacy list
    manager.init_providers(['sytral'])
    assert not manager._equipment_providers
    assert len(manager._equipment_providers_legacy) == 1
    assert list(manager._equipment_providers_legacy)[0] == 'sytral'

    # Provider 'sytral' will be created in providers list and deleted from legacy
    manager._providers_getter = providers_getter_ok
    manager.update_config()
    assert not manager._equipment_providers_legacy
    assert 'sytral' in manager._equipment_providers
