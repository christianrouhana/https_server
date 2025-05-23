#include "https_tlsServer.h"
#include "https_client.h"
#include <thread>
#include <chrono>

int main()
{
    using namespace https;    
    TcpServer server = TcpServer("0.0.0.0", 443);   
    std::thread serverThread([&server]() {
        server.startListen();
    });

    std::this_thread::sleep_for(std::chrono::seconds(1));

    HttpsClient client("0.0.0.0", 443, "server.crt");
    if (client.connectToServer()) {
        std::string req = "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
        if (client.sendRequest(req)) {
            std::string response = client.receiveResponse();
            std::cout << "Client received:\n" << response << std::endl;
        } else {
            std::cerr << "Failed to send request\n";
        }
    } else {
        std::cerr << "Failed to connect to server\n";
    }

    serverThread.join();
    return 0;
}
