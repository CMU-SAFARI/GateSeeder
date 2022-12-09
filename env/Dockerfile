FROM ubuntu:20.04
##
## install Vitis dependencies
##
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && \
    apt-get upgrade -y -qq && \
    apt-get install -y -qq --no-install-recommends \
        tofrodos \
        iproute2 \
        gawk \
        net-tools \
        libtinfo5 \
        libncurses5-dev \
        libncursesw5-dev \
        libssl-dev \
        flex \
        bison \
        libselinux1 \
        gnupg \
        wget \
        diffstat \
        chrpath \
        socat \
        x11-utils \
        dbus-x11 \
        xterm \
        autoconf \
        libtool \
        tar \
        unzip \
        texinfo \
        zlib1g-dev \
        g++-multilib \
        gcc-multilib \
        build-essential \
        libc6-dev-i386 \
        screen \
        pax \
        gzip \
        python2.7-dev \
        python2.7 \
        python3-dev \
        python3 \
        apt-utils \
        binutils \
        cpio \
        expect \
        file \
        fonts-noto-cjk \
        git \
        gosu \
        lib32z1-dev \
        libcanberra-gtk-module \
        libcanberra-gtk3-module \
        libglib2.0-dev \
        libgtk2.0-0 \
        libjpeg62-dev \
        libsdl1.2-dev \
        libstdc++6 \
        libtext-csv-perl \
        libtool-bin \
        libxi6 \
        libxrender1 \
        libxtst6 \
        locales \
        lsb-core \
        lsb-release \
        ncurses-dev \
        ocl-icd-libopencl1 \
        ocl-icd-opencl-dev \
        opencl-headers \
        pkg-config \
        software-properties-common \
        sudo \
        u-boot-tools \
        vim \
        xvfb \
        parted \
        udev \
        graphviz && \
    \
    add-apt-repository ppa:xorg-edgers/ppa && \
    apt-get update && \
    apt-get install -y -qq \
        libgl1-mesa-glx \
        libgl1-mesa-dri \
        libgl1-mesa-dev && \
    add-apt-repository --remove ppa:xorg-edgers/ppa && \
    \
    apt-get clean && \
    apt-get autoclean && \
    ln -fs /bin/bash /bin/sh && \
    \
    rm -rf /tmp/* && \
    rm -rf /var/tmp/* && \
    rm -rf /var/cache/* && \
    rm -rf /var/lib/apt/lists/*

##
## locale settings
##
RUN locale-gen en_US.UTF-8 && \
    update-locale
ENV LANG=en_US.UTF-8 LANGUAGE=en_US:en LC_ALL=en_US.UTF-8
#COPY xrt_202210.2.13.479_20.04-amd64-xrt.deb .
#COPY xilinx-u280-gen3x16-xdma-1-202211-1-dev_1-3585755_all.deb .
#RUN apt-get -y install ./xrt_202210.2.13.479_20.04-amd64-xrt.deb
#RUN apt-get -y install ./xilinx-u280-gen3x16-xdma-1-202211-1-dev_1-3585755_all.deb
CMD echo "FINISH"
