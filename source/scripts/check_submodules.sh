#!/bin/bash

red=`tput setaf 1`

submodules=`git submodule | tr -s \  : `  # trim the spaces, and convert them to colon

echo "CI : $CI"
echo "TRAVIS : $TRAVIS"
echo "CONTINUOUS_INTEGRATION : $CONTINUOUS_INTEGRATION"
echo "DEBIAN_FRONTEND : $DEBIAN_FRONTEND"
echo "HAS_JOSH_K_SEAL_OF_APPROVAL : $HAS_JOSH_K_SEAL_OF_APPROVAL"
echo "USER : $USER"
echo "HOME : $HOME"
echo "LANG : $LANG"
echo "LC_ALL : $LC_ALL"
echo "RAILS_ENV : $RAILS_ENV"
echo "RACK_ENV : $RACK_ENV"
echo "MERB_ENV : $MERB_ENV"
echo "JRUBY_OPTS : $JRUBY_OPTS"
echo "JAVA_HOME : $JAVA_HOME"
echo "TRAVIS_ALLOW_FAILURE : $TRAVIS_ALLOW_FAILURE"
echo "TRAVIS_BRANCH : $TRAVIS_BRANCH"
echo "TRAVIS_BUILD_DIR : $TRAVIS_BUILD_DIR"
echo "TRAVIS_BUILD_ID : $TRAVIS_BUILD_ID"
echo "TRAVIS_BUILD_NUMBER : $TRAVIS_BUILD_NUMBER"
echo "TRAVIS_COMMIT : $TRAVIS_COMMIT"
echo "TRAVIS_COMMIT_MESSAGE : $TRAVIS_COMMIT_MESSAGE"
echo "TRAVIS_COMMIT_RANGE : $TRAVIS_COMMIT_RANGE"
echo "TRAVIS_EVENT_TYPE : $TRAVIS_EVENT_TYPE"
echo "TRAVIS_JOB_ID : $TRAVIS_JOB_ID"
echo "TRAVIS_JOB_NUMBER : $TRAVIS_JOB_NUMBER"
echo "TRAVIS_OS_NAME : $TRAVIS_OS_NAME"
echo "TRAVIS_OSX_IMAGE : $TRAVIS_OSX_IMAGE"
echo "TRAVIS_PULL_REQUEST : $TRAVIS_PULL_REQUEST"
echo "TRAVIS_PULL_REQUEST_BRANCH : $TRAVIS_PULL_REQUEST_BRANCH"
echo "TRAVIS_PULL_REQUEST_SHA : $TRAVIS_PULL_REQUEST_SHA"
echo "TRAVIS_PULL_REQUEST_SLUG : $TRAVIS_PULL_REQUEST_SLUG"
echo "TRAVIS_SECURE_ENV_VARS : $TRAVIS_SECURE_ENV_VARS"
echo "TRAVIS_TEST_RESULT : $TRAVIS_TEST_RESULT"
echo "TRAVIS_TAG : $TRAVIS_TAG"
echo "TRAVIS_BUILD_STAGE_NAME : $TRAVIS_BUILD_STAGE_NAME"
echo "TRAVIS_DART_VERSION : $TRAVIS_DART_VERSION"
echo "TRAVIS_GO_VERSION : $TRAVIS_GO_VERSION"
echo "TRAVIS_HAXE_VERSION : $TRAVIS_HAXE_VERSION"
echo "TRAVIS_JDK_VERSION : $TRAVIS_JDK_VERSION"
echo "TRAVIS_JULIA_VERSION : $TRAVIS_JULIA_VERSION"
echo "TRAVIS_NODE_VERSION : $TRAVIS_NODE_VERSION"
echo "TRAVIS_OTP_RELEASE : $TRAVIS_OTP_RELEASE"
echo "TRAVIS_PERL_VERSION : $TRAVIS_PERL_VERSION"
echo "TRAVIS_PHP_VERSION : $TRAVIS_PHP_VERSION"
echo "TRAVIS_PYTHON_VERSION : $TRAVIS_PYTHON_VERSION"
echo "TRAVIS_R_VERSION : $TRAVIS_R_VERSION"
echo "TRAVIS_RUBY_VERSION : $TRAVIS_RUBY_VERSION"
echo "TRAVIS_RUST_VERSION : $TRAVIS_RUST_VERSION"
echo "TRAVIS_SCALA_VERSION : $TRAVIS_SCALA_VERSION"
echo "TRAVIS_MARIADB_VERSION : $TRAVIS_MARIADB_VERSION"
echo "TRAVIS_XCODE_SDK : $TRAVIS_XCODE_SDK"
echo "TRAVIS_XCODE_SCHEME : $TRAVIS_XCODE_SCHEME"
echo "TRAVIS_XCODE_PROJECT : $TRAVIS_XCODE_PROJECT"
echo "TRAVIS_XCODE_WORKSPACE : $TRAVIS_XCODE_WORKSPACE"

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

