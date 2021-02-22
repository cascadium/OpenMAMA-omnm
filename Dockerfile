# Use configurable image name
ARG IMAGE=openmama/openmama-dev:centos-7
FROM $IMAGE

ARG IMAGE
ARG PACKAGE_TYPE=rpm
ARG VERSION=0.0.1
ARG DISTRIB_PACKAGE_QUALIFIER=el7

ENV DEBIAN_FRONTEND=noninteractive
ENV IMAGE $IMAGE
ENV VERSION_GTEST 1.8.0
ENV SRC_DIR /apps/src
ENV INSTALL_DIR /opt/openmama-omnm
ENV LD_LIBRARY_PATH=/opt/openmama/lib:$INSTALL_DIR/lib
ENV WOMBAT_PATH=/opt/openmama/config
ENV PACKAGE_TYPE=$PACKAGE_TYPE
ENV VERSION=$VERSION
ENV RELEASE_DIR /app/release
ENV DISTRIB_PACKAGE_QUALIFIER $DISTRIB_PACKAGE_QUALIFIER

WORKDIR /app/deps

# Ubuntu dependencies
RUN if grep -qi ubuntu /etc/*-release; then \
        apt-get update \
        && apt-get install -y build-essential zip unzip curl git libz-dev wget apt-transport-https ca-certificates cmake libssl-dev \
        && curl -1sLf 'https://dl.cloudsmith.io/public/openmama/openmama-experimental/cfg/setup/bash.deb.sh' | bash \
        && apt-get update \
        && apt-get install -y openmama git; \
    fi

# CentOS dependencies
RUN if grep -qi centos /etc/*-release; then \
        curl -1sLf 'https://dl.cloudsmith.io/public/openmama/openmama-experimental/cfg/setup/bash.rpm.sh' | bash \
        && yum install -y openmama git; \
    fi

# Fedora dependencies
RUN if grep -qP "^NAME=.*Fedora.*" /etc/*-release; then \
        dnf install --refresh -y libffi-devel ruby-devel rubygems redhat-rpm-config rpm-build cmake make gcc-c++ git redhat-rpm-config rpm-build zlib-devel openssl-devel yum-utils dnf-plugin-config-manager \
        && dnf update -y \
        && curl -1sLf 'https://dl.cloudsmith.io/public/openmama/openmama-experimental/cfg/setup/bash.rpm.sh' | bash \
        && dnf install --refresh -y openmama git; \
    fi

# Looking through OpenMAMA code base
RUN find /opt/openmama

# Add the code for building
ADD . $SRC_DIR

# Perform the build and install it
RUN mkdir build && cd build && cmake -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR $SRC_DIR && make -j install

# Perform unit tests. These should all yield successful exit codes
RUN ./build/src/unittests
RUN ./build/UnitTestMamaMsgC -m qpid -p omnmmsg -i O
RUN ./build/UnitTestMamaPayloadC -m qpid -p omnmmsg -i O --gtest_filter=-PayloadSubMsgTests.UpdateSubMsg

# When RPMs are generated, they'll go here
WORKDIR $RELEASE_DIR

# Generate the package (deb / rpm / tarball).
CMD fpm -s dir \
        -t $PACKAGE_TYPE \
        -m "contact@cascadium.io" \
        --name openmama-omnm \
        --version `cat VERSION` \
        --iteration 1 \
        --url "http://cascadium.io/" \
        --license MIT \
        -d openmama \
        -p openmama-omnm-$VERSION-1.$DISTRIB_PACKAGE_QUALIFIER.x86_64.$PACKAGE_TYPE \
        --description "OpenMAMA OMNM payload bridge" \
        $INSTALL_DIR/=/opt/openmama/
