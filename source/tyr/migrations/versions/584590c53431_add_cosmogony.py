"""change all autocomplete_parameters to text instead of enum
since enums are a pain to migrate in PG

Revision ID: 584590c53431
Revises: 19be8e1cd586
Create Date: 2019-02-26 10:09:00.935094

"""

# revision identifiers, used by Alembic.
revision = '584590c53431'
down_revision = '19be8e1cd586'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.alter_column('autocomplete_parameter', 'admin', type_=sa.TEXT())
    op.alter_column('autocomplete_parameter', 'poi', type_=sa.TEXT())
    op.alter_column('autocomplete_parameter', 'street', type_=sa.TEXT())
    op.alter_column('autocomplete_parameter', 'address', type_=sa.TEXT())
    sa.Enum(name='source_street').drop(op.get_bind(), checkfirst=False)
    sa.Enum(name='source_address').drop(op.get_bind(), checkfirst=False)
    sa.Enum(name='source_poi').drop(op.get_bind(), checkfirst=False)
    sa.Enum(name='source_admin').drop(op.get_bind(), checkfirst=False)

    # with cosmogony admin_level are no longer mandatory
    op.alter_column('autocomplete_parameter', 'admin_level', nullable=True, server_default="{}")


def downgrade():
    source_street = sa.Enum('OSM', name='source_street')
    source_address = sa.Enum('BANO', 'OSM', name='source_address')
    source_poi = sa.Enum('FUSIO', 'OSM', name='source_poi')
    source_admin = sa.Enum('OSM', name='source_admin')

    source_street.create(op.get_bind())
    source_address.create(op.get_bind())
    source_poi.create(op.get_bind())
    source_admin.create(op.get_bind())

    op.alter_column('autocomplete_parameter', 'admin', new_column_name='_admin')
    op.alter_column('autocomplete_parameter', 'poi', new_column_name='_poi')
    op.alter_column('autocomplete_parameter', 'street', new_column_name='_street')
    op.alter_column('autocomplete_parameter', 'address', new_column_name='_address')

    op.add_column('autocomplete_parameter', sa.Column('admin', source_admin))
    op.add_column('autocomplete_parameter', sa.Column('poi', source_poi))
    op.add_column('autocomplete_parameter', sa.Column('street', source_street))
    op.add_column('autocomplete_parameter', sa.Column('address', source_address))

    # we put some default values for every one
    op.execute("update autocomplete_parameter set admin='OSM', poi='OSM', street='OSM', address='BANO';")

    op.drop_column('autocomplete_parameter', '_admin')
    op.drop_column('autocomplete_parameter', '_poi')
    op.drop_column('autocomplete_parameter', '_street')
    op.drop_column('autocomplete_parameter', '_address')
