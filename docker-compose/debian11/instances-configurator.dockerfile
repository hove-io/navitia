FROM debian:bullseye-slim

COPY ./source/sql/requirements.txt /tmp/requirements.txt

RUN apt clean \
    && apt update --fix-missing \
    && apt install -o Acquire::Retries=10 -y libpq5 python3.9-dev python3-pip postgresql-client gettext-base \
    && pip3 install --no-cache-dir -U -r /tmp/requirements.txt \
    && apt autoremove -y

COPY ./source/sql/alembic /usr/share/navitia/ed/alembic
COPY ./source/cities /usr/share/navitia/cities
COPY ./docker/templates/* /templates/

COPY --chmod=755 docker/instances_configuration.sh /instances_configuration.sh

ENTRYPOINT ["/bin/bash","/instances_configuration.sh"]
