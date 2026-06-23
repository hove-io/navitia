FROM python:3.10-slim-bullseye
WORKDIR /usr/src/app
COPY ./source/navitiacommon ./navitiacommon
COPY ./source/tyr ./tyr
#not sure navitia-proto is used...
COPY ./source/navitia-proto ./navitia-proto
COPY ./docker/ca-certificates/*.crt /usr/local/share/ca-certificates/
COPY ./docker/run_tyr_web.sh /usr/src/app/run.sh

RUN apt clean \
    && apt update --fix-missing \
    && apt install -o Acquire::Retries=10 -y curl libpq5 git ca-certificates libgeos-c1v5 postgresql-client protobuf-compiler gcc \
    && update-ca-certificates \
    && (cd navitia-proto && protoc --python_out=../navitiacommon/navitiacommon type.proto response.proto request.proto task.proto stat.proto) \
    && 2to3 --no-diffs -w ./navitiacommon/navitiacommon \
    && (cd navitiacommon && python setup.py install) \
    && (cd tyr && python setup.py install && pip install --no-cache-dir -U -r requirements.txt)\
    && pip install --no-cache-dir uwsgi==2.0.22 \
    && chmod +x /usr/src/app/run.sh \
    && ln -sf /usr/share/tyr/migrations migrations \
    && ln -sf /usr/share/tyr/manage_tyr.py manage_tyr.py \
    && rm -rf navitia-proto \
    && apt purge -y \
        protobuf-compiler \
        2to3 \
        python3-pip \
        git \
    && apt autoremove -y

ENV REQUESTS_CA_BUNDLE=/etc/ssl/certs

ENTRYPOINT ["bash", "/usr/src/app/run.sh" ]
