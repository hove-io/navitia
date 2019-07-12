FROM navitia/python

WORKDIR /usr/src/app
COPY navitia/source/jormungandr/requirements.txt /usr/src/app
COPY navitia/source/jormungandr/jormungandr /usr/src/app/jormungandr
COPY navitia/source/navitiacommon/navitiacommon /usr/src/app/navitiacommon

RUN echo @testing http://dl-cdn.alpinelinux.org/alpine/edge/testing >> /etc/apk/repositories

RUN apk --update --no-cache add \
        g++ \
        build-base \
        python-dev \
        git \
        geos@testing \
        postgresql-dev \
        libxslt \
        libxml2-dev \
        libxslt-dev \
        libpq \
        curl && \
    pip install --no-cache-dir -r requirements.txt && \
    apk --no-cache del \
        g++ \
        build-base \
        python-dev \
        zlib-dev \
        musl-dev \
        postgresql-dev \
        libxml2-dev \
        libxslt-dev

# ....
# I don't see a better way, geos try to find libc and fail, so ugly hack to give it
RUN ln -s /lib/ld-musl-x86_64.so.1 /usr/lib/libc.so

WORKDIR /usr/src/app/

HEALTHCHECK CMD curl -f http://localhost/v1/coverage || exit 1
ENV PYTHONPATH=.
ENV PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION=cpp
ENV PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION_VERSION=2
CMD ["uwsgi", "--master", "--lazy-apps", "--mount", "/=jormungandr:app", "--http", "0.0.0.0:80"]
