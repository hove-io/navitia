# Ask and install the packages for the current distribution (for natif).
ask_install_natif_packages() {
    echo "Packages to install: $ALL_NATIF_PACKAGES"
    print_question "Do you wish to install thoses requierements ? [Y/n]"
    user_answer
    if [ $? == 1 ]; then
        print_info "Installing packages ..."
        distrib_natif_install
    else
        print_info "Packages are not installed"
    fi
}

# Ask and install the packages for the current distribution (for Docker).
ask_install_docker_packages() {
    echo "Packages to install: $ALL_DOCKER_PACKAGES"
    print_question "Do you wish to install thoses requierements ? [Y/n]"
    user_answer
    if [ $? == 1 ]; then
        print_info "Installing packages ..."
        distrib_docker_install
    else
        print_info "Packages are not installed"
    fi
}

# Ask and configure Docker
ask_configure_docker() {
    print_question "Do you wish to configure Docker ? [Y/n]"
    user_answer
    if [ $? == 1 ]; then
        print_info "Configuring Docker ..."
        distrib_configure_docker
    else
        print_info "Docker is not configured"
    fi
}

# Ask and configure PostgreSQL
ask_configure_postgresql() {
    print_question "Do you wish to configure PostgreSQL ? [Y/n]"
    user_answer
    if [ $? == 1 ]; then
        print_info "Configuring PostgreSQL ..."
        distrib_configure_postgresql
    else
        print_info "PostgreSQL is not configured"
    fi
}
