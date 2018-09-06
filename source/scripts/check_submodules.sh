#!/bin/bash

red=`tput setaf 1`

submodules=`git submodule | tr -s \  : `  # trim the spaces, and convert them to colon

for submod in $submodules
do
	submod_len=${#submod}
	module=${submod:1:$submod_len} # remove the first character that can be a '+' or '-'

	sha1=`echo $module | cut -d: -f1` # Extract submodule Sha1
	module_path=`echo $module | cut -d: -f2` # Extract submodule path

	head=`cat .git/modules/$module_path/refs/remotes/origin/HEAD` # eg. "ref: refs/remotes/origin/master"
	remote_ref=`echo $head | sed "s%ref: refs/remotes/%%"` # eg. origin/master

	pushd $module_path > /dev/null
		# Check whether the current commit is merged or contained in the remote HEAD
		is_merged=`git branch --all --merged HEAD --no-color | grep $remote_ref`
		contained=`git branch --all --contains HEAD --no-color | grep $remote_ref`
		if [ -z "$is_merged" -a -z "$contained" ]; then
		  echo "${red}A submodule points to a commit not merged to its head ! "
		  echo "${red} > submodule: $module_path"
		  echo "${red} > commit: $sha1"
		  echo "${red} > HEAD: $remote_ref"
		  exit 1
		fi
	popd > /dev/null
done

