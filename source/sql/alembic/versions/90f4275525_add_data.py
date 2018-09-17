"""Add data

Revision ID: 90f4275525
Revises: 35c20e6103d6
Create Date: 2014-09-24 16:57:53.121569

"""

# revision identifiers, used by Alembic.
revision = '90f4275525'
down_revision = '35c20e6103d6'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.execute(
        """
    INSERT INTO navitia.connection_type (id, name) VALUES
    (0, 'StopPoint'), (1, 'StopArea'),
    (2, 'Walking'), (3, 'VJ'), (4, 'Guaranteed'),
    (5, 'Default');
    """
    )
    op.execute(
        """
    INSERT INTO navitia.connection_kind (id, name) VALUES
    (0, 'extension'), (1, 'guarantee'), (2, 'undefined'), (6, 'stay_in');
    """
    )
    op.execute(
        """
    INSERT INTO navitia.odt_type (id, name) VALUES
    (0, 'regular_line'), (1, 'virtual_with_stop_time'),
    (2, 'virtual_without_stop_time'), (3, 'stop_point_to_stop_point'),
    (4, 'adress_to_stop_point'), (5, 'odt_point_to_point');
    """
    )
    op.execute(
        """
    insert into navitia.object_type VALUES
    (0, 'ValidityPattern'),
    (1, 'Line'),
    (2, 'JourneyPattern'),
    (3, 'VehicleJourney'),
    (4, 'StopPoint'),
    (5, 'StopArea'),
    (6, 'Network'),
    (7, 'PhysicalMode'),
    (8, 'CommercialMode'),
    (9, 'Connection'),
    (10, 'JourneyPatternPoint'),
    (11, 'Company'),
    (12, 'Route'),
    (13, 'POI'),
    (15, 'StopPointConnection'),
    (16, 'Contributor'),
    (17, 'StopTime'),
    (18, 'Address'),
    (19, 'Coord'),
    (20, 'Unknown'),
    (21, 'Way'),
    (22, 'Admin'),
    (23, 'POIType');
    """
    )
    op.execute(
        """
        insert into realtime.message_status values (0, 'information'),
        (1, 'warning'),
        (2, 'disrupt');
    """
    )


def downgrade():
    op.execute("delete from navitia.connection_type where id <= 5;")
    op.execute("delete from navitia.connection_kind where id<=2 or id =6;")
    op.execute("delete from navitia.odt_type where id <= 5;")
    op.execute("delete from navitia.object_type where id <= 23;")
    op.execute("delete from realtime.message_status where id <=2;")
