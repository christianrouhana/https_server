#include "https_tlsServer_wsl.h"

#include <iostream>
#include <sstream>
#include <unistd.h>

namespace {
    const int BUFFER_SIZE = 30720;

    void log(const std::string &message)
    {
        std::cout << message << std::endl;
    }

    void exitWithError(const std::string &errorMessage)
    {
        log("ERROR: " + errorMessage);
        exit(1);
    }
}
namespace http
{
    TcpServer::TcpServer(std::string ip_address, int port) : m_ip_address(ip_address), m_port(port), m_socket(),
                                                             m_new_socket(), m_incomingMessage(), m_socketAddress(),
                                                             m_socketAddress_len(sizeof(m_socketAddress)), 
                                                             m_serverMessage(buildResponse()), m_ssl_ctx(nullptr), m_ssl(nullptr)
    {
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();

        m_ssl_ctx = SSL_CTX_new(TLS_server_method());
        if (!m_ssl_ctx)
        {
            exitWithError("Failed to create SSL context.");
        }

        if (SSL_CTX_use_certificate_file(m_ssl_ctx, "server.crt", SSL_FILETYPE_PEM) <= 0 || SSL_CTX_use_PrivateKey_file(m_ssl_ctx, "server.key", SSL_FILETYPE_PEM) <= 0)
        {
            exitWithError("Failed to load certificate or private key.");
        }

        m_socketAddress.sin_family = AF_INET;
        m_socketAddress.sin_port = htons(m_port);
        m_socketAddress.sin_addr.s_addr = inet_addr(m_ip_address.c_str());

        if (startServer() != 0) 
        {
            std::ostringstream ss;
            ss << "Failed to load start server with PORT: " << ntohs(m_socketAddress.sin_port);
            log(ss.str());
        }
    }
    TcpServer::~TcpServer()
    {
        SSL_free(m_ssl);
        SSL_CTX_free(m_ssl_ctx);
        EVP_cleanup();
        closeServer();
    }

    int TcpServer::startServer()
    {
        m_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (m_socket < 0)
        {
            exitWithError("Cannot create socket");
            return 1;
        }
        
        if (bind(m_socket, (sockaddr *)&m_socketAddress, m_socketAddress_len) < 0)
        {
            exitWithError("Cannot connect socket to address");
            return 1;
        }

        return 0;
    }

    void TcpServer::closeServer()
    {
        close(m_socket);
        close(m_new_socket);
        exit(0);
    }

    void TcpServer::startListen()
    {
        if (listen(m_socket, 20) < 0)
        {
            exitWithError("Socket listen failed.");
        }

        std::ostringstream ss;
        ss << "\n*** Listening on ADDRESS: " << inet_ntoa(m_socketAddress.sin_addr) << " PORT : " << ntohs(m_socketAddress.sin_port) << " ***\n\n";
        log(ss.str());

        int bytesReceived;

        while (true)
        {
            log("===== Waiting for a new connection =====\n\n\n");
            acceptConnection(m_new_socket);
            
            m_ssl = SSL_new(m_ssl_ctx);
            SSL_set_fd(m_ssl, m_new_socket);

            if (SSL_accept(m_ssl) <= 0)
            {
                ERR_print_errors_fp(stderr);
                continue;
            }

            char buffer[BUFFER_SIZE] = {0};
            bytesReceived = SSL_read(m_ssl, buffer, BUFFER_SIZE);
            if (bytesReceived < 0) 
            {
                exitWithError("Failed to read bytes from client socket connection.");
            }

            std::ostringstream ss;
            ss << "----- Received request from client -----\n\n";
            log(ss.str());

            sendResponse();
            
            SSL_shutdown(m_ssl);
            SSL_free(m_ssl);
            close(m_new_socket);
        }
    }

    void TcpServer::acceptConnection(int &new_socket)
    {
        new_socket = accept(m_socket, (sockaddr *)&m_socketAddress, &m_socketAddress_len);
        if (new_socket < 0)
        {
            std::ostringstream ss;
            ss << "Server failed to accept incoming connection from ADDRESS: " << inet_ntoa(m_socketAddress.sin_addr) << "; PORT: " << ntohs(m_socketAddress.sin_port);
            exitWithError(ss.str());
        }
    }

    std::string TcpServer::buildResponse()
    {
        std::string htmlFile = "<!DOCTYPE html><html lang=\"en\"><body><h1> Success </h1><p> Server works. </p></body></html>";
        std::ostringstream ss;
        ss << "HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: " << htmlFile.size() << "\n\n"
           << htmlFile;

        return ss.str();
    }

    void TcpServer::sendResponse()
    {
        long bytesSent;

        bytesSent = SSL_write(m_ssl, m_serverMessage.c_str(), m_serverMessage.size());

        if (bytesSent == m_serverMessage.size())
        {
            log("----- Server Response sent to client -----\n\n");
        }
        else 
        {
            log("Error sending response to client");
        }
        
    }
} // namespace http
