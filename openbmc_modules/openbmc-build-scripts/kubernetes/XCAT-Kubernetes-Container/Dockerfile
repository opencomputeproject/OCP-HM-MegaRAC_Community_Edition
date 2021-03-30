FROM ubuntu:14.04

RUN apt-get update && apt-get install -yy \
    bzip2 \
    locales \
    openssh-server \
    wget

# Set the locale
RUN locale-gen en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8

RUN mkdir /var/run/sshd
RUN echo 'root:0penBMC' | chpasswd
RUN sed -ri 's/^PermitRootLogin\s+.*/PermitRootLogin yes/' /etc/ssh/sshd_config
RUN sed -ri 's/UsePAM yes/#UsePAM yes/g' /etc/ssh/sshd_config
expose 22

COPY setup_xcat.sh /
RUN /setup_xcat.sh

COPY entrypoint.sh /
CMD ["/entrypoint.sh"]
