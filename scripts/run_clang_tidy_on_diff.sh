#!/bin/bash

# This script is adapted from here
# https://pspdfkit.com/blog/2018/using-clang-tidy-and-integrating-it-in-jenkins/

# TO CHANGE AND PUT AS ARG
remote_name="origin"

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

# TO CHANGE AND PUT AS ARG
build_dir="build"

# -m specifies that `parallel` should distribute the arguments evenly across the executing jobs.
# -p Tells clang-tidy where to find the `compile_commands.json`.
# `{}` specifies where `parallel` adds the command-line arguments.
# `:::` separates the command `parallel` should execute from the arguments it should pass to the commands.
# `| tee` specifies that we would like the output of clang-tidy to go to `stdout` and also to capture it in
# `$build_dir/clang-tidy-output` for later processing.
# parallel -m clang-tidy -checks='*, -fuchsia-overloaded-operator, -fuchsia-default-arguments,-google*, -cppcoreguidelines-pro-bounds-array-to-pointer-decay, -hicpp-no-array-decay, -readability-implicit-bool-conversion, -misc-macro-parentheses, -clang-diagnostic-unused-command-line-argument' \
# -p $build_dir {} ::: "${modified_filepaths[@]}"

clang-tidy -checks='*, -fuchsia-overloaded-operator, -fuchsia-default-arguments,-google*, -cppcoreguidelines-pro-bounds-array-to-pointer-decay, -hicpp-no-array-decay, -readability-implicit-bool-conversion, -misc-macro-parentheses, -clang-diagnostic-unused-command-line-argument' \
-p $build_dir -warnings-as-errors='*, -fuchsia-overloaded-operator, -fuchsia-default-arguments,-google*, -cppcoreguidelines-pro-bounds-array-to-pointer-decay, -hicpp-no-array-decay, -readability-implicit-bool-conversion, -misc-macro-parentheses, -clang-diagnostic-unused-command-line-argument' \
"${modified_filepaths[@]}"