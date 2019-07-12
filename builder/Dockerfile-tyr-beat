FROM navitia/python

WORKDIR /usr/src/app
COPY navitia/source/tyr/requirements.txt /usr/src/app
COPY navitia/source/tyr/tyr /usr/src/app/tyr
COPY navitia/source/tyr/manage_tyr.py /usr/src/app/
COPY navitia/source/tyr/migrations /usr/src/app/migrations
COPY navitia/source/navitiacommon/navitiacommon /usr/src/app/navitiacommon

RUN echo @testing http://dl-cdn.alpinelinux.org/alpine/edge/testing >> /etc/apk/repositories

RUN apk --update --no-cache add \
        git \
        python-dev \
        postgresql-dev \
        postgresql-client \
        gcc \
        geos@testing \
        musl-dev \
        python-dev \
    && pip install --no-cache-dir -r requirements.txt \
    && apk --no-cache del \
            python-dev \
            git \
            gcc

COPY tyr_settings.py /srv/navitia/settings.py

COPY run_tyr_beat.sh /
RUN chmod +x /run_tyr_beat.sh

CMD ["sh", "/run_tyr_beat.sh"]
