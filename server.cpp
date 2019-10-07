#include <algorithm>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <stdexcept>
#include <iostream>

#define ARG_COUNT 5
#define MAX_PORT 65535

int main(int argc, char *argv[]) {
    unsigned portNumber;
    unsigned long workersNumber;

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
                 << "./client -a <server_ipv4_address> -p <server_port_number>\n";
        return -1;
    }
    
    return 0;
}