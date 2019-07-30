# Display a text for information.
#
# param1 : the text to display
print_info() {
    echo -e "\e[32m$1\e[39m"
}

# Display a text for warning.
#
# param1 : the text to display
print_warn() {
    echo -e "\e[33m$1\e[39m"
}

# Display a text for error.
#
# param1 : the text to display
print_error() {
    echo -e "\e[31m$1\e[39m"
}

# Display a text for question.
#
# param1 : the text to display
print_question() {
    echo -ne "\e[1m$1\e[24m "
}

# Check the answer of the user. Only 'y', 'Y', 'yes', 'n', 'N' and 'no' are valid answer. While the answer is invalid,
# the prompt will ask the user.
#
# return: 1 if yes, otherwise 0
user_answer() {
    local answer
    while read answer; do
        case $answer in
            ''|'y'|'Y'|'yes')
                return 1
                ;;

            'n'|'N'|'no')
                return 0
                ;;

            *)
                echo >&2 "invalid anwser '$answer', try again ..."
                ;;
        esac
    done
}
