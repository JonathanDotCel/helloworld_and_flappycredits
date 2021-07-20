#!/bin/bash

# If docker's not on the path:
#docker=/Applications/Docker.app/Contents/Resources/bin/docker

ROOT=$(dirname $0)
echo root is $ROOT
CWD=$(pwd)
cd $ROOT
ROOT=$(pwd)
cd $CWD

#$docker pull unirom/build:latest

opts="--rm -t -i"
workdir="-w /project"
vol="-v ${ROOT}:/project"
user="-u `id -u`:`id -g`"

docker run $opts $vol $workdir $user unirom/build:latest bash -l


