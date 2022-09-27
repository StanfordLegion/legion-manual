#!/usr/bin/env bash

set -e

LEGION_BRANCH="${1:-master}"

# Build Legion
if [[ ! -d legion ]]; then
    git clone -b "$LEGION_BRANCH" https://github.com/StanfordLegion/legion.git
fi
if [[ ! -d legion/build ]]; then
    mkdir legion/build legion/install
    pushd legion/build
    cmake -DCMAKE_INSTALL_PREFIX="$PWD"/../install -DCMAKE_CXX_STANDARD=11 ..
    popd
fi
pushd legion/build
make install -j${THREADS:-2}
popd

# Build Examples
mkdir -p Examples/build
pushd Examples/build
cmake -DCMAKE_PREFIX_PATH="$PWD"/../../legion/install ..
make -j${THREADS:-2}
ctest --output-on-failure -j${THREADS:-2}
popd
