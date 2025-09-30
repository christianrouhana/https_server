FROM ubuntu:22.04

RUN apt-get update && \
    apt-get install -y build-essential libssl-dev openssl ca-certificates && \
    rm -rf /var/lib/apt/lists/* && \
    update-ca-certificates

COPY . /usr/src/https_server
WORKDIR /usr/src/https_server

RUN g++ -std=c++17 -Wall -Wextra -Wpedantic -o HttpsWSL \
    server.cpp https_tlsServer.cpp https_client.cpp osrs_hiscore.cpp \
    -lssl -lcrypto -lpthread

ENV OSRS_CA_BUNDLE=/etc/ssl/certs/ca-certificates.crt

# change this to whatever port you choose to run your server on
EXPOSE 443

ENTRYPOINT ["/usr/src/https_server/HttpsWSL"]
