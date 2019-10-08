#!/bin/bash
g++ -Wall -Wextra -o server -std=c++11 -pthread server.cpp
g++ -Wall -Wextra -o client -std=c++11 -pthread client.cpp