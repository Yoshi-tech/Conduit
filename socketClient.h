#pragma once


#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

class socketClient {
    public:
    socketClient(std::string ip, int port);
    ~socketClient();
    void sendMessage(std::string message);
    std::string receiveMessage();
    int sockfd;

    private:
    std::string ip;
    int port;
    void myConnect();
};