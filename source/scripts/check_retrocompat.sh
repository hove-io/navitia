#!/bin/bash

branch_to=$1 # Branch to merge to
pull_request_num=$2 # Github PR number
pull_request_slug=$3 # Git respo destination (eg. hove-io/navitia)
buildbotctp_token=$4 # Github token
github_api_endpoint="https://api.github.com/repos"
warning_msg="## Warning \n You've modified files that potentially affect the model. \
    If so, for compatilility reason, please make sure you've updated \`Data::data_version\` in \`source/type/data.cpp\`."

model_files=(
    "source/type/data.h"
    "source/type/geographical_coord.h"
    "source/type/headsign_handler.h"
    "source/type/message.h"
    "source/type/meta_data.h"
    "source/type/pb_converter.h"
    "source/type/pt_data.h"
    "source/type/time_duration.h"
    "source/type/timezone_manager.h"
    "source/type/type.h"
    "source/type/validity_pattern.h"
)


diff_filenames=`git diff --name-only HEAD^)` # List the updated files
num_model_files=${#model_files[@]}
modified_model_files=()

i=0
while [[ $i -lt $num_model_files ]]; do
    model_file="${model_files[$i]}"

    # Search if the modified file is part of our model_files list
    found=`echo "${diff_filenames[@]}" | grep "$model_file" `
    if [ -n "$found" ]; then
        modified_model_files=("${modified_model_files[@]}" "$model_file")
    fi
    ((i=i+1))
done

num_modified_model_file=${#modified_model_files[@]}
files_list=""
if [ $num_modified_model_file -gt 0 ]; then
    echo "Files from the model have been touched !"
    i=0
    while [[ $i -lt $num_modified_model_file ]]; do
        modified_model_file="${modified_model_files[$i]}"
        echo " - $modified_model_file"
        files_list="$files_list\n- \`$modified_model_file\`"
        ((i=i+1))
    done

    github_add_comment=`printf "%s/%s/issues/%s/comments" $github_api_endpoint $pull_request_slug $pull_request_num`

    echo "$files_list"

    curl -i -X POST \
        -H "Accept:application/vnd.github.v3+json" \
        -H "Authorization:token $buildbotctp_token" \
        -H "Content-Type:application/json" \
        -d "{ \"body\": \"$warning_msg \n\n Files: $files_list\" }" \
        $github_add_comment
fi
