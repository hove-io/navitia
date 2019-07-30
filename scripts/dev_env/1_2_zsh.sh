# Setup the "~/.zshrc" file
shell_setup() {
    cat >>~/.bashrc <<END

# ----------> KISIO DIGITAL <----------

export NAVITIA_HOME=$NAVITIA_HOME
export NAVITIA_BUILD=$NAVITIA_BUILD

# Update Python virtual environment
navitia_env_update() {
    rmvirtualenv $PYTHON_ENV_NAME
    mkvirtualenv -p /usr/bin/python2 $PYTHON_ENV_NAME
    pip install -r \$NAVITIA_HOME/source/eitri/requirements.txt
    pip install -r \$NAVITIA_HOME/source/cities/requirements.txt
    pip install -r \$NAVITIA_HOME/source/tyr/requirements_dev.txt
    pip install -r \$NAVITIA_HOME/source/monitor/requirements.txt
    pip install -r \$NAVITIA_HOME/source/jormungandr/requirements_dev.txt
    pip install -r \$NAVITIA_HOME/source/sql/requirements.txt
}

alias -s eitri='\$NAVITIA_HOME/source/eitri/eitri.py -e \$NAVITIA_BUILD/ed/'
alias -s docker_update='docker pull navitia/debian8_dev'

# Run a command in the development Docker
docker_navitia() {
    if [ \$# == 0 ]; then
        echo "docker_navitia: error, need at least 1 parameter (the command to execute in the docker)"
        return -1
    fi

    docker run -it --rm --user=$(id -u):$(id -g) -v \$NAVITIA_HOME:/navitia/source -v \$NAVITIA_BUILD:/navitia/build -v \$HOME/.zshrc:/.zshrc -w /navitia navitia/debian8_dev \$@
}

# ----------> END KISIO DIGITAL <----------
END

    print_info "Your \"~/.zshrc\" file is modified"
}
