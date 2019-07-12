FROM alpine:3.4

RUN apk --update --no-cache add \
        bash \
        gettext \
        py-pip \
        postgresql-dev \
        musl-dev \
        gcc \
        python-dev \
    && \
    pip install \
        alembic \
        geoalchemy2 \
        psycopg2 \
    && \
# we can remove the dependencies used to build psycopg2
    apk del \
        postgresql-dev \
        musl-dev \
        gcc \
    && \
# there is a conflict between postgresql-dev and postgresql-bdr, so it's installed after
    apk add postgresql-bdr

COPY instances_configuration.sh /
COPY templates/* /templates/
COPY navitia/source/sql/alembic /usr/share/navitia/ed/alembic
COPY navitia/source/cities /usr/share/navitia/cities

CMD sh -x /instances_configuration.sh
