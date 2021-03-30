# Dockerfile for x86_64 Jenkins JNLP Slave Agent Container with Kubectl and Docker Installed

FROM jenkinsci/jnlp-slave

USER root

# Download and install Kubectl
RUN curl -LO https://storage.googleapis.com/kubernetes-release/release/$(curl -s \
    https://storage.googleapis.com/kubernetes-release/release/stable.txt)/bin/linux/amd64/kubectl \
    && chmod +x ./kubectl \
    && mv ./kubectl /usr/local/bin/kubectl

# install Docker and grant access to Jenkins user
RUN apt-get update && apt-get install -yy \
    apt-transport-https \
    ca-certificates \
    software-properties-common

RUN add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu xenial stable" \
    && apt-get update \
    && apt-get install -yy --allow-unauthenticated docker-ce \
    && groupmod --gid 999 docker

RUN usermod -a -G docker jenkins

USER jenkins

ENTRYPOINT ["jenkins-slave"]
