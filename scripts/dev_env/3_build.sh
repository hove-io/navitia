NAVITIA_BUILD=""

# Ask the question for build directory path
ask_build_path() {
    print_question "What is your build directory? (will be created if needed)"
    local answer
    read answer
    if [ ! -d $answer ]; then
        print_info "Folder doesn't exist, making it ..."
        mkdir -p $answer
    fi
    NAVITIA_BUILD=$answer
}

# Ask to build all binaries
ask_build_all() {
    print_question "Do you wish to build Navitia binaries ? [Y/n]"
    user_answer
    if [ $? == 1 ]; then
        print_info "Building Navitia binaries ..."
        build_all
    else
        print_info "Navitia binaries are not built"
    fi
}

# Ask the diretcory to build in then build all
build_all() {
    if [ -z $NAVITIA_BUILD ]; then
        ask_build_path
    fi

    if [ $NATIF_AVAILABLE = "true" ]; then
        build_natif
    elif [ $DOCKER_AVAILABLE = "true" ]; then
        build_docker
    else
        print_error "Neither a natif or a Docker build is available, aborting ..."
        exit
    fi
}

# Do a natif build
build_natif() {
    local nbproc
    nbproc=`nproc`

    cd $NAVITIA_BUILD
    cmake $NAVITIA_HOME/source
    make all -j$nbproc -l$nbproc
    cd -
}

# Do a Docker build
build_docker() {
    local nbproc
    nbproc=`nproc`

    docker pull navitia/debian8_dev
    docker run -it --rm --user=$(id -u):$(id -g) -v $NAVITIA_HOME:/navitia/repo -v $NAVITIA_BUILD:/navitia/build -w /navitia/build navitia/debian8_dev /bin/bash -c "cmake ../repo/source && make all -j$nbproc -l$nbproc"
}
