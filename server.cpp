#include <algorithm>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <stdexcept>
#include <iostream>
#include <cstring>

#define ARG_COUNT 5
#define MAX_PORT 65535
#define MAX_INCOMING_Q 30

int main(int argc, char *argv[]) {
    unsigned portNumber;
    unsigned long workersNumber;
    struct sockaddr_in serverAddress, clientAddress;
    int serverSock, clientSock;
    unsigned long addrLen;

    /*DEBUG*/
    const char *hello = (std::string ("Hello from server")).c_str();
    char buffer[1024] = {0};
    /*DEBUG*/

    //check for required command line arguments
    if (argc == ARG_COUNT
        && strncmp(argv[1], "-p ", 3)
        && strncmp(argv[3], "-t ", 3)) {

        //test for negative values in arguments
        if (argv[2][0] == '-' || argv[4][0] == '-') {
            std::cout << "Arguments must be positive values\n";
            return -1;
        }

        //parse port argument
        try {
            unsigned long parsedPort = std::stoul(argv[2], nullptr, 10);
            if (parsedPort > MAX_PORT) {
                std::cout << "Given port number is too large. It must fit in range of (0, 65535) inclusive\n";
                return -1;
            }
            else {
                portNumber = (unsigned) parsedPort;
            }
        }
        catch (const std::invalid_argument& ia) {
            std::cout << "Incorrect port number. Must be an int in range of (0, 65535) inclusive\n";
            return -1;
        }

        //parse workers argument
        try {
            workersNumber= std::stoul(argv[4], nullptr, 10);
        }
        catch (const std::invalid_argument& ia) {
            std::cout << "Incorrect workers argument. Must be an unsigned long value\n";
            return -1;
        }
    }
    else {
        std::cout << "Wrong command line arguments format. Correct usage:" << "\n" 
                 << "./server -p <server_port_number> -t <number_of_workers>\n";
        return -1;
    }

    //setup sockaddr_in for server side
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(portNumber);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    //open socket
    if ((serverSock = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::cerr << "Error occurred while opening a server socket\n";
        return -1;
    }

    //bind socket
    if (bind(serverSock, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "Error occurred while binding socket\n";
        return -1;
    }

    //start listening for incoming connections on created socket
    if (listen(serverSock, MAX_INCOMING_Q) == -1) {
        std::cerr << "Error occurred while starting to listen on a socket\n";
        return -1;
    }

    //accept incoming connection and save it to clientSock
    addrLen = sizeof(serverAddress);
    if ((clientSock = accept(serverSock, (struct sockaddr*) &clientAddress, (socklen_t*) &addrLen)) < 0) {
        std::cerr << "Error occurred while accepting incoming connection\n";
        return -1;
    }

    /*DEBUG*/
    int recvValue;
    if ((recvValue = recv(clientSock, buffer, 1024, 0)) <= 0) {
        std::cerr << "Error occurred while receiving data\n";
	std::cerr << errno << "\n" << recvValue << "\n";
        return -1;
    }
    std::cout << "Message received:\n" << buffer;

    if (send(clientSock, hello, strlen(hello), 0) <= 0) {
        std::cerr << "Error occurred while sending data\n";
        return -1;
    }
    std::cout << "Message sent\n";
    /*DEBUG*/

    shutdown(clientSock, 0);
    shutdown(serverSock, 0);
    
    return 0;
}
