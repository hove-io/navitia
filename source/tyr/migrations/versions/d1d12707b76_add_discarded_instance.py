# Copyright (c) 2001-2016, Hove and/or its affiliates. All rights reserved.
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

""" Add field 'discarded' (boolean) to table 'instance',
    to manage instance deletion.
Revision ID: d1d12707b76
Revises: 520d48947e65
Create Date: 2016-06-10 12:11:00.704902
"""

# revision identifiers, used by Alembic.
revision = 'd1d12707b76'
down_revision = '520d48947e65'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column('instance', sa.Column('discarded', sa.Boolean(), nullable=False, server_default=sa.false()))


def downgrade():
    op.drop_column('instance', 'discarded')
