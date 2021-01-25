#
# CentOS
#
ARG BUILD_DATE
ARG IMAGE_VERSION
ARG GIT_VERSION
ARG CMAKE_VERSION
ARG DISPLAY
ARG SCREEN

FROM centos:${IMAGE_VERSION:-centos7}

LABEL maintainer="Gaia3D Inc. <mago3d@gaia3d.com>" \
		description="Development environment for CentOS" \
		org.label-schema.name="" \
		org.label-schema.build-date=$BUILD_DATE \
		org.label-schema.schema-version="1.0.0"

ENV GIT_VERSION ${GIT_VERSION:-2.30.0}
ENV CMAKE_VERSION ${CMAKE_VERSION:-3.19.2}
ENV LIBLAS_VERSION ${LIBLAS_VERSION:-1.8.1}
ENV DISPLAY ${DISPLAY:-:99}
ENV SCREEN ${SCREEN:-1920x1080x24}

# Dependencies for development environment
RUN yum -y install wget centos-release-scl epel-release && \
	yum -y install https://download.postgresql.org/pub/repos/yum/reporpms/EL-7-x86_64/pgdg-redhat-repo-latest.noarch.rpm && \
	yum -y update --exclude=kernel* && \
	yum -y install \
		make gcc-c++ \
		devtoolset-7 \
		boost-devel \
		libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel \
		mesa-dri-drivers mesa-libGL-devel mesa-libGLU-devel \
		xorg-x11-server-Xvfb xorg-x11-utils \
		proj71-devel xerces-c-devel libgeotiff-devel gdal31-devel geos38 \
		openssl-devel curl-devel expat-devel zlib-devel gettext bzip2\
		&& \
	yum clean all && \
    rm -fr /var/cache/yum/*

# Build and install git from source.
WORKDIR /usr/src

RUN wget --no-check-certificate https://www.kernel.org/pub/software/scm/git/git-${GIT_VERSION}.tar.gz && \
	tar xvzf git-${GIT_VERSION}.tar.gz && \
	cd git-${GIT_VERSION} && \
	./configure --with-expat --with-openssl --prefix=/usr/local && \
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

# Build and install liblas from source.
RUN wget http://download.osgeo.org/liblas/libLAS-${LIBLAS_VERSION}.tar.bz2 && \
	tar xvf libLAS-${LIBLAS_VERSION}.tar.bz2 && \
	cd libLAS-${LIBLAS_VERSION} && \
	mkdir build && cd build && \
	cmake .. -DGEOTIFF_INCLUDE_DIR="/usr/include/libgeotiff" && \
	make -j$(grep -c processor /proc/cpuinfo) && \
	make install && \
	cd ../.. && rm -rf libLAS-${LIBLAS_VERSION}*
	
RUN git clone --recursive https://github.com/Gaia3D/NewF4DConverter.git && \
	cd NewF4DConverter && \
	mkdir build && cd build && \
	source scl_source enable devtoolset-7 && \
	cmake .. -DCMAKE_INSTALL_PREFIX=/opt/gaia3d/f4dconverter -DPROJ4_INCLUDE_DIRS=/usr/proj71/include -DPROJ4_INCLUDE_DIR=/usr/proj71/include -DPROJ4_LIBRARY=/usr/proj71/lib/libproj.so -DGDAL_CONFIG=/usr/gdal31/bin/gdal-config && \
	make -j$(grep -c processor /proc/cpuinfo) && \
	make install && \
	cp -r /usr/proj71/share/proj /opt/gaia3d/f4dconverter/bin &&\
	cd ../.. && rm -rf NewF4DConverter

WORKDIR /opt/gaia3d

ENV PATH /opt/gaia3d/f4dconverter/bin:$PATH
ENV LD_LIBRARY_PATH /opt/gaia3d/f4dconverter/lib:/usr/local/lib:$LD_LIBRARY_PATH

# Set the locale
RUN localedef -i ko_KR -f UTF-8 ko_KR.utf8 && \
	ln -sf /usr/share/zoneinfo/Asia/Seoul /etc/localtime
ENV LANG ko_KR.utf8
ENV LC_ALL ko_KR.utf8

# Enable the SCL for all bash scripts.
# source scl_source enable devtoolset-7
COPY env/enabledevtoolset-7.sh /etc/profile.d/
COPY env/set-locale.sh /etc/profile.d/
COPY env/set-display.sh /etc/profile.d/

RUN chmod 644 /etc/profile.d/set-locale.sh \
	/etc/profile.d/set-display.sh \
	/etc/profile.d/enabledevtoolset-7.sh

# Copy our entrypoint into the container.
COPY env/entrypoint.sh /usr/local/bin/entrypoint.sh

RUN chmod +x /usr/local/bin/entrypoint.sh

# Set the default command.
ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]

CMD ["/bin/bash"]

