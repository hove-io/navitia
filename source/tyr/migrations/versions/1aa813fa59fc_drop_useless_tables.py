"""Drop useless tables

Revision ID: 1aa813fa59fc
Revises: 45d7fa0ad882
Create Date: 2014-10-13 14:17:52.013831

"""

# revision identifiers, used by Alembic.
revision = '1aa813fa59fc'
down_revision = '45d7fa0ad882'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.execute("DROP VIEW ptobject CASCADE;")
    op.drop_table('rel_poi_instance')
    op.drop_table('rel_line_instance')
    op.drop_table('rel_admin_instance')
    op.drop_table('rel_route_instance')
    op.drop_table('rel_network_instance')
    op.drop_table('rel_stop_point_instance')
    op.drop_table('rel_stop_area_instance')
    op.drop_table('route')
    op.drop_table('network')
    op.drop_table('line')
    op.drop_table('stop_point')
    op.drop_table('stop_area')
    op.drop_table('admin')
    op.drop_table('poi')


def downgrade():
    op.create_table('rel_stop_area_instance',
    sa.Column('object_id', sa.INTEGER(), autoincrement=False, nullable=False),
    sa.Column('instance_id', sa.INTEGER(), autoincrement=False, nullable=False),
    sa.ForeignKeyConstraint(['instance_id'], [u'instance.id'], name=u'rel_stop_area_instance_instance_id_fkey', ondelete=u'CASCADE'),
    sa.ForeignKeyConstraint(['object_id'], [u'stop_area.id'], name=u'rel_stop_area_instance_object_id_fkey', ondelete=u'CASCADE'),
    sa.PrimaryKeyConstraint('object_id', 'instance_id', name=u'rel_stop_area_instance_pkey')
    )
    op.create_table('admin',
    sa.Column('id', sa.INTEGER(), server_default=sa.text("nextval('admin_id_seq'::regclass)"), nullable=False),
    sa.Column('uri', sa.TEXT(), autoincrement=False, nullable=False),
    sa.Column('name', sa.TEXT(), autoincrement=False, nullable=False),
    sa.Column('external_code', sa.TEXT(), autoincrement=False, nullable=True),
    sa.PrimaryKeyConstraint('id', name=u'admin_pkey'),
    postgresql_ignore_search_path=False
    )
    op.create_table('stop_area',
    sa.Column('id', sa.INTEGER(), nullable=False),
    sa.Column('uri', sa.TEXT(), autoincrement=False, nullable=False),
    sa.Column('name', sa.TEXT(), autoincrement=False, nullable=False),
    sa.Column('external_code', sa.TEXT(), autoincrement=False, nullable=True),
    sa.Column('timezone', sa.VARCHAR(), autoincrement=False, nullable=True),
    sa.PrimaryKeyConstraint('id', name=u'stop_area_pkey')
    )
    op.create_table('stop_point',
    sa.Column('id', sa.INTEGER(), server_default=sa.text("nextval('stop_point_id_seq'::regclass)"), nullable=False),
    sa.Column('uri', sa.TEXT(), autoincrement=False, nullable=False),
    sa.Column('name', sa.TEXT(), autoincrement=False, nullable=False),
    sa.Column('external_code', sa.TEXT(), autoincrement=False, nullable=True),
    sa.PrimaryKeyConstraint('id', name=u'stop_point_pkey'),
    postgresql_ignore_search_path=False
    )
    op.create_table('line',
    sa.Column('id', sa.INTEGER(), server_default=sa.text("nextval('line_id_seq'::regclass)"), nullable=False),
    sa.Column('uri', sa.TEXT(), autoincrement=False, nullable=False),
    sa.Column('name', sa.TEXT(), autoincrement=False, nullable=False),
    sa.Column('external_code', sa.TEXT(), autoincrement=False, nullable=True),
    sa.PrimaryKeyConstraint('id', name=u'line_pkey'),
    postgresql_ignore_search_path=False
    )
    op.create_table('rel_network_instance',
    sa.Column('object_id', sa.INTEGER(), autoincrement=False, nullable=False),
    sa.Column('instance_id', sa.INTEGER(), autoincrement=False, nullable=False),
    sa.ForeignKeyConstraint(['instance_id'], [u'instance.id'], name=u'rel_network_instance_instance_id_fkey', ondelete=u'CASCADE'),
    sa.ForeignKeyConstraint(['object_id'], [u'network.id'], name=u'rel_network_instance_object_id_fkey', ondelete=u'CASCADE'),
    sa.PrimaryKeyConstraint('object_id', 'instance_id', name=u'rel_network_instance_pkey')
    )
    op.create_table('rel_route_instance',
    sa.Column('object_id', sa.INTEGER(), autoincrement=False, nullable=False),
    sa.Column('instance_id', sa.INTEGER(), autoincrement=False, nullable=False),
    sa.ForeignKeyConstraint(['instance_id'], [u'instance.id'], name=u'rel_route_instance_instance_id_fkey', ondelete=u'CASCADE'),
    sa.ForeignKeyConstraint(['object_id'], [u'route.id'], name=u'rel_route_instance_object_id_fkey', ondelete=u'CASCADE'),
    sa.PrimaryKeyConstraint('object_id', 'instance_id', name=u'rel_route_instance_pkey')
    )
    op.create_table('rel_admin_instance',
    sa.Column('object_id', sa.INTEGER(), autoincrement=False, nullable=False),
    sa.Column('instance_id', sa.INTEGER(), autoincrement=False, nullable=False),
    sa.ForeignKeyConstraint(['instance_id'], [u'instance.id'], name=u'rel_admin_instance_instance_id_fkey', ondelete=u'CASCADE'),
    sa.ForeignKeyConstraint(['object_id'], [u'admin.id'], name=u'rel_admin_instance_object_id_fkey', ondelete=u'CASCADE'),
    sa.PrimaryKeyConstraint('object_id', 'instance_id', name=u'rel_admin_instance_pkey')
    )
    op.create_table('rel_line_instance',
    sa.Column('object_id', sa.INTEGER(), autoincrement=False, nullable=False),
    sa.Column('instance_id', sa.INTEGER(), autoincrement=False, nullable=False),
    sa.ForeignKeyConstraint(['instance_id'], [u'instance.id'], name=u'rel_line_instance_instance_id_fkey', ondelete=u'CASCADE'),
    sa.ForeignKeyConstraint(['object_id'], [u'line.id'], name=u'rel_line_instance_object_id_fkey', ondelete=u'CASCADE'),
    sa.PrimaryKeyConstraint('object_id', 'instance_id', name=u'rel_line_instance_pkey')
    )
    op.create_table('rel_poi_instance',
    sa.Column('object_id', sa.INTEGER(), autoincrement=False, nullable=False),
    sa.Column('instance_id', sa.INTEGER(), autoincrement=False, nullable=False),
    sa.ForeignKeyConstraint(['instance_id'], [u'instance.id'], name=u'rel_poi_instance_instance_id_fkey', ondelete=u'CASCADE'),
    sa.ForeignKeyConstraint(['object_id'], [u'poi.id'], name=u'rel_poi_instance_object_id_fkey', ondelete=u'CASCADE'),
    sa.PrimaryKeyConstraint('object_id', 'instance_id', name=u'rel_poi_instance_pkey')
    )
    op.create_table('network',
    sa.Column('id', sa.INTEGER(), nullable=False),
    sa.Column('uri', sa.TEXT(), autoincrement=False, nullable=False),
    sa.Column('name', sa.TEXT(), autoincrement=False, nullable=False),
    sa.Column('external_code', sa.TEXT(), autoincrement=False, nullable=True),
    sa.PrimaryKeyConstraint('id', name=u'network_pkey')
    )
    op.create_table('rel_stop_point_instance',
    sa.Column('object_id', sa.INTEGER(), autoincrement=False, nullable=False),
    sa.Column('instance_id', sa.INTEGER(), autoincrement=False, nullable=False),
    sa.ForeignKeyConstraint(['instance_id'], [u'instance.id'], name=u'rel_stop_point_instance_instance_id_fkey', ondelete=u'CASCADE'),
    sa.ForeignKeyConstraint(['object_id'], [u'stop_point.id'], name=u'rel_stop_point_instance_object_id_fkey', ondelete=u'CASCADE'),
    sa.PrimaryKeyConstraint('object_id', 'instance_id', name=u'rel_stop_point_instance_pkey')
    )
    op.create_table('route',
    sa.Column('id', sa.INTEGER(), nullable=False),
    sa.Column('uri', sa.TEXT(), autoincrement=False, nullable=False),
    sa.Column('name', sa.TEXT(), autoincrement=False, nullable=False),
    sa.Column('external_code', sa.TEXT(), autoincrement=False, nullable=True),
    sa.PrimaryKeyConstraint('id', name=u'route_pkey')
    )
    op.execute("""
        CREATE VIEW ptobject AS
            SELECT id, uri, external_code, name, 'stop_area' as type FROM stop_area UNION
            SELECT id, uri, external_code, name, 'stop_point' as type FROM stop_point UNION
            SELECT id, uri, external_code, name, 'poi' as type FROM poi UNION
            SELECT id, uri, external_code, name, 'admin' as type FROM admin UNION
            SELECT id, uri, external_code, name, 'line' as type FROM line UNION
            SELECT id, uri, external_code, name, 'route' as type FROM route UNION
            SELECT id, uri, external_code, name, 'network' as type FROM network
                      ;""")
    ### end Alembic commands ###
