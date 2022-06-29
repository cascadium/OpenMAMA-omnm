#!/bin/bash

# All errors fatal
set -x

. /etc/profile

set -e

brew install openmama

export DYLD_LIBRARY_PATH=$(pwd)/install/lib

mkdir bld
cd bld
cmake -DCMAKE_INSTALL_PREFIX=$(pwd)/install -DMAMA_ROOT=$(echo /usr/local/Cellar/openmama/*) ..
make -j
make install
ctest . -E MsgFieldVectorBoolTests.GetVectorBoolNullField
