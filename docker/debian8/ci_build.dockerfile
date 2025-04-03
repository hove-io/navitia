FROM 162230498103.dkr.ecr.eu-west-1.amazonaws.com/mutable-debian8_dev:latest
WORKDIR /
RUN apt remove --yes cmake &&\
        mkdir local_bin &&\
        cd local_bin &&\
        wget https://cmake.org/files/v3.18/cmake-3.18.6-Linux-x86_64.tar.gz &&\
        tar xf cmake-3.18.6-Linux-x86_64.tar.gz
RUN pip install virtualenv==20.4.7 -U
RUN groupadd docker

ENV PATH=/local_bin/cmake-3.18.6-Linux-x86_64/bin:$PATH
RUN cmake --version
