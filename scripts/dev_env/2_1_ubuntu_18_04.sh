NATIF_AVAILABLE="true"
DOCKER_AVAILABLE="true"

ALL_NATIF_PACKAGES='build-essential
    clang-format-6.0
    cmake
    curl
    libboost-all-dev
    libgeos-dev
    libgoogle-perftools-dev
    liblog4cplus-dev
    libosmpbf-dev
    libpqxx-dev
    libproj-dev
    libprotobuf-dev
    libssl-dev
    libzmq3-dev
    postgis
    postgresql
    protobuf-compiler
    python
    python-pip
    python-virtualenv
    rabbitmq-server
    virtualenvwrapper'

ALL_DOCKER_PACKAGES='docker.io
    postgis
    postgresql
    python
    python-pip
    python-virtualenv
    virtualenvwrapper'

# Install all packages (for natif)
distrib_natif_install() {
    sudo apt-get install -y $ALL_NATIF_PACKAGES
}

# Install all packages (for Docker)
distrib_docker_install() {
    sudo apt-get install -y $ALL_DOCKER_PACKAGES
}

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
    echo "Nothing to do"
}
