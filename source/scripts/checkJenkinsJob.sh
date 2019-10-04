#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
SET='\033[0m'

JENKINS_JOB=$1
JOB_COLOUR=$(curl -s "https://ci.navitia.io/api/json" | jq ".jobs[] | select(.name==\"$JENKINS_JOB\").color")

if [[ -z "$JENKINS_JOB" ]]; then
    echo "No job provided."
    echo ""
    echo "usage :"
    echo "  ./checkJenkinsJob <jobs>"
    echo ""
    echo " - job: a string that defines the job's name to be checked"
    echo ""
    exit 1
fi

if [[ -z $JOB_COLOUR ]]; then
    echo -e "${RED}Jenkins job '$JENKINS_JOB' does not exist${SET}" 1>&2
    exit 2
elif [[ "$JOB_COLOUR" != '"blue"' ]]; then
    echo -e "${RED} 🔥 Jenkins job '$JENKINS_JOB' is broken 🔥, please cease all acticity and go fix it 🚒 " 1>&2
    echo -e "${RED}   >> https://ci.navitia.io/job/$JENKINS_JOB/ ${SET}" 1>&2
    exit 3
fi

echo -e "${GREEN}Jenkins job '$JENKINS_JOB' is OK 👌 ${SET}"

