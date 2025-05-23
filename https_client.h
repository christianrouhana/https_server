#ifndef HTTPS_CLIENT
#define HTTPS_CLIENT

#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

namespace https {
    class HttpsClient {
        public:
            HttpsClient(const std::string& hostname, int port, const std::string& caCertPath);
            ~HttpsClient();

            bool connectToServer();
            bool sendRequest(const std::string& request);
            std::string receiveResponse();
        private: 
            std::string m_hostname;
            int m_port;
            int m_socket;
            std::string m_caCertPath;

            SSL_CTX* m_ctx;
            SSL* m_ssl;

            bool initSSL();
    void cleanupSSL();
    };
} //namespace https

#endif //HTTPS_CLIENT
