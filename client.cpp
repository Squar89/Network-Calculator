#include <algorithm>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <stdexcept>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>

#define ARG_COUNT 5

int main(int argc, char *argv[]) {
    struct sockaddr_in serverAddress;
    unsigned portNumber;
    int sockfd;

    /*DEBUG*/
    const char *hello = (std::string ("Hello from client")).c_str();
    char buffer[1024] = {0};
    /*DEBUG*/

    //check for required command line arguments
    if (argc == ARG_COUNT
        && strncmp(argv[1], "-a ", 3)
        && strncmp(argv[3], "-p ", 3)) {

        //test for negative values in arguments
        if (argv[4][0] == '-') {
            std::cout << "Port number must be a positive value\n";
            return -1;
        }
        
        //parse server address
        if (inet_pton(AF_INET, argv[2], &serverAddress.sin_addr) < 1) {
            std::cout << "Given address is not a valid one.\n";
            return -1;
        }

        //parse port number
        try {
            unsigned long parsedPort = std::stoul(argv[4], nullptr, 10);
            if (parsedPort > 65535) {
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
    }
    else {
        std::cout << "Wrong command line arguments format. Correct usage:" << "\n" 
                 << "./client -a <server_ipv4_address> -p <server_port_number>\n";
        return -1;
    }

    //setup rest of sockaddr_in
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(portNumber);

    //open socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::cerr << "Error occurred while opening a client socket\n";
        return -1; 
    }

    //open connection
    if (connect(sockfd, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "Error occurred while opening a socket connection\n";
        return -1; 
    }

    /*DEBUG*/
    if (send(sockfd, hello, strlen(hello), 0) <= 0) {
        std::cerr << "Error occurred while sending data\n";
        return -1;
    }
    std::cout << "Message sent\n";

    if (recv(sockfd, buffer, 1024, 0) <= 0) {
        std::cerr << "Error occurred while receiving data\n";
        return -1;
    }
    std::cout << "Message received:\n" << buffer;
    /*DEBUG*/

    //close the socket
    shutdown(sockfd, 0);

    return 0;
}