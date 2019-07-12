#!/bin/bash


branch=dev
tag_latest=0
push=0
user=''
password=''
components='jormungandr kraken tyr-beat tyr-worker tyr-web instances-configurator mock-kraken'
navitia_local=0
registry="build-tc-data-001:5000"


function show_help() {
    cat << EOF
Usage: ${0##*/} [-lr] [-b branch] [-u user] [-p password]
    -b      git branch to build
    -l      tag images as lastest
    -p      push images to a registry
    -r      registry to push the image to (default=$registry)
    -n      does not update the sources (if the sources have been provided by volume for example)
EOF
}

#we want to be able to interupt the build, see: http://veithen.github.io/2014/11/16/sigterm-propagation.html
function run() {
    trap 'kill -TERM $PID' TERM INT
    $@ &
    PID=$!
    wait $PID
    trap - TERM INT
    wait $PID
    return $?
}

while getopts "lrnb:u:p:" opt; do
    case $opt in
        b)
            branch=$OPTARG
            ;;
        p)
            password=$OPTARG
            ;;
        u)
            user=$OPTARG
            ;;
        n)
            navitia_local=1
            ;;
        l)
            tag_latest=1
            ;;
        p)
            push=1
            ;;
        r)
            registry=$OPTARG
            ;;
        h|\?)
            show_help
            exit 1
            ;;
    esac
done

set -e

#build_dir=/build
navitia_dir=$(pwd)/navitia

if [ $navitia_local -eq 1 ]; then
    echo "Using navitia local path, no update"
else
    echo "building branch $branch"
    pushd $navitia_dir
    run git pull && git checkout $branch && git submodule update --init --recursive
    popd
fi

TARGETS="protobuf_files kraken ed_executables cities integration_tests_bin"
run cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE $navitia_dir/source
run make -j$(nproc) $TARGETS

strip --strip-unneeded tests/*_test kraken/kraken ed/*2ed cities/cities ed/ed2nav

pushd $navitia_dir
version=$(git describe)
echo "building version $version"
popd

#comp_header=navitia
comp_header=mappytia

for component in $components; do
    run docker build -t $comp_header/$component:$version -f  Dockerfile-$component .
        docker tag $comp_header/$component:$version $comp_header/$component:$branch
    if [ $tag_latest -eq 1 ]; then
        docker tag $comp_header/$component:$version $comp_header/$component:latest
    fi
done

if [ $push -eq 1 ]; then
    for component in $components; do
        docker push $registry/$comp_header/$component:$version
        docker push $registry/$comp_header/$component:$branch
    if [ $tag_latest -eq 1 ]; then
        docker push $registry/$comp_header/$component:latest
    fi
    done
fi
