#
# CentOS
#
ARG IMG_VER
ARG BUILD_DATE
ARG GIT_VERSION
ARG CMAKE_VERSION
ARG DISPLAY
ARG SCREEN

FROM centos:${IMG_VER:-latest}

LABEL maintainer="Gaia3D Inc. <mago3d@gaia3d.com>" \
		description="Development environment for CentOS" \
		org.label-schema.name="" \
		org.label-schema.build-date=$BUILD_DATE \
		org.label-schema.schema-version="1.0.0"

ENV GIT_VERSION ${GIT_VERSION:-2.17.0}
ENV CMAKE_VERSION ${CMAKE_VERSION:-3.11.0}
ENV DISPLAY ${DISPLAY:-:99}
ENV SCREEN ${SCREEN:-1920x1080x24}

# Dependencies for development environment
RUN yum -y install wget centos-release-scl && \
	wget http://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm && \
	rpm -Uvh epel-release-latest-7.noarch.rpm && \
	yum -y update
	
RUN yum -y install \
		openssh-server \
		make gcc-c++ gdb gdb-gdbserver zip rsync \
		devtoolset-4-toolchain scl-utils \
		curl-devel expat-devel zlib-devel gettext \
		openssl-devel ncurses-devel \
		boost-devel \
		libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel \
		mesa-dri-drivers mesa-libGL-devel mesa-libGLU-devel\
		xorg-x11-server-Xvfb \
		proj-devel xerces-c-devel libgeotiff-devel gdal-devel liblas-devel \
		&& \
    rm -fr /var/cache/yum/*

RUN mkdir /var/run/sshd && \
	ssh-keygen -t rsa -f /etc/ssh/ssh_host_rsa_key -N '' && \
	ssh-keygen -t ecdsa -f /etc/ssh/ssh_host_ecdsa_key -N '' && \
	ssh-keygen -A

RUN echo X11UseLocalhost yes >> /etc/ssh/sshd_config && \
	echo X11Forwarding yes >> /etc/ssh/sshd_config && \
	echo PubkeyAuthentication yes >> /etc/ssh/sshd_config

# Create User
RUN mkdir /app && useradd -m -s /bin/bash -d /app/mago3d mago3d

RUN	mkdir /app/mago3d/.ssh && \
	chown mago3d /app/mago3d/.ssh && \
	chmod 700 /app/mago3d/.ssh

#RUN echo 'mago3d:welcome' | chpasswd

COPY mago3d.pub /app/mago3d/.ssh/authorized_keys
RUN chown mago3d:mago3d /app/mago3d/.ssh/authorized_keys && \
	chmod 644 /app/mago3d/.ssh/authorized_keys

# Enable the SCL for all bash scripts.
# source scl_source enable devtoolset-4
COPY env/enabledevtoolset-4.sh /etc/profile.d/

# Build and install git from source.
WORKDIR /usr/src

RUN wget --no-check-certificate https://www.kernel.org/pub/software/scm/git/git-${GIT_VERSION}.tar.gz && \
	tar xvzf git-${GIT_VERSION}.tar.gz && \
	cd git-${GIT_VERSION} && \
	./configure --prefix=/usr/local && \
	make -j$(grep -c processor /proc/cpuinfo) && \
	make install && \
	cd .. && rm -rf git-${GIT_VERSION}*

# Build and install CMake from source.
RUN VERSION="$(cut -d '.' -f 1 <<<"${CMAKE_VERSION}")"."$(cut -d '.' -f 2 <<<"${CMAKE_VERSION}")" && \
	wget --no-check-certificate https://cmake.org/files/v${VERSION}/cmake-${CMAKE_VERSION}.tar.gz && \
	tar xvzf cmake-${CMAKE_VERSION}.tar.gz && \
	cd cmake-${CMAKE_VERSION} && \
	./configure --prefix=/usr/local && \
	make -j$(grep -c processor /proc/cpuinfo) && \
	./bin/cmake \
		-DCMAKE_BUILD_TYPE:STRING=Release \
		-DCMAKE_USE_OPENSSL:BOOL=ON . && \
	make install && \
	cd .. && rm -rf cmake-${CMAKE_VERSION}*

WORKDIR /app/mago3d

EXPOSE 22

# Set the locale
ENV LANG ko_KR.utf8
ENV LC_ALL ko_KR.utf8
RUN localedef -f UTF-8 -i ko_KR ko_KR.utf8 && \
	ln -sf /usr/share/zoneinfo/Asia/Seoul /etc/localtime

#export DISPLAY=:99
COPY env/set-locale.sh /etc/profile.d/
#export LANG=ko_KR.utf8
#export LC_ALL=ko_KR.utf8
COPY env/set-display.sh /etc/profile.d/

# Copy our entrypoint into the container.
COPY env/entrypoint.sh /usr/local/bin/entrypoint.sh

# Set the default command.
ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]

CMD ["/bin/bash"]

