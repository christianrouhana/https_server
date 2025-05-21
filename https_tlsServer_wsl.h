#ifndef INCLUDED_HTTPS_TLSSERVER_WSL
#define INCLUDED_HTTPS_TLSSERVER_WSL

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string>
#include <openssl/ssl.h>
#include <openssl/err.h>

namespace http 
{
    class TcpServer
    {
        public:
            TcpServer(std::string ip_address, int port);
            ~TcpServer();
            void startListen();
            
        private:
            std::string m_ip_address;
            int m_port;
            int m_socket;
            int m_new_socket;
            long m_incomingMessage;
            struct sockaddr_in m_socketAddress;
            unsigned int m_socketAddress_len;
            std::string m_serverMessage;

            SSL_CTX *m_ssl_ctx;
            SSL *m_ssl;
            
            int startServer();
            void closeServer();
            void acceptConnection(int &new_socket);
            std::string buildResponse();
            void sendResponse();

    };
} // namespace http

#endif
