from __future__ import with_statement
from alembic import context
from sqlalchemy import engine_from_config, pool, MetaData
from geoalchemy2 import Geography
from logging.config import fileConfig

config = context.config
fileConfig(config.config_file_name)
try:
    from models import metadata as target_metadata
except ImportError:
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

def run_migrations_offline():
    """Run migrations in 'offline' mode.

    This configures the context with just a URL
    and not an Engine, though an Engine is acceptable
    here as well.  By skipping the Engine creation
    we don't even need a DBAPI to be available.

    Calls to context.execute() here emit the given string to the
    script output.

    """
    url = config.get_main_option("sqlalchemy.url")
    context.configure(url=url)

    with context.begin_transaction():
        context.run_migrations()

def run_migrations_online():
    """Run migrations in 'online' mode.

    In this scenario we need to create an Engine
    and associate a connection with the context.

    """
    engine = engine_from_config(
                config.get_section(config.config_ini_section),
                prefix='sqlalchemy.',
                poolclass=pool.NullPool)

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
    run_migrations_offline()
else:
    run_migrations_online()

