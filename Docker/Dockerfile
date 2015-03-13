# VERSION 0.1
FROM fedora:24
MAINTAINER lordrasmus@gmail.com

RUN dnf -y install libevent openssl python-libs && dnf clean all
#libpython


ADD run.sh /run.sh
ADD server.pem /libcwebui/server.pem
ADD root.pem /libcwebui/root.pem
ADD dh1024.pem /libcwebui/dh1024.pem

ADD libWebserver.so /libcwebui/libWebserver.so
ADD testSite /testSite

ADD main /libcwebui/main

EXPOSE 8080 4443 4444

ENTRYPOINT ["/run.sh"]

