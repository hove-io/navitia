#  Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
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
# IRC #navitia on freenode
# https://groups.google.com/d/forum/navitia
# www.navitia.io

"""modify_id_type

Revision ID: 236b716d0c88
Revises: 176f17297061
Create Date: 2014-06-10 11:30:56.113410

"""

# revision identifiers, used by Alembic.
revision = '236b716d0c88'
down_revision = '176f17297061'

from alembic import op
import sqlalchemy as sa


def upgrade():
    alter_requests_id_BigInt()
    alter_journeys_id_BigInt()
    alter_journey_sections_id_BigInt()

def downgrade():
    alter_requests_id_Int()
    alter_journeys_id_Int()
    alter_journey_sections_id_Int()

def alter_requests_id_BigInt():
    op.alter_column(
        'requests',
        'id',
        type_=sa.BigInteger(),
        existing_type=sa.Integer(),
        schema='stat'
    )

def alter_journeys_id_BigInt():
    op.alter_column(
        'journeys',
        'id',
        type_=sa.BigInteger(),
        existing_type=sa.Integer(),
        schema='stat'
    )

def alter_journey_sections_id_BigInt():
    op.alter_column(
        'journey_sections',
        'id',
        type_=sa.BigInteger(),
        existing_type=sa.Integer(),
        schema='stat'
    )

def alter_requests_id_Int():
    op.alter_column(
        'requests',
        'id',
        type_=sa.Integer(),
        existing_type=sa.BigInteger(),
        schema='stat'
    )

def alter_journeys_id_Int():
    op.alter_column(
        'journeys',
        'id',
        type_=sa.Integer(),
        existing_type=sa.BigInteger(),
        schema='stat'
    )

def alter_journey_sections_id_Int():
    op.alter_column(
        'journey_sections',
        'id',
        type_=sa.Integer(),
        existing_type=sa.BigInteger(),
        schema='stat'
    )