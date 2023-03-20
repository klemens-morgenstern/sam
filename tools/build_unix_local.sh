#!/bin/bash


set -e

BK=b2
IMAGE=build-gcc11
CONTAINER=builder-sam-$IMAGE-$BK
FULL_IMAGE=ghcr.io/anarthal-containers/$IMAGE
REPO_BASE=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )/.." &> /dev/null && pwd )

docker start $CONTAINER || docker run -dit \
    --name $CONTAINER \
    -v $REPO_BASE:/opt/boost-sam \
    $FULL_IMAGE
docker exec $CONTAINER python /opt/boost-sam/tools/ci.py --source-dir=/opt/boost-sam \
    --build-kind=$BK \
    --build-shared-libs=1 \
    --valgrind=0 \
    --coverage=0 \
    --clean=0 \
    --toolset=gcc \
    --cxxstd=20 \
    --variant=release \
    --cmake-standalone-tests=1 \
    --cmake-add-subdir-tests=1 \
    --cmake-install-tests=1 \
    --cmake-build-type=Release \
    --stdlib=native
