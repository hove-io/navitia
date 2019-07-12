FROM debian:jessie


RUN mkdir build
WORKDIR /build

RUN apt-get update && apt-get upgrade -y
RUN apt-get install -y git g++ cmake libboost-all-dev libzmq3-dev libosmpbf-dev libpqxx3-dev libgoogle-perftools-dev  libprotobuf-dev libproj-dev protobuf-compiler libgeos-c1 liblog4cplus-dev binutils

WORKDIR /build

RUN apt-get install -y apt-transport-https ca-certificates
RUN apt-key adv --keyserver hkp://p80.pool.sks-keyservers.net:80 --recv-keys 58118E89F3A912897C070ADBF76221572C52609D
RUN echo "deb https://apt.dockerproject.org/repo debian-jessie main" >> /etc/apt/sources.list
RUN apt-get update && apt-get install -y docker-engine

COPY Dockerfile-jormungandr Dockerfile-jormungandr
COPY Dockerfile-kraken Dockerfile-kraken
COPY Dockerfile-tyr-beat Dockerfile-tyr-beat
COPY Dockerfile-tyr-worker Dockerfile-tyr-worker
COPY Dockerfile-tyr-web Dockerfile-tyr-web
COPY Dockerfile-instances-configurator Dockerfile-instances-configurator
COPY Dockerfile-mock-kraken Dockerfile-mock-kraken

RUN echo "**/*.a" > .dockerignore
RUN echo "**/CMakeFiles/*" >> .dockerignore
RUN echo "**/*.o" >> .dockerignore
RUN echo ".git" >> .dockerignore
RUN echo "**/CMakeFiles" >> .dockerignore
RUN echo "**/*.cpp" >> .dockerignore
RUN echo "**/*.h" >> .dockerignore
RUN echo "**/test" >> .dockerignore

COPY build.sh build.sh
COPY run_tyr_beat.sh run_tyr_beat.sh
COPY tyr_settings.py tyr_settings.py
COPY instances_configuration.sh instances_configuration.sh
COPY templates templates

ARG BUILD_TYPE=Release
ENV BUILD_TYPE=${BUILD_TYPE}

ENTRYPOINT ["./build.sh"]

