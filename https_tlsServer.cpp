#include "https_tlsServer.h"

#include <iostream>
#include <map>
#include <sstream>
#include <cstdlib>
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

    std::string urlDecode(const std::string &value)
    {
        std::string output;
        output.reserve(value.size());

        for (std::size_t i = 0; i < value.size(); ++i)
        {
            char ch = value[i];
            if (ch == '+')
            {
                output.push_back(' ');
            }
            else if (ch == '%' && i + 2 < value.size())
            {
                std::string hex = value.substr(i + 1, 2);
                char *end = nullptr;
                int decoded = static_cast<int>(strtol(hex.c_str(), &end, 16));
                if (end != nullptr && *end == '\0')
                {
                    output.push_back(static_cast<char>(decoded));
                    i += 2;
                }
                else
                {
                    output.push_back(ch);
                }
            }
            else
            {
                output.push_back(ch);
            }
        }

        return output;
    }

    std::map<std::string, std::string> parseQuery(const std::string &queryString)
    {
        std::map<std::string, std::string> params;
        std::size_t start = 0;
        while (start < queryString.size())
        {
            auto end = queryString.find('&', start);
            if (end == std::string::npos)
            {
                end = queryString.size();
            }

            auto separator = queryString.find('=', start);
            std::string key;
            std::string value;
            if (separator != std::string::npos && separator < end)
            {
                key = urlDecode(queryString.substr(start, separator - start));
                value = urlDecode(queryString.substr(separator + 1, end - separator - 1));
            }
            else
            {
                key = urlDecode(queryString.substr(start, end - start));
            }

            if (!key.empty())
            {
                params[key] = value;
            }

            start = end + 1;
        }

        return params;
    }

    std::string reasonPhrase(int statusCode)
    {
        switch (statusCode)
        {
        case 200:
            return "OK";
        case 400:
            return "Bad Request";
        case 404:
            return "Not Found";
        case 405:
            return "Method Not Allowed";
        case 500:
            return "Internal Server Error";
        default:
            return "";
        }
    }

    std::string buildHttpResponse(int statusCode, const std::string &body, const std::string &contentType)
    {
        std::ostringstream response;
        response << "HTTP/1.1 " << statusCode << ' ' << reasonPhrase(statusCode) << "\r\n";
        response << "Content-Type: " << contentType << "\r\n";
        response << "Content-Length: " << body.size() << "\r\n";
        response << "Connection: close\r\n\r\n";
        response << body;
        return response.str();
    }
}
namespace https
{
    TcpServer::TcpServer(std::string ip_address, int port, std::string caCertPath) : m_ip_address(std::move(ip_address)),
                                                                                    m_port(port),
                                                                                    m_socket(),
                                                                                    m_new_socket(),
                                                                                    m_incomingMessage(),
                                                                                    m_socketAddress(),
                                                                                    m_socketAddress_len(sizeof(m_socketAddress)),
                                                                                    m_ssl_ctx(nullptr),
                                                                                    m_ssl(nullptr),
                                                                                    m_caCertPath(std::move(caCertPath))
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

            if (bytesReceived == 0)
            {
                SSL_shutdown(m_ssl);
                SSL_free(m_ssl);
                m_ssl = nullptr;
                close(m_new_socket);
                continue;
            }

            std::ostringstream ss;
            ss << "----- Received request from client -----\n\n";
            log(ss.str());

            std::string request(buffer, bytesReceived);
            std::string responsePayload = handleRequest(request);

            long bytesSent = SSL_write(m_ssl, responsePayload.c_str(), responsePayload.size());

            if (bytesSent == static_cast<long>(responsePayload.size()))
            {
                log("----- Server Response sent to client -----\n\n");
            }
            else
            {
                log("Error sending response to client");
            }
            
            SSL_shutdown(m_ssl);
            SSL_free(m_ssl);
            m_ssl = nullptr;
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

    std::string TcpServer::handleRequest(const std::string &request)
    {
        std::istringstream requestStream(request);
        std::string method;
        std::string target;
        std::string version;
        requestStream >> method >> target >> version;

        if (method.empty())
        {
            return buildHttpResponse(400, "{\"error\":\"Malformed request\"}", "application/json");
        }

        if (method != "GET")
        {
            return buildHttpResponse(405, "{\"error\":\"Only GET supported\"}", "application/json");
        }

        std::string path = target;
        std::string queryString;
        auto queryPos = target.find('?');
        if (queryPos != std::string::npos)
        {
            path = target.substr(0, queryPos);
            queryString = target.substr(queryPos + 1);
        }

        if (path == "/" || path.empty())
        {
            std::string body = "{\"message\":\"OSRS Hiscore service. Use /player?name=Display%20Name\"}";
            return buildHttpResponse(200, body, "application/json");
        }

        if (path == "/player")
        {
            auto params = parseQuery(queryString);
            auto nameIt = params.find("name");
            if (nameIt == params.end() || nameIt->second.empty())
            {
                return buildHttpResponse(400, "{\"error\":\"Query parameter 'name' is required\"}", "application/json");
            }

            const std::string &playerName = nameIt->second;
            osrs::HiscoreClient client(m_caCertPath);
            osrs::PlayerSnapshot snapshot = client.fetchPlayer(playerName);
            std::string body = osrs::ToJson(snapshot);
            return buildHttpResponse(snapshot.success ? 200 : 502, body, "application/json");
        }

        return buildHttpResponse(404, "{\"error\":\"Not Found\"}", "application/json");
    }
} // namespace https
