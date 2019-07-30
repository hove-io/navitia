# Ask and install Navitia databases (PostgreSQL format)
ask_db_install() {
    if [ -z `command -v psql` ]; then
        print_error "\"psql\" is not available, script is aborting"
        exit
    fi

    if [ -z `command -v virtualenv` ]; then
        print_error "\"virtualenv\" is not available, script is aborting"
        exit
    fi

    print_question "Do you wish to configure Navitia databases (PostgreSQL format) ? [Y/n]"
    user_answer
    if [ $? == 1 ]; then
        print_info "Configuring Navitia databases ..."
        db_create
    else
        print_info "Navitia databases is not configured"
    fi
}

# Create Navitia databases (PostgreSQL format)
db_create() {
    db_create_navitia
    db_create_jormungandr
}

# Create Navitia database
db_create_navitia() {
    local db_user_password
    db_user_password='navitia'
    local db_name
    db_name='navitia'
    local db_owner
    db_owner="navitia"

    print_info "Creating Navitia user ..."
    local encap
    encap=$(sudo -i -u postgres psql postgres -tAc "SELECT 1 FROM pg_roles WHERE rolname='$db_owner'")  # we check if there is already a user
    if [ -z "$encap" ]; then
        sudo -i -u postgres psql -c "create user $db_owner;alter user $db_owner password '$db_user_password';"
        print_info "Navitia user is created"
    else
        print_warn "User \"$db_owner\" already exists"
    fi

    print_info "Creating Navitia database ..."
    if ! sudo -i -u postgres psql -l | grep -q "^ ${db_name}"; then
        sudo -i -u postgres createdb "$db_name" -O "$db_owner"
        sudo -i -u postgres psql -c "create extension postgis; " "$db_name"
        print_info "Navitia database is created"
    else
        print_warn "Database \"$db_name\" already exists"
    fi

    print_info "Creating temporary virtualenv for migration ..."
    if [ -d /tmp/venv_navitia_sql ]; then
        rm -rf /tmp/venv_navitia_sql
    fi
    virtualenv --python=python2 /tmp/venv_navitia_sql
    . /tmp/venv_navitia_sql/bin/activate
    pip install -r "$NAVITIA_HOME"/source/sql/requirements.txt

    print_info "Migrating Navitia database with \"alembic\" software ..."
    cd "$NAVITIA_HOME"/source/sql
    PYTHONPATH=. alembic -x dbname="postgresql://$db_owner:$db_user_password@localhost/$db_name" upgrade head
    deactivate

    print_info "Cleaning temporary virtualenv ..."
    rm -rf /tmp/venv_navitia_sql

    print_info "Database \"$db_name\" is created with user:\"$db_owner\" and password:\"$db_user_password\""
}

# Create Jormungandr database
db_create_jormungandr() {
    local db_owner
    db_owner="navitia"

    print_info "Creating Jormungandr database ..."
    if ! sudo -i -u postgres psql -l | grep -q "^ jormungandr"; then
        sudo -i -u postgres createdb jormungandr -O "$db_owner"
        sudo -i -u postgres psql -c "create extension postgis; " jormungandr
        print_info "Jormungandr database is created"
    else
        print_warn "Database \"jormungandr\" already exists"
    fi

    print_info "Creating temporary virtualenv for migration ..."
    if [ -d /tmp/venv_tyr ]; then
        rm -rf /tmp/venvtyr
    fi
    virtualenv --python=python2 /tmp/venv_tyr
    . /tmp/venv_tyr/bin/activate
    pip install -r "$NAVITIA_HOME"/source/tyr/requirements_dev.txt

    print_info "Migrating Jormungandr database with \"alembic\" software ..."
    cd "$NAVITIA_HOME"/source/tyr
    PYTHONPATH=.:../navitiacommon/ TYR_CONFIG_FILE=default_settings.py ./manage_tyr.py db upgrade
    cd -
    deactivate

    print_info "Cleaning temporary virtualenv ..."
    rm -rf /tmp/venv_tyr

    print_info "Database \"jormungandr\" is created with user:\"$db_owner\" and no password"
}
