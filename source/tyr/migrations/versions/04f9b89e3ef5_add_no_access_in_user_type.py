"""Add a new type no_access in user_type

Revision ID: 04f9b89e3ef5
Revises: 75681b9c74aa
Create Date: 2022-12-30 11:23:58.436438

"""

# revision identifiers, used by Alembic.
revision = '04f9b89e3ef5'
down_revision = '75681b9c74aa'

from alembic import op
import sqlalchemy as sa

new_options = ('with_free_instances', 'without_free_instances', 'super_user', 'no_access')
old_type = sa.Enum('with_free_instances', 'without_free_instances', 'super_user', name='user_type')
new_type = sa.Enum(*new_options, name='user_type')
tmp_type = sa.Enum(*new_options, name='_user_type')


def upgrade():
    tmp_type.create(op.get_bind(), checkfirst=False)
    op.execute('ALTER TABLE "user" ALTER COLUMN type DROP DEFAULT')
    op.execute('ALTER TABLE "user" ALTER COLUMN type TYPE _user_type USING type::text::_user_type')
    old_type.drop(op.get_bind(), checkfirst=False)

    new_type.create(op.get_bind(), checkfirst=False)
    op.execute('ALTER TABLE "user" ALTER COLUMN type TYPE user_type USING type::text::user_type')
    op.execute('ALTER TABLE "user" ALTER COLUMN type SET DEFAULT \'with_free_instances\'')
    tmp_type.drop(op.get_bind(), checkfirst=False)
    op.execute('INSERT INTO "user"(login, email, type) values(\'user_without_access\', \'user_without_access@noreply.com\', \'no_access\')')


def downgrade():
    op.execute('DELETE FROM "user" where type = \'no_access\'')
    tmp_type.create(op.get_bind(), checkfirst=False)
    op.execute('ALTER TABLE "user" ALTER COLUMN type DROP DEFAULT')
    op.execute('ALTER TABLE "user" ALTER COLUMN type TYPE _user_type USING type::text::_user_type')
    new_type.drop(op.get_bind(), checkfirst=False)

    old_type.create(op.get_bind(), checkfirst=False)
    op.execute('ALTER TABLE "user" ALTER COLUMN type TYPE user_type USING type::text::user_type')
    op.execute('ALTER TABLE "user" ALTER COLUMN type SET DEFAULT \'with_free_instances\'')
    tmp_type.drop(op.get_bind(), checkfirst=False)
