FROM python:3.11-slim-bullseye


COPY ./source/sql/alembic /usr/share/navitia/ed/alembic
COPY ./source/sql/requirements.txt /tmp/requirements.txt
COPY ./source/cities /usr/share/navitia/cities
COPY ./docker/templates/* /templates/

COPY docker/instances_configuration.sh /
RUN chmod +x /instances_configuration.sh

RUN apt clean \
    && rm -rf /var/lib/apt/lists/* \
	&& printf 'deb [check-valid-until=no] http://archive.debian.org/debian/ bullseye main\ndeb [check-valid-until=no] http://archive.debian.org/debian/ bullseye-updates main\ndeb [check-valid-until=no] http://snapshot.debian.org/archive/debian-security/20260903T000000Z bullseye-security main\n' > /etc/apt/sources.list \
	&& apt-get -o Acquire::Check-Valid-Until=false update -y \
    && apt install -o Acquire::Retries=10 -y libpq5 postgresql-client gettext-base \
    && pip install --no-cache-dir -U -r /tmp/requirements.txt \
    && apt autoremove -y

ENTRYPOINT ["/bin/bash","/instances_configuration.sh"]
