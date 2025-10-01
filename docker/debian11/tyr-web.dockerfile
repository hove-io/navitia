FROM debian:bullseye-slim
WORKDIR /usr/src/app
COPY ./docker/ca-certificates/*.crt /usr/local/share/ca-certificates/

RUN apt clean \
    && apt update --fix-missing \
    && apt install -o Acquire::Retries=10 -y curl libpq5 python3.9-dev python3-pip git ca-certificates libgeos-c1v5 postgresql-client protobuf-compiler 2to3 \
    && update-ca-certificates \
    && pip3 install --no-cache-dir -U uv

#not sure navitia-proto is used...
# COPY ./source/navitia-proto ./navitia-proto
# RUN (cd navitia-proto && protoc --python_out=../navitiacommon/navitiacommon type.proto response.proto request.proto task.proto stat.proto) \
COPY ./source/navitiacommon ./navitiacommon
RUN 2to3 --no-diffs -w ./navitiacommon/navitiacommon \
    && (cd navitiacommon && python3 setup.py install) \
    && rm -rf navitia-proto

COPY ./source/tyr ./tyr
RUN (cd tyr && python3 setup.py install && uv pip install --system --no-cache-dir -U -r requirements.txt)\
    && uv pip install --system --no-cache-dir uwsgi==2.0.22 \
    && uv pip install --system --no-cache-dir -r /usr/share/tyr/requirements.txt \
    && ln -sf /usr/share/tyr/migrations migrations \
    && ln -s /usr/bin/python3.9 /usr/bin/python \
    && ln -sf /usr/share/tyr/manage_tyr.py manage_tyr.py

RUN apt purge -y \
        protobuf-compiler \
        2to3 \
        python3-pip \
        git \
    && apt autoremove -y

ENV REQUESTS_CA_BUNDLE=/etc/ssl/certs
COPY ./source/tyr ./tyr
COPY --chmod=755 ./docker/run_tyr_web.sh /usr/src/app/run.sh
ENTRYPOINT ["bash", "/usr/src/app/run.sh" ]
