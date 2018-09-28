"""multiple comments

Remove all comment column and add 2 new tables: 
    * one comment table
    * one table to make the link between pt object and the comments
Revision ID: 538bc4ea9cd1
Revises: 29fc422c56cb
Create Date: 2015-05-05 11:03:45.982893

"""

# revision identifiers, used by Alembic.
revision = '538bc4ea9cd1'
down_revision = '2c510fae878d'

from alembic import op
import sqlalchemy as sa
import geoalchemy2 as ga


def upgrade():
    op.create_table(
        'comments',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('comment', sa.TEXT(), nullable=False),
        sa.PrimaryKeyConstraint('id'),
        schema='navitia',
    )
    op.create_table(
        'ptobject_comments',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('object_type', sa.TEXT(), nullable=False),
        sa.Column('object_id', sa.BIGINT(), nullable=False),
        sa.Column('comment_id', sa.BIGINT(), nullable=False),
        sa.ForeignKeyConstraint(
            ['comment_id'], [u'navitia.comments.id'], name=u'ptobject_comments_comment_id_fkey'
        ),
        sa.PrimaryKeyConstraint('id'),
        schema='navitia',
    )
    op.drop_column('company', 'comment', schema='navitia')
    op.drop_column('journey_pattern', 'comment', schema='navitia')
    op.drop_column('journey_pattern_point', 'comment', schema='navitia')
    op.drop_column('line', 'comment', schema='navitia')
    op.drop_column('network', 'comment', schema='navitia')
    op.drop_column('route', 'comment', schema='navitia')
    op.drop_column('stop_area', 'comment', schema='navitia')
    op.drop_column('stop_point', 'comment', schema='navitia')
    op.drop_column('stop_time', 'comment', schema='navitia')
    op.drop_column('vehicle_journey', 'comment', schema='navitia')


def downgrade():
    op.add_column('vehicle_journey', sa.Column('comment', sa.TEXT(), nullable=True), schema='navitia')
    op.add_column('stop_time', sa.Column('comment', sa.TEXT(), nullable=True), schema='navitia')
    op.add_column('stop_point', sa.Column('comment', sa.TEXT(), nullable=True), schema='navitia')
    op.add_column('stop_area', sa.Column('comment', sa.TEXT(), nullable=True), schema='navitia')
    op.add_column('route', sa.Column('comment', sa.TEXT(), nullable=True), schema='navitia')
    op.add_column('network', sa.Column('comment', sa.TEXT(), nullable=True), schema='navitia')
    op.add_column('line', sa.Column('comment', sa.TEXT(), nullable=True), schema='navitia')
    op.add_column('journey_pattern_point', sa.Column('comment', sa.TEXT(), nullable=True), schema='navitia')
    op.add_column('journey_pattern', sa.Column('comment', sa.TEXT(), nullable=True), schema='navitia')
    op.add_column('company', sa.Column('comment', sa.TEXT(), nullable=True), schema='navitia')
    op.drop_table('ptobject_comments', schema='navitia')
    op.drop_table('comments', schema='navitia')
