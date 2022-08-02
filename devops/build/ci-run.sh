#!/bin/bash

. /etc/profile

# All errors fatal
set -e -x

export OPENMAMA_INSTALL_DIR=/opt/openmama
BUILD_DIR=/build-ci-run
SOURCE_PATH_RELATIVE=$(dirname "$0")/../..
SOURCE_PATH_ABSOLUTE=$(cd "$SOURCE_PATH_RELATIVE" && pwd)

git config --global --add safe.directory SOURCE_PATH_ABSOLUTE
git config --global user.email "devops@cascadium.io"
git config --global user.name "Cascadium Devops"

# Update to required cmake version
cd /usr
wget -c https://github.com/Kitware/CMake/releases/download/v3.19.4/cmake-3.19.4-Linux-x86_64.tar.gz -O - | tar -xz  --strip-components 1

# Constants
RHEL=CentOS
UBUNTU=Ubuntu

if [ -f /etc/redhat-release ]
then
    DISTRIB_RELEASE=$(cat /etc/redhat-release | tr " " "\n" | egrep "^[0-9]")
    DEPENDS_FLAGS="-d libuuid -d libevent -d ncurses -d apr -d java-11-openjdk"
    DISTRIB_ID=$RHEL
    PACKAGE_TYPE=rpm
    CLOUDSMITH_DISTRO_NAME=centos
    CLOUDSMITH_DISTRO_VERSION=${DISTRIB_RELEASE%%.*}
    PACKAGE_MANAGER=yum
fi

if [ -f /etc/lsb-release ]
then
    # Will set DISTRIB_ID and DISTRIB_RELEASE
    source /etc/lsb-release
    PACKAGE_TYPE=deb
    DEPENDS_FLAGS="-d uuid -d libevent-dev -d libzmq3-dev -d ncurses-dev -d libapr1-dev -d openjdk-11-jdk"
    CLOUDSMITH_DISTRO_NAME=ubuntu
    CLOUDSMITH_DISTRO_VERSION=${DISTRIB_CODENAME}
    PACKAGE_MANAGER=apt
fi

# Set up cloudsmith repository
curl -1sLf "https://dl.cloudsmith.io/public/openmama/openmama-experimental/cfg/setup/bash.${PACKAGE_TYPE}.sh" | bash

# Install OpenMAMA and git
$PACKAGE_MANAGER install -y openmama git

# Build the project
if [ -d $BUILD_DIR ]
then
    rm -rf $BUILD_DIR
fi

mkdir -p $BUILD_DIR
cd $BUILD_DIR
export LD_LIBRARY_PATH=/opt/openmama-omnm/lib:/opt/openmama/lib
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DWITH_UNITTEST=ON -DCMAKE_INSTALL_PREFIX=/opt/openmama-omnm -DMAMA_ROOT=$OPENMAMA_INSTALL_DIR "$SOURCE_PATH_ABSOLUTE"
make -j
make install
ctest . --timeout 120 --output-on-failure -E MsgFieldVectorBoolTests.GetVectorBoolNullField

# Generate the package (deb / rpm / tarball).
# Globals
ARTIFACT_TYPE=${ARTIFACT_TYPE:-dev}
VERSION_FILE=${VERSION_FILE:-"$SOURCE_PATH_ABSOLUTE/VERSION"}
VERSION=$(cat "$VERSION_FILE" | xargs)

if [ "$DISTRIB_ID" == "$RHEL" ]
then
    DISTRIB_PACKAGE_QUALIFIER=el${DISTRIB_RELEASE%%.*}
elif [ "$DISTRIB_ID" == "$UBUNTU" ]
then
    DISTRIB_PACKAGE_QUALIFIER=ubuntu${DISTRIB_RELEASE%%.*}
else
    echo "Unsupported distro [$DISTRIB_ID] found: $(cat /etc/*-release)" && exit $LINENO
fi

DIST_DIR="$SOURCE_PATH_ABSOLUTE/dist"
if [ ! -d "$DIST_DIR" ]
then
    mkdir "$DIST_DIR"
fi

PACKAGE_FILE="$DIST_DIR/openmama-omnm-$VERSION-1.$DISTRIB_PACKAGE_QUALIFIER.x86_64.$PACKAGE_TYPE"
fpm -s dir --force \
        -t $PACKAGE_TYPE \
        -m "contact@cascadium.io" \
        --name openmama-omnm \
        --version $VERSION \
        --iteration 1 \
        --url "http://cascadium.io" \
        --license MIT \
        -d openmama \
        -p "$PACKAGE_FILE" \
        --description "OpenMAMA OMNM payload bridge" \
        /opt/openmama-omnm/=/opt/openmama/

if [ "true" = "$PACKAGE_UPLOAD_ENABLED" ] && [ "$CLOUDSMITH_REPOSITORY" != "none" ]
then
    echo cloudsmith push $PACKAGE_TYPE "openmama/$CLOUDSMITH_REPOSITORY/$CLOUDSMITH_DISTRO_NAME/$CLOUDSMITH_DISTRO_VERSION" "${PACKAGE_FILE}"
fi
