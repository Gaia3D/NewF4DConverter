#
# CentOS
#
ARG IMG_VER
ARG GIT_VERSION
ARG CMAKE_VERSION

FROM centos:${IMG_VER:-latest}

LABEL maintainer="Gaia3D Inc. <mago3d@gaia3d.com>"

ENV GIT_VERSION ${GIT_VERSION:-2.17.0}
ENV CMAKE_VERSION ${CMAKE_VERSION:-3.11.0}

# Dependencies for development environment
RUN yum -y install wget && \
	wget http://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm && \
	rpm -Uvh epel-release-latest-7.noarch.rpm && \
	yum -y update && \
	yum -y install \
		openssh-server \
		make gcc-c++ gdb gdb-gdbserver zip \
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

# Build and install git from source.
WORKDIR /usr/src

WORKDIR /app/mago3d

EXPOSE 22

CMD ["/usr/sbin/sshd", "-D"]