ARG GIT_REVISION="UNKNOWN_VERSION"
FROM debian:bullseye-slim

ARG GIT_REVISION
ARG GITHUB_TOKEN

WORKDIR /usr/src/app

COPY ./source/navitiacommon ./navitiacommon
COPY ./source/jormungandr ./jormungandr
COPY ./source/navitia-proto ./navitia-proto
COPY ./docker/ca-certificates/*.crt /usr/local/share/ca-certificates/
#TODO remove the need for this file
RUN echo "__version__ = '$GIT_REVISION'" > jormungandr/jormungandr/_version.py

RUN apt clean \
    && apt update --fix-missing \
    && apt install -o Acquire::Retries=10 -y curl libpq5 apache2 python3.9-dev python3-pip git libgeos-c1v5 ca-certificates protobuf-compiler 2to3 \
    && update-ca-certificates \
    && (cd navitia-proto && protoc --python_out=../navitiacommon/navitiacommon type.proto response.proto request.proto task.proto stat.proto) \
    && 2to3 --no-diffs -w ./navitiacommon/navitiacommon \
    && (cd navitiacommon && python3 setup.py install) \
    && (cd jormungandr && python3 setup.py install && pip3 install --no-cache-dir -U -r requirements.txt)\
    && pip3 install --no-cache-dir uwsgi==2.0.21 \
    && rm -rf navitiacommon jormungandr navitia-proto \
    && apt purge -y \
        python3-pip \
        2to3 \
        protobuf-compiler

COPY ./docker/run_jormungandr.sh ./run.sh
COPY ./docker/jormungandr.wsgi ./jormungandr.wsgi

# Add apache config
RUN rm /etc/apache2/sites-available/000-default.conf /etc/apache2/conf-available/other-vhosts-access-log.conf
COPY ./docker/apache/jormungandr.default.conf /etc/apache2/sites-available/000-default.conf
COPY ./docker/apache/status.conf /etc/apache2/mods-available/status.conf
COPY ./docker/apache/apache2.conf /etc/apache2/apache2.conf
COPY ./docker/apache/envvars /etc/apache2/envvars
COPY ./docker/apache/logformat.conf /etc/apache2/conf-available/logformat.conf
COPY ./docker/apache/security.conf /etc/apache2/conf-available/security.conf

RUN a2ensite 000-default.conf \
    && a2enmod rewrite \
    && a2enmod proxy \
    && a2enmod proxy_balancer \
    && a2enmod proxy_http \
    && a2enmod headers \
    && a2enconf logformat.conf \
    && a2enmod deflate


# Redirect apache log output to docker log collector
 RUN ln -sf /proc/self/fd/1 /var/log/apache2/access.log \
     && ln -sf /proc/self/fd/2 /var/log/apache2/error.log

# Jormungandr configuration files
RUN git config --global url."https://x-access-token:${GITHUB_TOKEN}@github.com/hove-io/".insteadOf "ssh://git@github.com/hove-io/"
RUN git clone --verbose https://x-access-token:${GITHUB_TOKEN}@github.com/hove-io/corefront-aws-assets

RUN cd corefront-aws-assets && cp -r ./jormungandr /jormungandr && cd .. && rm -rf corefront-aws-assets
RUN apt purge -y \
        git \
    && apt autoremove -y

ENV JORMUNGANDR_BEST_BOARDING_POSITIONS_DIR=/jormungandr/best_boarding_positions/
ENV JORMUNGANDR_ORIGIN_DESTINATION_DIR=/jormungandr/origin_destination_data/
ENV JORMUNGANDR_OLYMPIC_SITE_PARAMS_DIR=/jormungandr/olympic_site_params/

HEALTHCHECK CMD curl -f http://localhost/v1 || exit 1

EXPOSE 80 9091 5050

ENTRYPOINT [ "/usr/src/app/run.sh" ]
