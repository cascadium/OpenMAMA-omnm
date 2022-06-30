#!/bin/bash

# All errors fatal
set -x

. /etc/profile

set -e

brew install openmama


mkdir bld
cd bld
export DYLD_LIBRARY_PATH=$(pwd)/install/lib:$(echo /usr/local/Cellar/openmama/*)/lib
cmake -DCMAKE_INSTALL_PREFIX=$(pwd)/install -DMAMA_ROOT=$(echo /usr/local/Cellar/openmama/*) ..
make -j
make install
ctest . -E MsgFieldVectorBoolTests.GetVectorBoolNullField --timeout 120 --output-on-failure
