#include <algorithm>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <stdexcept>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <cstring>

#define ARG_COUNT 5
#define RECV_TIMEOUT 60 //in seconds

int main(int argc, char *argv[]) {
    struct sockaddr_in serverAddress;
    unsigned portNumber;
    int sockfd;
    struct timeval tv;
    tv.tv_sec = RECV_TIMEOUT;
    tv.tv_usec = 0;

    /*DEBUG*/
    char hello[1024];
        memset(hello, 0, sizeof(hello));
    strcpy(hello, "Hello from client");
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

    //set timeout on recv for created socket
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv) == -1) {
        std::cerr << "Error occurred while setting socket options\n";
        return -1;
    }

    //open connection
    if (connect(sockfd, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "Error occurred while opening a socket connection\n";
        return -1; 
    }

    /*DEBUG*/
    int sendRet;
    if ((sendRet = send(sockfd, hello, strlen(hello), 0)) <= 0) {
        std::cerr << "Error occurred while sending data\n";
	std::cerr << errno << "\n" << sendRet << "\n";
        return -1;
    }
    std::cout << "Message sent\n";

    if (recv(sockfd, buffer, 1024, 0) <= 0) {
/*===================================================================================================================================================================================================*/
        /* OK THIS IS ACTUALLY NOT A DEBUG */
        if (errno == EWOULDBLOCK || errno == EAGAIN)
        {
            std::cout << "TIMEOUT";
            return 1;
        }
        /* OK THIS IS ACTUALLY NOT A DEBUG */
/*===================================================================================================================================================================================================*/
        std::cerr << "Error occurred while receiving data\n";
        return -1;
    }
    std::cout << "Message received:\n" << buffer;
    /*DEBUG*/

    //close the socket
    shutdown(sockfd, 0);

    return 0;
}
