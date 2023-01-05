FROM navitia/master

# copy package from context inside the docker
COPY navitia-ed_*.deb /
COPY navitia/source/ /navitia/source/

# install navitia-ed package
RUN dpkg -i ./navitia-ed_*.deb || exit 0
RUN apt-get install -f -y

# install eitri requirements
RUN pip install --no-cache-dir -r /navitia/source/eitri/requirements.txt
