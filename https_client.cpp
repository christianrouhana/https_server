#include "https_client.h"

#include <cstring>
#include <unistd.h>
#include <netdb.h>

namespace https {
    HttpsClient::HttpsClient(const std::string& hostname, int port, const std::string& caCertPath)
        : m_hostname(hostname), m_port(port), m_socket(-1), m_caCertPath(caCertPath), m_ctx(nullptr), m_ssl(nullptr) {}

    HttpsClient::~HttpsClient() {
        cleanupSSL();
    }

    bool HttpsClient::initSSL() {
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();

        const SSL_METHOD* method = TLS_client_method();
        m_ctx = SSL_CTX_new(method);
        if (!m_ctx) return false;

        if (SSL_CTX_load_verify_locations(m_ctx, m_caCertPath.c_str(), nullptr) != 1) {
            std::cerr << "Failed to load CA certificate from " << m_caCertPath << std::endl;
            return false;
        }

        SSL_CTX_set_verify(m_ctx, SSL_VERIFY_PEER, nullptr);
        return true;
    }

    bool HttpsClient::connectToServer() {
        if (!initSSL()) return false;

        m_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (m_socket < 0) return false;

        struct hostent* host = gethostbyname(m_hostname.c_str());
        if (!host) return false;

        sockaddr_in server_addr;
        std::memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(m_port);
        std::memcpy(&server_addr.sin_addr.s_addr, host->h_addr, host->h_length);

        if (connect(m_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
            close(m_socket);
            return false;
        }

        m_ssl = SSL_new(m_ctx);
        SSL_set_fd(m_ssl, m_socket);

        if (SSL_connect(m_ssl) <= 0) {
            ERR_print_errors_fp(stderr);
            return false;
        }

        // Certificate verification
        if (SSL_get_verify_result(m_ssl) != X509_V_OK) {
            std::cerr << "Certificate verification failed" << std::endl;
            return false;
        }

        return true;
    }
    bool HttpsClient::sendRequest(const std::string& request) {
        if (!m_ssl) return false;
        return SSL_write(m_ssl, request.c_str(), request.length()) > 0;
    }

    std::string HttpsClient::receiveResponse() {
        if (!m_ssl) return "";

        std::string response;
        char buffer[4096];
        int bytes;
        while ((bytes = SSL_read(m_ssl, buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytes] = 0;
            response += buffer;
        }
        return response;
    }

    void HttpsClient::cleanupSSL() {
        if (m_ssl) {
            SSL_shutdown(m_ssl);
            SSL_free(m_ssl);
        }
        if (m_socket != -1) {
            close(m_socket);
        }
        if (m_ctx) {
            SSL_CTX_free(m_ctx);
        }
        EVP_cleanup();
    }
} // namespace https
