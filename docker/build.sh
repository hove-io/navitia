#!/bin/bash

# exit this script with failure on any error
set -e

function show_help() {
    cat << EOF
Usage: ${0##*/} -e event -b branch -d debian_version -o oauth_token (-f pull_request_fork) [-t tag] [-r -u dockerhub_user -p dockerhub_password]
    -e      [push|pull_request]
    -b      [dev|release] if -e push, or the branch name if -e pull_request
    -f      pull_request fork owner, needed only if -e pull_request
    -o      oauth token for github
    -t      tag images with the given string
    -r      push images to a registry
    -u      username for authentication on dockerhub
    -p      password for authentication on dockerhub
    -n      build from Navitia CI, in this case, no packages need to be downloaded from github actions, default False
    -d      [debian8|debian10]
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


while getopts "o:t:b:rp:u:e:f:d:nh" opt; do
    case $opt in
        o) token=$OPTARG
            ;;
        t) tag=$OPTARG
            ;;
        b) branch=$OPTARG
            ;;
        r) push=1
            ;;
        p) password=$OPTARG
            ;;
        u) user=$OPTARG
            ;;
        e) event=$OPTARG
            ;;
        f) fork=$OPTARG
            ;;
        n) navitia_ci=1
            ;;
        d) debian=$OPTARG
            ;;
        h|\?)
            show_help
            exit 1
            ;;
    esac
done

if [[ -z $token ]] ||  [[ -z $branch ]] || [[ -z $debian ]];
then
     echo "Missing argument."
     show_help
     exit 1
fi

if [[ $debian == "debian8" ]]; then
    navitia_package="navitia-debian8-packages.zip"
    inside_navitia_package="navitia_debian8_packages.zip"
    cosmogony2cities_package="package-debian8.zip"
elif [[ $debian == "debian10" ]]; then
    navitia_package="navitia-debian10-packages.zip"
    inside_navitia_package="navitia_debian10_packages.zip"
    cosmogony2cities_package="package-debian10.zip"
else
    echo """debian version must be "debian8" or "debian10" """
    echo "***${debian}***"
    show_help
    exit 1
fi
echo "******************************************************************"
echo "navitia package selected: ${navitia_package}"
echo "inside navitia package selected: ${inside_navitia_package}"
echo "cosmogony2cities package selected: ${cosmogony2cities_package}"
echo "******************************************************************"

mimirsbrunn_package="debian-package-release.zip"

if [[ $event == "push" ]]; then
    if [[ $branch == "dev" ]]; then
        workflow="build_navitia_packages_for_dev_multi_distribution.yml"
        archive=$navitia_package
        inside_archive=$inside_navitia_package
    elif [[ $branch == "release" ]]; then
        workflow="build_navitia_packages_for_release.yml"
        archive=$navitia_package
        inside_archive=$inside_navitia_package
    else
        echo """branch must be "dev" or "release" for push events (-e push)"""
        echo "***${branch}***"
        show_help
        exit 1
    fi
    fork="hove-io"
elif [[ $event == "pull_request" ]]; then
    if [[ -z $branch ]]; then
        echo "branch must be set for pull_request events (-e pull_request -b branch_name)"
        show_help
        exit 1
    fi
    if [[ -z $fork ]]; then
        echo "fork must be set for pull_request events (-e pull_request -f fork)"
        show_help
        exit 1
    fi
    workflow="build_navitia_packages_for_dev_multi_distribution.yml"
    archive=$navitia_package
    inside_archive=$inside_navitia_package
else
    echo """event must be "push" or "pull_request" """
    echo "***${event}***"
    show_help
    exit 1
fi

if [[ $push -eq 1 ]]; then
    if [ -z $user ];
    then
        echo """Cannot push to docker registry without a "-u user." """
        show_help
        exit 1
    fi
    if [ -z $password ]; then
    echo """Cannot push to docker registry without a "-p password." """
        show_help
        exit 1
    fi
fi

# clone navitia source code
rm -rf ./navitia/
git clone https://x-token-auth:${token}@github.com/${fork}/navitia.git --branch $branch ./navitia/

# let's dowload the package built on gihub actions
# for that we need the submodule core_team_ci_tools
rm -rf ./core_team_ci_tools/
git clone https://x-token-auth:${token}@github.com/hove-io/core_team_ci_tools.git  ./core_team_ci_tools/

# we setup the right python environnement to use core_team_ci_tools
#pip install virtualenv -U
#virtualenv -py python3 ci_tools
#. ci_tools/bin/activate
pip install -r core_team_ci_tools/github_artifacts/requirements.txt --user

## let's download the navitia packages
## clone navitia source code
if [[ $navitia_ci -ne 1 ]]; then
    # let's dowload the package built on gihub actions
    rm -f $archive
    python core_team_ci_tools/github_artifacts/github_artifacts.py -o hove-io -r navitia -t $token -w $workflow -b $branch -a $archive -e $event --output-dir . --waiting
    # let's unzip what we received
    rm -f ./$inside_archive
    unzip -q ${archive}
#else
# In CI we assume that navitia_packages is already downloaded
fi


# let's unzip (again) to obtain the packages
rm -f navitia*.deb
unzip -qo ${inside_archive} -d .

# let's download mimirsbrunn package
python core_team_ci_tools/github_artifacts/github_artifacts.py -o hove-io -r mimirsbrunn -t $token -w release.yml -a $mimirsbrunn_package --output-dir .
unzip -qo $mimirsbrunn_package
# we select mimirsbrunn_jessie-*.deb
rm -f $mimirsbrunn_package

# Download cosmogony2cities
python core_team_ci_tools/github_artifacts/github_artifacts.py -o hove-io -r cosmogony2cities -t  $token -w build_package.yml -a $cosmogony2cities_package --output-dir .
# cosmogony2cities_*.deb
unzip -qo $cosmogony2cities_package
rm -f $cosmogony2cities_package

#deactivate

# let's retreive the navitia version
pushd navitia
version=$(git describe)
echo "building version $version"
popd



run docker build -f $debian/Dockerfile-master -t navitia/master .

components='jormungandr kraken tyr-beat tyr-worker tyr-web instances-configurator mock-kraken eitri'
for component in $components; do
    echo "*********  Building $component ***************"
    run docker build -t navitia/$component:$version -f  $debian/Dockerfile-${component} .

    # tag image if a -t tag was given
    if [ -n "${tag}" ]; then
        run docker tag navitia/$component:$version navitia/$component:$tag
    fi
done




# push image to docker registry if required with -r
if [[ $push -eq 1 ]]; then
    docker login -u $user -p $password
    for component in $components; do
        docker push navitia/$component:$version
        # also push tagged image if -t tag was given
        if [ -n "${tag}" ]; then
            docker push navitia/$component:$tag
        fi
    done
    docker logout
fi


# clean up
# we remove the ./navitia/ dir
rm -rf ./navitia/
# we remove the ./navitia/ dir
rm -rf ./core_team_ci_tools/
# the dowloaded archive for navitia package
rm -f ./$archive
# what was inside the archive
rm -f ./$inside_archive

# and all dowloaded packages
rm -f navitia*.deb

# the archive from mimirsbrunn package
rm -f archive.zip
# and what was inside the package
rm -f mimirsbrunn*.deb

rm -f mock-kraken*.deb
rm -f cosmogony2cities*.deb

# let's remove the navita/master docker image
docker rmi -f navitia/master
