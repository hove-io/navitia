NAVITIA_HOME=`dirname $(dirname $(readlink -f $0))`

# Ask if the shell should be configured.
ask_configure_shell() {
    print_question "Do you wish to setup your Shell ? [Y/n]"
    user_answer
    if [ $? == 1 ]; then
        print_info "Setting up the Shell ..."
        which_shell
    else
        print_info "Shell is not set up"
    fi
}

# Ask which shell should be configured.
which_shell() {
    print_question "Which Shell do you which to setup? [bash/zsh]"

    local answer
    while read answer; do
        case $answer in
            'bash')
                source ./dev_env/1_1_bash.sh
                break
                ;;

            'zsh')
                source ./dev_env/1_1_zsh.sh
                break
                ;;

            *)
                echo "Shell '$answer' is unknown, try again"
                echo "(you can contribute to add your Shell to the script, check \"dev_env\" directory)"
                ;;
        esac
    done

    ask_build_path
    ask_python_env_name
    shell_setup
}
