from __future__ import with_statement
from alembic import context
from sqlalchemy import engine_from_config, pool, MetaData, create_engine
from geoalchemy2 import Geography
from logging.config import fileConfig

config = context.config
fileConfig(config.config_file_name)
try:
    from models import metadata as target_metadata
except ImportError:
    print('cannot import metadata, skipping')
    target_metadata = None

def include_object(object_, name, type_, reflected, compare_to):
    if type_ == "table":
        return object_.schema in ["navitia", "georef", "realtime"]
    else:
        return True

def render_item(type_, obj, autogen_context):
    if type_ == "type" and isinstance(obj, Geography):
        return "ga.%r" %obj
    return False

def make_engine():
    cmd_line_url = context.get_x_argument(as_dictionary=True).get('dbname')
    if cmd_line_url:
        engine = create_engine(cmd_line_url)
    else:
        engine = engine_from_config(
                config.get_section(config.config_ini_section),
                prefix='sqlalchemy.',
                poolclass=pool.NullPool)

    return engine

def run_migrations_online():
    """Run migrations in 'online' mode.

    In this scenario we need to create an Engine
    and associate a connection with the context.

    """
    # Note: alembic does not seems to handle multiple schema well
    # it changes the default schema everytime when migrating
    # And thus cannont properly find the alembic version table
    # An UGGLY fix is below, we explictly change the default schema to public
    # We need to create again the engine afterward since the alembic_version table is 
    # searched at the creation
    engine = make_engine()
    r = engine.execute("set search_path to 'public'") 
    r.close()
    engine.dispose()

    engine = make_engine() # second creation with the right default schema

    connection = engine.connect()
    try:
        context.configure(
                connection=connection,
                target_metadata=target_metadata,
                include_schemas=True,
                include_object=include_object,
                render_item=render_item
                )

        with context.begin_transaction():
            context.run_migrations()
    finally:
        connection.close()

if context.is_offline_mode():
    print("we don't handle offline mode")
else:
    run_migrations_online()

