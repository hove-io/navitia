#!/bin/bash

# This script is adapted from here
# https://pspdfkit.com/blog/2018/using-clang-tidy-and-integrating-it-in-jenkins/

usage()
{
cat << EOF
usage: $0 -r <remote_name> -s <absolute_root_sources_dir_path> -b <absolute_build_dir_path>

This script runs clang-tidy-6.0 on all modified files of your actual branch
compared the the dev branch of a remote (preferably, the navitia one)

Note that you must have clang-tidy-6.0 installed and generate
your cmake build with the option -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

OPTIONS:
   -h           Show this message
   -r           Name of the remote you want to compare your actual branch
   -s           Absolute path of your navitia root sources directory
   -b           Absolute path of your navitia build directory

EOF
}

while getopts “hr:s:b:” OPTION
do
     case "$OPTION" in
         h)
             usage
             exit 1
             ;;
         r)
             remote_name="$OPTARG"
             ;;
         s)
             absolute_root_sources_dir_path="$OPTARG"
             ;;
         b)
             absolute_build_dir_path="$OPTARG"
             ;;
     esac
done

if [ -z "$remote_name" ]
then
    echo "no remote_name given, abort"
    usage
    exit 1
fi

if [ -z "$absolute_root_sources_dir_path" ]
then
    echo "no absolute path of your navitia root sources directory given, abort"
    exit 1
fi

if [ -z "$absolute_build_dir_path" ]
then
    echo "no absolute path of your navitia build directory given, abort"
    exit 1
fi

cd $absolute_root_sources_dir_path

# Find the merge base compared to dev.
base=$(git merge-base refs/remotes/$remote_name/dev HEAD)
# Create an empty array that will contain all the filepaths of files modified.
modified_filepaths=()

# To properly handle file names with spaces, we have to do some bash magic.
# We set the Internal Field Separator to nothing and read line by line.
while IFS='' read -r line
do
  # For each line of the git output, we call `realpath` to get the absolute path of the file.
  absolute_filepath=$(realpath "$line")

  # Append the absolute filepath.
  modified_filepaths+=("$absolute_filepath")

# `git diff-tree` outputs all the files that differ between the different commits.
# By specifying `--diff-filter=d`, it doesn't report deleted files.
done < <(git diff-tree --no-commit-id --diff-filter=d --name-only -r "$base" HEAD)

echo "modified files : " ${modified_filepaths[@]}

# -checks Are all the checks that are enabled. Here, we enable everything except all the checks starting with "-"
# -p Tells clang-tidy where to find the `compile_commands.json`.
# -warnings-as-errors All following warnings will be treated as errors. Same format as -checks
clang-tidy-6.0 -checks='*, -fuchsia-overloaded-operator, -fuchsia-default-arguments,-google*, -cppcoreguidelines-pro-bounds-array-to-pointer-decay, -hicpp-no-array-decay, -readability-implicit-bool-conversion, -misc-macro-parentheses, -clang-diagnostic-unused-command-line-argument' \
-p $absolute_build_dir_path -warnings-as-errors='*, -fuchsia-overloaded-operator, -fuchsia-default-arguments,-google*, -cppcoreguidelines-pro-bounds-array-to-pointer-decay, -hicpp-no-array-decay, -readability-implicit-bool-conversion, -misc-macro-parentheses, -clang-diagnostic-unused-command-line-argument' \
"${modified_filepaths[@]}"