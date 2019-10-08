#include <algorithm>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <stdexcept>
#include <iostream>
#include <cstring>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <thread>

#define ARG_COUNT 5
#define MAX_PORT 65535
#define MAX_INCOMING_Q 30
#define BUFFER_SIZE 99999

template <typename T>
class BlockingQ {

private:
    std::mutex              d_mutex;
    std::condition_variable d_condition;
    std::deque<T>           d_queue;

public:
    void push(T const& value) {
        {
            std::unique_lock<std::mutex> lock(this->d_mutex);
            d_queue.push_front(value);
        }
        this->d_condition.notify_one();
    }
    T pop() {
        std::unique_lock<std::mutex> lock(this->d_mutex);
        this->d_condition.wait(lock, [=]{ return !this->d_queue.empty(); });
        T rc(std::move(this->d_queue.back()));
        this->d_queue.pop_back();
        return rc;
    }
};

void listenerFunction(struct sockaddr_in serverAddress, BlockingQ<int>& requestsQ) {
    int serverSock, clientSock;
    unsigned long addrLen;
    struct sockaddr_in clientAddress;

    //open socket
    if ((serverSock = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::cerr << "Error occurred while opening a server socket\n";
        return;
    }

    //bind socket
    if (bind(serverSock, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "Error occurred while binding socket\n";
        return;
    }

    //start listening for incoming connections on created socket
    if (listen(serverSock, MAX_INCOMING_Q) == -1) {
        std::cerr << "Error occurred while starting to listen on a socket\n";
        return;
    }

    //start endless loop waiting for connections and handling them
    while(true) {
        //accept incoming connection and save it to clientSock
        addrLen = sizeof(serverAddress);
        if ((clientSock = accept(serverSock, (struct sockaddr*) &clientAddress, (socklen_t*) &addrLen)) < 0) {
            std::cerr << "Error occurred while accepting incoming connection\n";
            return;
        }

        //put the client Socket descriptor onto Blocking queue so one of the workers picks it up
        requestsQ.push(clientSock);

        //loop again, waiting for another connection
    }

    //same as thread joining in main, probably never going to happen but here for good measures
    shutdown(clientSock, 0);
    shutdown(serverSock, 0);

    return;
}

std::string calculateExpression(char* expression) {
    long long partialResult, lastParsed;
    bool addInProgress, subtractInProgress, numberWasLast = false, firstNumber = true;

    partialResult = lastParsed = 0;
    addInProgress = subtractInProgress = false;
    for (unsigned long i = 0; i < strlen(expression); i++) {
        //parse the whole next number
        if (expression[i] >= 48 && expression[i] <= 57) {
            //if there is a number after a number then return ERROR
            if (numberWasLast) {
                return "ERROR";
            }

            while (expression[i] >= 48 && expression[i] <= 57 && i < strlen(expression)) {
                lastParsed = lastParsed * 10 + (((int) expression[i]) - 48);
                i++;
                numberWasLast = true;
            }
        }
        //a char (non whitespace) before last parsed number was a plus sign
        if (addInProgress && numberWasLast) {
            partialResult += lastParsed;
            addInProgress = false;
            lastParsed = 0;
            firstNumber = false;
        }
        //a char (non whitespace) before last parsed number was a minus sign
        else if (subtractInProgress && numberWasLast) {
            partialResult -= lastParsed;
            subtractInProgress = false;
            lastParsed = 0;
            firstNumber = false;
        }

        //first number goes directly into partialResult (unless already added by add or subtract)
        if (firstNumber && numberWasLast) {
            partialResult = lastParsed;
            firstNumber = false;
            lastParsed = 0;
        }

        //check if the last parsed number was the last one of the expression
        if (i >= strlen(expression)) {
            return std::to_string(partialResult);
        }

        //check if next char is a plus sign
        if (expression[i] == '+' && !addInProgress && !subtractInProgress && !firstNumber) {
            addInProgress = true;
            numberWasLast = false;
        }
        //check if next char is a minus sign
        else if (expression[i] == '-' && !addInProgress && !subtractInProgress && !firstNumber) {
            subtractInProgress = true;
            numberWasLast = false;
        }
        //return the result if EOL or EOF is found
        else if ((expression[i] == EOF || expression[i] == '\n') && numberWasLast) {
            return std::to_string(partialResult);
        }
        //skip the whitespace
        else if (expression[i] == ' ' || expression[i] == 9) {}
        //if none of these triggered, then the expression is incorrect
        else {
            return "ERROR";
        }
    }

    //expression cant finish in the middle of operation
    if (addInProgress || subtractInProgress) {
        return "ERROR";
    }

    return std::to_string(partialResult);
}

void workerFunction(BlockingQ<int>& requestsQ) {
    char expression[BUFFER_SIZE], response[BUFFER_SIZE];

    //start endless loop of receiving, calculating and responding
    while(true) {
        //clear previous expressions and responses
        memset(expression, 0, sizeof(expression));
        memset(response, 0, sizeof(response));

        //pickup next request socket descriptor or wait if there are none (until awaken by q)
        int clientSock = requestsQ.pop();

        //receive raw format of expression from the client
        if (recv(clientSock, expression, BUFFER_SIZE, 0) < 0) {
            std::cerr << "Error occurred while receiving data\n";
            return;
        }

        //parse the expression and prepare the response
        strcpy(response, (calculateExpression(expression)).c_str());

        if (send(clientSock, response, strlen(response), 0) < 0) {
            std::cerr << "Error occurred while sending data\n";
            return;
        }
    }

    return;
}

int main(int argc, char *argv[]) {
    unsigned portNumber;
    unsigned long workersNumber;
    struct sockaddr_in serverAddress;
    BlockingQ<int> requestsQ;//queue for client requests, listener populates it, workers pop it

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

    std::thread workerThread[workersNumber];
    //start worker threads
    for (unsigned long i = 0; i < workersNumber; i++) {
        workerThread[i] = std::thread(workerFunction, std::ref(requestsQ));
    }

    //start listener thread
    std::thread listenerThread(listenerFunction, serverAddress, std::ref(requestsQ));

    //this will probably never happen as both listener and worker operates in endless loop
    //but I placed it here for good measures - join created threads
    listenerThread.join();
    for (unsigned long i = 0; i < workersNumber; i++) {
        workerThread[i].join();
    }
    
    return 0;
}
