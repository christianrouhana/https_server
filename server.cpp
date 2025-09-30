#include "https_tlsServer.h"

#include <cstdlib>
#include <iostream>
#include <string>

int main()
{
    const char *caEnv = std::getenv("OSRS_CA_BUNDLE");
    std::string caPath = caEnv ? caEnv : "cacert.pem"; // Replace with path to your CA bundle

    https::TcpServer server("0.0.0.0", 443, caPath);

    std::cout << "Starting HTTPS server for OSRS hiscore proxy..." << std::endl;
    server.startListen();

    return 0;
}
