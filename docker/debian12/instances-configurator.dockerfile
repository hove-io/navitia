FROM python:3.11-slim-bookworm


COPY ./source/sql/alembic /usr/share/navitia/ed/alembic
COPY ./source/sql/requirements.txt /tmp/requirements.txt
COPY ./source/cities /usr/share/navitia/cities
COPY ./docker/templates/* /templates/

COPY docker/instances_configuration.sh /
RUN chmod +x /instances_configuration.sh

RUN apt clean \
    && rm -rf /var/lib/apt/lists/* \
    && apt update --fix-missing \
    && apt-get upgrade -y \
    && apt install -o Acquire::Retries=10 -y libpq5 postgresql-client gettext-base \
    && pip install --no-cache-dir -U -r /tmp/requirements.txt \
    && apt autoremove -y

ENTRYPOINT ["/bin/bash","/instances_configuration.sh"]
