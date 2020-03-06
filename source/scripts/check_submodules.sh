#!/bin/bash

red=`tput setaf 1`
reset=`tput sgr0`
_errors=()

function _add_error() {
	_errors=("${_errors[@]}" "${red}$1")
}

function _check_error() {
	err_num=${#_errors[@]}
	if [ $err_num -gt 0 ]; then
		i=0
		while [[ $i -lt $err_num ]]; do
			echo "${_errors[$i]}"
			((i=i+1))
		done
		echo "${reset}"
		exit 1
	fi
}

submodules=`git submodule | tr -s \  : `  # trim the spaces, and convert them to colon

for submod in $submodules; do
	submod_len=${#submod}
	module=${submod:1:$submod_len} # remove the first character that can be a '+' or '-'

	sha1=`echo $module | cut -d: -f1` # Extract submodule Sha1
	module_path=`echo $module | cut -d: -f2` # Extract submodule path

	head=`cat .git/modules/$module_path/refs/remotes/origin/HEAD` # eg. "ref: refs/remotes/origin/master"
	remote_ref=`echo $head | sed "s%ref: refs/remotes/%%"` # eg. origin/master

    echo "---------------------------"
    echo $module_path
    echo $head
    echo $remote_ref
	pushd $module_path > /dev/null
		# Check whether the current commit contained in the remote HEAD
		contained=`git branch --all --contains HEAD --no-color | grep $remote_ref`
        echo "contained $contained"
		if [ -z "$contained" ]; then
			_add_error "Submodule $module_path points to a commit not merged to its head ! "
			_add_error " > submodule: $module_path"
			_add_error " > commit: $sha1"
			_add_error " > HEAD: $remote_ref"
		fi
	popd > /dev/null
done

_check_error
