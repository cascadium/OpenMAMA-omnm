# Use configurable image name
ARG IMAGE=centos:7.5.1804
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
        && apt-get install -y openmama; \
    fi

# CentOS dependencies
RUN if grep -qi centos /etc/*-release; then \
        yum install -y epel-release \
        && yum install -y cmake make gcc-c++ git redhat-rpm-config rpm-build zlib-devel openssl-devel \
        && curl -1sLf 'https://dl.cloudsmith.io/public/openmama/openmama-experimental/cfg/setup/bash.rpm.sh' | bash \
        && yum install -y openmama; \
    fi

# Fedora dependencies
RUN if grep -qP "^NAME=.*Fedora.*" /etc/*-release; then \
        dnf install --refresh -y libffi-devel ruby-devel rubygems redhat-rpm-config rpm-build cmake make gcc-c++ git redhat-rpm-config rpm-build zlib-devel openssl-devel yum-utils dnf-plugin-config-manager \
        && dnf update -y \
        && curl -1sLf 'https://dl.cloudsmith.io/public/openmama/openmama-experimental/cfg/setup/bash.rpm.sh' | bash \
        && dnf install --refresh -y openmama; \
    fi

# Install gtest
RUN curl -sL http://github.com/google/googletest/archive/release-$VERSION_GTEST.tar.gz | tar xz \
    && cd googletest-release-$VERSION_GTEST \
    && mkdir bld \
    && cd bld \
    && cmake -DCMAKE_INSTALL_PREFIX=/usr .. \
    && make -j \
    && make install \
    && rm -rf googletest-release-$VERSION_GTEST

# Install version of ruby known to play nicely with FPM on all platforms
RUN curl -sL https://cache.ruby-lang.org/pub/ruby/2.6/ruby-2.6.1.tar.gz | tar xz \
    && cd ruby-2.6.1 \
    && ./configure --prefix=/usr \
    && make -j \
    && make install \
    && gem update --system

# Install FPM
RUN gem install -N fpm

# Add the code for building
ADD . $SRC_DIR

# Run out-of-source builds
WORKDIR $SRC_DIR/build

# Perform the build
RUN git clone https://github.com/OpenMAMA/OpenMAMA.git \
    && cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
        -DOPENMAMA_SRC=$SRC_DIR/build/OpenMAMA \
        -DMAMA_ROOT=/opt/openmama \
        -DGTEST_ROOT=/usr .. \
    && make -j \
    && make install

# Perform unit tests. These should all yield successful exit codes
RUN ./src/unittests
RUN ./UnitTestMamaMsgC -m qpid -p omnmmsg -i O
RUN ./UnitTestMamaPayloadC -m qpid -p omnmmsg -i O --gtest_filter=-PayloadSubMsgTests.UpdateSubMsg

# When RPMs are generated, they'll go here
WORKDIR $RELEASE_DIR

# Generate the package (deb / rpm / tarball).
CMD fpm -s dir \
        -t $PACKAGE_TYPE \
        -m "openmama-users@lists.openmama.org" \
        --name openmama-omnm \
        --version $VERSION \
        --iteration 1 \
        --url "http://cascadium.io/" \
        --license MIT \
        -d openmama \
        -p openmama-omnm-$VERSION-1.$DISTRIB_PACKAGE_QUALIFIER.x86_64.$PACKAGE_TYPE \
        --description "OpenMAMA high performance Market Data API" \
        $INSTALL_DIR/=/opt/openmama/
