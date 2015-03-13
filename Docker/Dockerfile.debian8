# VERSION 0.1
FROM debian:8
MAINTAINER lordrasmus@gmail.com

RUN apt-get -y update
RUN apt-get -y install 	libevent-2.0 libevent-pthreads-2.0 libssl1.0.0 	libpython2.7


ADD run.sh /run.sh
ADD server.pem /libcwebui/server.pem
ADD root.pem /libcwebui/root.pem
ADD dh1024.pem /libcwebui/dh1024.pem

ADD libWebserver.so /libcwebui/libWebserver.so
ADD testSite /testSite

ADD main /libcwebui/main

EXPOSE 8080 4443

ENTRYPOINT ["/run.sh"]

