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

standard_mock_escalator_data = {
    "equipments_details": [
        {
            "id": "1",
            "name": "equipment_detail_name_1",
            "embedded_type": "escalator",
            "current_availaibity": {
                "status": "available",
                "cause": {"label": "cause_message_1"},
                "effect": {"label": "effect_message_1"},
                "periods": [{"begin": "2012-07-01T08:10:00+02:00", "end": "2012-09-01T23:30:00+02:00"}],
                "updated_at": "2012-07-25T15:01:31+02:00",
            },
        },
        {
            "id": "2",
            "name": "equipment_detail_name_2",
            "embedded_type": "escalator",
            "current_availaibity": {
                "status": "unavailable",
                "cause": {"label": "cause_message_2"},
                "effect": {"label": "effect_message_2"},
                "periods": [{"begin": "2012-07-01T08:10:00+02:00", "end": "2012-09-01T23:30:00+02:00"}],
                "updated_at": "2012-07-25T15:01:31+02:00",
            },
        },
        {
            "id": "3",
            "name": "equipment_detail_name_3",
            "embedded_type": "escalator",
            "current_availaibity": {
                "status": "unknown",
                "cause": {"label": "cause_message_3"},
                "effect": {"label": "effect_message_3"},
                "periods": [{"begin": "2012-07-01T08:10:00+02:00", "end": "2012-09-01T23:30:00+02:00"}],
                "updated_at": "2012-07-25T15:01:31+02:00",
            },
        },
    ]
}

wrong_mock_with_bad_id_data = {
    "equipments_details": [
        {
            "id": "qwdwqq",
            "name": "equipment_detail_name_1",
            "embedded_type": "escalator",
            "current_availaibity": {
                "status": "available",
                "cause": {"label": "cause_message_1"},
                "effect": {"label": "effect_message_1"},
                "periods": [{"begin": "2012-07-01T08:10:00+02:00", "end": "2012-09-01T23:30:00+02:00"}],
                "updated_at": "2012-07-25T15:01:31+02:00",
            },
        },
        {
            "id": "-1234",
            "name": "equipment_detail_name_2",
            "embedded_type": "escalator",
            "current_availaibity": {
                "status": "unavailable",
                "cause": {"label": "cause_message_2"},
                "effect": {"label": "effect_message_2"},
                "periods": [{"begin": "2012-07-01T08:10:00+02:00", "end": "2012-09-01T23:30:00+02:00"}],
                "updated_at": "2012-07-25T15:01:31+02:00",
            },
        },
        {
            "id": "3123.124",
            "name": "equipment_detail_name_3",
            "embedded_type": "escalator",
            "current_availaibity": {
                "status": "unknown",
                "cause": {"label": "cause_message_3"},
                "effect": {"label": "effect_message_3"},
                "periods": [{"begin": "2012-07-01T08:10:00+02:00", "end": "2012-09-01T23:30:00+02:00"}],
                "updated_at": "2012-07-25T15:01:31+02:00",
            },
        },
    ]
}

standard_mock_elevator_data = {
    "equipments_details": [
        {
            "id": "6",
            "name": "equipment_detail_name_1",
            "embedded_type": "elevator",
            "current_availaibity": {
                "status": "available",
                "cause": {"label": "cause_message_1"},
                "effect": {"label": "effect_message_1"},
                "periods": [{"begin": "2012-07-01T08:10:00+02:00", "end": "2012-09-01T23:30:00+02:00"}],
                "updated_at": "2012-07-25T15:01:31+02:00",
            },
        },
        {
            "id": "7",
            "name": "equipment_detail_name_2",
            "embedded_type": "elevator",
            "current_availaibity": {
                "status": "unavailable",
                "cause": {"label": "cause_message_2"},
                "effect": {"label": "effect_message_2"},
                "periods": [{"begin": "2012-07-01T08:10:00+02:00", "end": "2012-09-01T23:30:00+02:00"}],
                "updated_at": "2012-07-25T15:01:31+02:00",
            },
        },
        {
            "id": "8",
            "name": "equipment_detail_name_3",
            "embedded_type": "elevator",
            "current_availaibity": {
                "status": "unknown",
                "cause": {"label": "cause_message_3"},
                "effect": {"label": "effect_message_3"},
                "periods": [{"begin": "2012-07-01T08:10:00+02:00", "end": "2012-09-01T23:30:00+02:00"}],
                "updated_at": "2012-07-25T15:01:31+02:00",
            },
        },
    ]
}

standard_mock_mixed_data = {
    "equipments_details": [
        {
            "id": "1",
            "name": "equipment_detail_name_1",
            "embedded_type": "escalator",
            "current_availaibity": {
                "status": "available",
                "cause": {"label": "cause_message_1"},
                "effect": {"label": "effect_message_1"},
                "periods": [{"begin": "2012-07-01T08:10:00+02:00", "end": "2012-09-01T23:30:00+02:00"}],
                "updated_at": "2012-07-25T15:01:31+02:00",
            },
        },
        {
            "id": "6",
            "name": "equipment_detail_name_2",
            "embedded_type": "elevator",
            "current_availaibity": {
                "status": "available",
                "cause": {"label": "cause_message_2"},
                "effect": {"label": "effect_message_2"},
                "periods": [{"begin": "2012-07-01T08:10:00+02:00", "end": "2012-09-01T23:30:00+02:00"}],
                "updated_at": "2012-07-25T15:01:31+02:00",
            },
        },
        {
            "id": "3",
            "name": "equipment_detail_name_3",
            "embedded_type": "escalator",
            "current_availaibity": {
                "status": "available",
                "cause": {"label": "cause_message_3"},
                "effect": {"label": "effect_message_3"},
                "periods": [{"begin": "2012-07-01T08:10:00+02:00", "end": "2012-09-01T23:30:00+02:00"}],
                "updated_at": "2012-07-25T15:01:31+02:00",
            },
        },
    ]
}
