#!/bin/bash

#http://www.g-loaded.eu/2005/11/10/be-your-own-ca/
#http://www.g-loaded.eu/2007/08/10/ssl-enabled-name-based-apache-virtual-hosts-with-mod_gnutls/

openssl req -config openssl.cnf -new -x509 -extensions v3_ca -keyout private/myca.key -out certs/myca.crt -days 1825
openssl req -config openssl.cnf -new -nodes -keyout private/server.key -out server.csr -days 365
openssl ca -config openssl.cnf -policy policy_anything -out certs/server.crt -infiles server.csr 


#openssl x509 -subject -issuer -enddate -noout -in certs/server.crt
#openssl verify -purpose sslserver -CAfile certs/myca.crt certs/server.crt


cat certs/server.crt private/server.key > private/server-key-cert.pe
