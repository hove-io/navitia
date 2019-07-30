NATIF_AVAILABLE="false"
DOCKER_AVAILABLE="true"

ALL_DOCKER_PACKAGES='docker \
    postgresql \
    postgis \
    python \
    python-pip \
    python-virtualenv \
    python-virtualenvwrapper \
    python2 \
    python2-pip \
    python2-virtualenv'

# Configure Docker
distrib_configure_docker() {
    if [ -z `command -v docker` ]; then
        print_error "\"docker\" is not available, script is aborting"
        exit
    fi

    sudo usermod -aG docker $USER
    sudo systemctl restart docker
    sudo systemctl enable docker
}

# Configure PostgreSQL
distrib_configure_postgresql() {
    if [ -z `command -v psql` ]; then
        print_error "\"psql\" is not available, script is aborting"
        exit
    fi

    # create initial database
    sudo -i -u postgres initdb --locale $LANG -E UTF8 -D '/var/lib/postgres/data/'
    sudo systemctl restart postgresql.service
    sudo systemctl enable postgresql.service
}
