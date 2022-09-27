#!/usr/bin/env bash

set -e

if [[ -n $TEST_LATEX ]]; then
    docker build -f docker/Dockerfile.latex -t build .
    tmp=$(docker create build)
    docker cp $tmp:/build/legion.pdf .
    docker rm $tmp
fi

if [[ -n $TEST_CXX ]]; then
    docker build --build-arg LEGION_BRANCH=$LEGION_BRANCH -f docker/Dockerfile.cxx -t build .
fi
