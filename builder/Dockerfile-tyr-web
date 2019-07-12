FROM navitia/python

WORKDIR /usr/src/app
COPY navitia/source/tyr/requirements.txt /usr/src/app
COPY navitia/source/tyr/tyr /usr/src/app/tyr
COPY navitia/source/tyr/manage_tyr.py /usr/src/app/
COPY navitia/source/navitiacommon/navitiacommon /usr/src/app/navitiacommon

RUN echo @testing http://dl-cdn.alpinelinux.org/alpine/edge/testing >> /etc/apk/repositories

RUN apk --update --no-cache add \
        git \
        python-dev \
        build-base \
        linux-headers \
        postgresql-dev \
        gcc \
        geos@testing \
        musl-dev \
        python-dev \
    && pip install --no-cache-dir -r requirements.txt \
    && apk --no-cache del \
            python-dev \
            git \
            build-base \
            linux-headers \
            gcc

COPY tyr_settings.py /srv/navitia/settings.py

ENV TYR_CONFIG_FILE=/srv/navitia/settings.py
ENV PYTHONPATH=.:../navitiacommon
CMD ["uwsgi",  "--mount", "/=tyr:app",  "--http", "0.0.0.0:80"]
