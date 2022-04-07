# encoding: utf-8
# Copyright (c) 2001-2019, Hove and/or its affiliates. All rights reserved.
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

from mock import MagicMock
from jormungandr.equipments.sytral import SytralProvider
from navitiacommon.type_pb2 import StopPoint, StopAreaEquipment, StopArea

mock_data = {
    "equipments_details": [
        {
            "id": "261",
            "name": "sortie Place Basse niveau Parc Relais",
            "embedded_type": "escalator",
            "current_availaibity": {
                "status": "available",
                "cause": {"label": "Probleme technique"},
                "effect": {"label": "."},
                "periods": [{"begin": "2018-09-14T00:00:00+02:00", "end": "2018-09-15T23:30:00+02:00"}],
                "updated_at": "2018-09-15T12:01:31+02:00",
            },
        }
    ]
}


def create_stop_point(code_type, code_value):
    st = StopPoint()
    code = st.codes.add()
    code.type = code_type
    code.value = code_value
    return st


def equipments_get_information_test():
    """
    Test that 'equipment_details' structure is added to StopPoint proto when conditions are met
    """
    provider = SytralProvider(url="sytral.url", timeout=3, code_types=["TCL_ASCENSEUR", "TCL_ESCALIER"])
    provider._call_webservice = MagicMock(return_value=mock_data)

    # stop point has code with correct type and value present in webservice response
    # equipment_details is added
    st = create_stop_point("TCL_ASCENSEUR", "261")
    provider.get_informations_for_journeys([st])
    assert st.equipment_details

    # stop point has code with correct type but value not present in webservice response
    # equipment_details is not added
    st = create_stop_point("TCL_ASCENSEUR", "262")
    provider.get_informations_for_journeys([st])
    assert not st.equipment_details

    # stop point has code with incorrect type but value present in webservice response
    # equipment_details is not added
    st = create_stop_point("TCL_ASCENCEUR", "261")
    provider.get_informations_for_journeys([st])
    assert not st.equipment_details


def equipments_get_informations_for_reports_not_duplicated():
    """
    When 2 stop points (from the same stop areas) refer to the same equipment (type and value),
    we only want 1 equipment_details object that refers to it as we report information per stop area.
    """
    CODE_TYPE = "TCL_ESCALIER"
    provider = SytralProvider(url="sytral.url", timeout=3, code_types=[CODE_TYPE])
    provider._call_webservice = MagicMock(return_value=mock_data)

    sae = StopAreaEquipment()
    sae.stop_area.name = "StopArea1_name"
    sae.stop_area.uri = "StopArea1_uri"

    sp1 = sae.stop_area.stop_points.add()
    codes1 = sp1.codes.add()
    codes1.type = CODE_TYPE
    codes1.value = "val"

    sp2 = sae.stop_area.stop_points.add()
    codes2 = sp2.codes.add()
    codes2.type = CODE_TYPE
    codes2.value = "val"

    provider.get_informations_for_equipment_reports([sae])

    assert len(sae.equipment_details) == 1
