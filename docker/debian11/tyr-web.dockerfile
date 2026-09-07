FROM python:3.11-slim-bullseye
WORKDIR /usr/src/app
COPY ./source/navitiacommon ./navitiacommon
COPY ./source/tyr ./tyr
#not sure navitia-proto is used...
COPY ./source/navitia-proto ./navitia-proto
COPY ./docker/ca-certificates/*.crt /usr/local/share/ca-certificates/
COPY ./docker/run_tyr_web.sh /usr/src/app/run.sh

RUN apt clean \
    && rm -rf /var/lib/apt/lists/* \
	&& printf 'deb [check-valid-until=no] http://archive.debian.org/debian/ bullseye main\ndeb [check-valid-until=no] http://archive.debian.org/debian/ bullseye-updates main\ndeb [check-valid-until=no] http://snapshot.debian.org/archive/debian-security/20260903T000000Z bullseye-security main\n' > /etc/apt/sources.list \
	&& apt-get -o Acquire::Check-Valid-Until=false update -y \
    && apt install -o Acquire::Retries=10 -y curl libpq5 git ca-certificates libgeos-c1v5 postgresql-client gcc \
    && update-ca-certificates \
    && curl -fsSL -o /tmp/protoc-python.zip https://github.com/protocolbuffers/protobuf/releases/download/v29.5/protoc-29.5-linux-x86_64.zip \
    && python3 -m zipfile -e /tmp/protoc-python.zip /tmp/protoc-python && install -Dm0755 /tmp/protoc-python/bin/protoc /usr/local/bin/protoc-python \
    && (cd navitia-proto && protoc-python --python_out=../navitiacommon/navitiacommon type.proto response.proto request.proto task.proto stat.proto) \
    && 2to3 --no-diffs -w navitiacommon/navitiacommon/*_pb2.py \
    && (cd navitiacommon && python setup.py install) \
    && (cd tyr && python setup.py install && pip install --no-cache-dir -U -r requirements.txt)\
    && pip install --no-cache-dir uwsgi==2.0.22 \
    && chmod +x /usr/src/app/run.sh \
    && ln -sf /usr/share/tyr/migrations migrations \
    && ln -sf /usr/share/tyr/manage_tyr.py manage_tyr.py \
    && rm -rf navitia-proto /usr/local/bin/protoc-python \
    && apt purge -y \
        python3-pip \
        git \
    && apt autoremove -y

ENV REQUESTS_CA_BUNDLE=/etc/ssl/certs

ENTRYPOINT ["bash", "/usr/src/app/run.sh" ]
