from __future__ import with_statement
from alembic import context
from sqlalchemy import engine_from_config, pool, MetaData
from logging.config import fileConfig

# this is the Alembic Config object, which provides
# access to the values within the .ini file in use.
config = context.config
from models import metadata as target_metadata

# Interpret the config file for Python logging.
# This line sets up loggers basically.
fileConfig(config.config_file_name)
from geoalchemy2 import Geography

# other values from the config, defined by the needs of env.py,
# can be acquired:
# my_important_option = config.get_main_option("my_important_option")
# ... etc.

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

