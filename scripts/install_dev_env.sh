#!/bin/bash

# ********************************************************************
# Complete development environment installation for Navitia.
# After the installation, you'll be able to :
# - Compile Kraken
# - Run all integration and unitary tests
# - Run Docker (Tyr + Eitri) tests
# - Run binaries to fill ed database (fusio2ed, osm2ed, ...)
# - Run ed2nav binary to generate data.nav.lz4 from the ed database
# - Run Eitri (the Docker method) to build data.nav.lz4 with your own NTFS and Osm files
# ********************************************************************

source ./dev_env/0_utils.sh
source ./dev_env/1_shell_variables.sh
source ./dev_env/2_install_packages.sh
source ./dev_env/3_build.sh
source ./dev_env/4_database_postgresql.sh
source ./dev_env/5_python_env.sh


# entry point

DISTRIBUTION=`grep PRETTY_NAME /etc/os-release | sed 's/^PRETTY_NAME="\(.*\)".*/\1/'`
case $DISTRIBUTION in
    'Ubuntu 18.04')
        source ./dev_env/2_1_ubuntu_18_04.sh
        ;;

    'Ubuntu 19.04')
        source ./dev_env/2_1_ubuntu_19_04.sh
        ;;

    'Manjaro Linux')
        source ./dev_env/2_1_manjaro.sh
        ;;

    *)
        print_error "Distribution '$DISTRIBUTION' is unknown, script is aborting"
        echo "(you can contribute to add your distribution to the script, check \"dev_env\" directory)"
        exit
        ;;
esac

print_info "Launching Navitia development environment installation ...\n"

echo "--------------------------------------------------"
echo "| architecture: `uname -m`"
echo "| kernel familly: `uname -s`"
echo "| kernel release: `uname -r`"
echo "| operating system: `uname -o`"
echo "| distribution: $DISTRIBUTION"
echo -e "--------------------------------------------------\n"

print_warn "All questions are optionnals and independants ! You can drop any part of the script."
print_warn "The only exception is the step 3 that is mandatory for the step 4 (at least, a build is necessary to setup the database)."

print_info "\n1. Shell (append text configuration at the end of your configuration file)"
ask_configure_shell

print_info "\n2. Packages (install and configure packages for your distribution)"
if [ $DOCKER_AVAILABLE = "true" ] && [ $NATIF_AVAILABLE = "false" ]; then
    print_info "Your distribution doesn't support natif build"
elif [ $DOCKER_AVAILABLE = "false" ] && [ $NATIF_AVAILABLE = "true" ]; then
    print_info "Your distribution doesn't support docker"
elif [ $DOCKER_AVAILABLE = "true" ] && [ $NATIF_AVAILABLE = "true" ]; then
    print_info "Your distribution support natif build and docker"
    print_question "Which installation do you want? [natif/docker/both]"
    while read answer; do
        case $answer in
            'natif')
                ask_install_natif_packages
                break
                ;;

            'docker')
                ask_install_docker_packages
                ask_configure_docker
                break
                ;;

            'both')
                ask_install_natif_packages
                ask_install_docker_packages
                ask_configure_docker
                break
                ;;

            *)
                echo >&2 "invalid anwser '$answer', try again ..."
                ;;
        esac
    done
else
    print_error "Neither a natif or a Docker build is available, aborting ..."
    exit
fi
ask_configure_postgresql

print_info "\n3. Build (necessary for the next step)"
ask_build_all

print_info "\n4. PostgreSQL (create database for Navitia)"
ask_db_install

print_info "\n5. Python environment"
ask_python_create

print_info "\nInstallation is finished !"
echo ""
echo "                     WELCOME TO"
echo ""
echo "##    ##    ###    ##     ## #### ######## ####    ###"
echo "###   ##   ## ##   ##     ##  ##     ##     ##    ## ##"
echo "####  ##  ##   ##  ##     ##  ##     ##     ##   ##   ##"
echo "## ## ## ##     ## ##     ##  ##     ##     ##  ##     ##"
echo "##  #### #########  ##   ##   ##     ##     ##  #########"
echo "##   ### ##     ##   ## ##    ##     ##     ##  ##     ##"
echo "##    ## ##     ##    ###    ####    ##    #### ##     ##"
echo ""
echo "                        ___"
echo "                     .-'   \`'."
echo "                    /         \\"
echo "                    |         ;"
echo "                    |         |           ___.--,"
echo "           _.._     |0) ~ (0) |    _.---'\`__.-( (_."
echo "    __.--'\`_.. '.__.\    '--. \_.-' ,.--'\`     \`\"\"\`"
echo "   ( ,.--'\`   ',__ /./;   ;, '.__.'\`    __"
echo "   _\`) )  .---.__.' / |   |\   \__..--\"\"  \"\"\"--.,_"
echo "  \`---' .'.''-._.-'\`_./  /\ '.  \ _.-~~~\`\`\`\`~~~-._\`-.__.'"
echo "        | |  .' _.-' |  |  \  \  '.               \`~---\`"
echo "         \ \/ .'     \  \   '. '-._)"
echo "          \/ /        \  \    \`=.__\`~-."
echo "          / /\         \`) )    / / \`\"\".\`"
echo "    , _.-'.'\ \        / /    ( (     / /"
echo "     \`--~\`   ) )    .-'.'      '.'.  | ("
echo "            (/\`    ( (\`          ) )  '-;"
echo "             \`      '-;         (-'"
echo ""
