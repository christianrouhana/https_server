FROM ubuntu:22.04

RUN apt-get update && \
    apt-get install -y build-essential libssl-dev openssl

COPY . /usr/src/https_server
WORKDIR /usr/src/https_server

# RUN ls
RUN g++ -o HttpsWSL server_wsl.cpp https_tlsServer_wsl.cpp -lssl -lcrypto

#change this to whatever port you choose to run your server on
EXPOSE 443
