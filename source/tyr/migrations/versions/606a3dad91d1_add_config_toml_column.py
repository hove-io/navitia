"""Add config_toml column

Revision ID: 606a3dad91d1
Revises: 9d7151d79dc3
Create Date: 2020-11-27 09:20:28.907940

"""

from alembic import op
import sqlalchemy as sa
import json

revision = '606a3dad91d1'
down_revision = '9d7151d79dc3'


def upgrade():
    op.add_column('autocomplete_parameter', sa.Column('config_toml', sa.Text(), nullable=True))
    # config_toml must contain the configuration of osm2mimir which looks like :
    # https://github.com/hove-io/mimirsbrunn/pull/436/files#diff-e90069baea048406777c4cc75d75c1c655ad2729ffec59108a1f30f2517f7d28
    # Create config_toml from old params
    connection = op.get_bind()

    result = connection.execute(
        'select id, name, street, address, poi, '
        'admin, admin_level, poi_types_json from autocomplete_parameter'
    )
    for row in result:
        config_toml = ""

        # DataSet
        config_toml += 'dataset = "{}"\n\n'.format(row["name"])

        # Administrative regions
        config_toml += "[admin]\n"
        tmp = "{}".format(row['admin'] == "OSM")
        config_toml += "import = {}\n".format(tmp.lower())
        config_toml += "city_level = 8\n"
        config_toml += "levels = {}\n\n".format(row['admin_level'])

        # Streets
        config_toml += "[street]\n"
        tmp = "{}".format(row['street'] == "OSM")
        config_toml += "import = {}\n\n".format(tmp.lower())

        # Pois
        config_toml += "[poi]\n"
        tmp = "{}".format(row['poi'] == "OSM")
        config_toml += "import = {}\n".format(tmp.lower())

        poi_types_json = row["poi_types_json"]
        if poi_types_json:
            poi_types_json = json.loads(poi_types_json)
            poi_types = poi_types_json.get("poi_types", [])
            rules = poi_types_json.get("rules", [])
            if poi_types and rules:
                config_toml += "[poi.config]\n"
                for poi_type in poi_types:
                    config_toml += "[[poi.config.types]]\n"
                    config_toml += 'id = "{}"\n'.format(poi_type["id"])
                    name = poi_type["name"].encode("utf-8")
                    name = name.replace("'", "''")
                    config_toml += 'name = "{}"\n'.format(name)

                for rule in rules:
                    poi_type_id = rule["poi_type_id"]
                    osm_tags_filters = rule["osm_tags_filters"]
                    for osm_tags_filter in osm_tags_filters:
                        config_toml += "[[poi.config.rules]]\n"
                        config_toml += 'type = "poi_type:{}"\n'.format(poi_type_id)
                        config_toml += '[[poi.config.rules.osm_tags_filters]]\n'
                        config_toml += 'key = "{}"\n'.format(osm_tags_filter["key"])
                        config_toml += 'value = "{}"\n'.format(osm_tags_filter["value"])
        query = "update autocomplete_parameter set config_toml='{}' where id={}".format(config_toml, row["id"])
        op.execute(query.decode("utf-8"))


def downgrade():
    op.drop_column('autocomplete_parameter', 'config_toml')
