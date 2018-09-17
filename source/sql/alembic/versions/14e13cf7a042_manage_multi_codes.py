"""Manage multi codes

Revision ID: 14e13cf7a042
Revises: 29fc422c56cb
Create Date: 2015-05-07 15:31:55.271785

"""

# revision identifiers, used by Alembic.
revision = '14e13cf7a042'
down_revision = '12660cd87568'

from alembic import op
import sqlalchemy as sa

map_merge = []
map_merge.append({"table_name": "network", "type_name": "Network"})
map_merge.append({"table_name": "line", "type_name": "Line"})
map_merge.append({"table_name": "route", "type_name": "Route"})
map_merge.append({"table_name": "stop_area", "type_name": "StopArea"})
map_merge.append({"table_name": "stop_point", "type_name": "StopPoint"})
map_merge.append({"table_name": "vehicle_journey", "type_name": "VehicleJourney"})


def upgrade():
    op.create_table(
        'object_code',
        sa.Column('object_type_id', sa.BIGINT(), nullable=False),
        sa.Column('object_id', sa.BIGINT(), nullable=False),
        sa.Column('key', sa.TEXT(), nullable=False),
        sa.Column('value', sa.TEXT(), nullable=False),
        sa.ForeignKeyConstraint(['object_type_id'], [u'navitia.object_type.id'], name=u'object_type_id_fkey'),
        schema='navitia',
    )
    op.drop_column("calendar", 'external_code', schema='navitia')
    for table in map_merge:
        query = (
            "INSERT INTO navitia.object_code (object_id, object_type_id, key, value) "
            "SELECT nt.id, ot.id, 'external_code', nt.external_code from navitia.{table_name} nt, navitia.object_type ot "
            "where ot.name = '{type_name}' "
            "and nt.external_code is not null "
            "and nt.external_code <> '' ".format(table_name=table["table_name"], type_name=table["type_name"])
        )
        op.execute(query)
        op.drop_column(table["table_name"], 'external_code', schema='navitia')


def downgrade():
    op.add_column("calendar", sa.Column('external_code', sa.TEXT(), nullable=True), schema='navitia')
    for table in map_merge:
        op.add_column(
            table["table_name"], sa.Column('external_code', sa.TEXT(), nullable=True), schema='navitia'
        )
        query = (
            "update navitia.{table} nt set external_code=aa.value "
            "from "
            "(select oc.value, oc.object_id from navitia.object_code oc, navitia.object_type ot "
            "where oc.key='external_code' "
            "and ot.id=oc.object_type_id "
            "and ot.name='{type_name}')aa "
            "where nt.id=aa.object_id ".format(table=table["table_name"], type_name=table["type_name"])
        )
        op.execute(query)
    op.drop_table('object_code', schema='navitia')
