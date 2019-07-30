if [ -f /usr/bin/virtualenvwrapper.sh ]; then
    source /usr/bin/virtualenvwrapper.sh
elif [ -f /usr/share/virtualenvwrapper/virtualenvwrapper.sh ]; then
    source /usr/share/virtualenvwrapper/virtualenvwrapper.sh
fi

PYTHON_ENV_NAME=""

# Ask the question for Python environment name
ask_python_env_name() {
    print_question "How do you wish to name your Python2 working environment ? [navitia]"
    local answer
    read answer
    if [ -z $answer ]; then
        PYTHON_ENV_NAME='navitia'
    else
        PYTHON_ENV_NAME=$answer
    fi
}

# Ask and create Python environment
ask_python_create() {
    if [ -z `command -v mkvirtualenv` ]; then
        print_error "\"mkvirtualenv\" is not available, script is aborting"
        exit
    fi

    print_question "Do you wish to set up a Python2 working environment for Navitia? [Y/n]"
    user_answer
    if [ $? == 1 ]; then
        print_info "Configuring Navitia databases ..."
        python_create_env
    else
        print_info "Navitia databases is not configured"
    fi
}

# Create Python environment
python_create_env() {
    if [ -z $PYTHON_ENV_NAME ]; then
        print_warn "If the virtual environment already exists, it will be destroyed"
        ask_python_env_name
    fi

    rmvirtualenv $PYTHON_ENV_NAME
    mkvirtualenv -p /usr/bin/python2 $PYTHON_ENV_NAME
    pip install -r $NAVITIA_HOME/source/eitri/requirements.txt
    pip install -r $NAVITIA_HOME/source/cities/requirements.txt
    pip install -r $NAVITIA_HOME/source/tyr/requirements_dev.txt
    pip install -r $NAVITIA_HOME/source/monitor/requirements.txt
    pip install -r $NAVITIA_HOME/source/jormungandr/requirements_dev.txt
    pip install -r $NAVITIA_HOME/source/sql/requirements.txt
}
