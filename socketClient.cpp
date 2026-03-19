#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include "socketClient.h"
#include "socketListen.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

socketClient::socketClient(std::string ip, int port){
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        std::cerr << "Error creating socket" << std::endl;
        exit(1);
    }

    this->ip = ip;
    this->port = port;
    myConnect();
}

socketClient::~socketClient(){
    close(sockfd);
}

void socketClient::myConnect(){

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if(getaddrinfo(ip.c_str(), std::to_string(port).c_str(), &hints, &res) != 0){
        std::cerr << "Error getting address info" << std::endl;
        exit(1);
    }
    struct sockaddr_in* server_addr = (struct sockaddr_in*)res->ai_addr;
    if(server_addr == nullptr){
        std::cerr << "Error getting server address" << std::endl;
        exit(1);
    }
    if(connect(sockfd, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0){
        std::cerr << "Error connecting to server" << std::endl;
        exit(1);
    }
    freeaddrinfo(res);
}

void socketClient::sendMessage(std::string message){
    if(send(sockfd, message.c_str(), message.size(), 0) < 0){
        std::cerr << "Error sending message" << std::endl;
        exit(1);
    }
}

std::string socketClient::receiveMessage(){
    char buffer[1024];
    ssize_t bytesToRead =  recv(sockfd, buffer, 1024, 0);
    std::cerr << "recv returned: " << bytesToRead << std::endl;
    if(bytesToRead < 0){
        std::cerr << "Error recieving message" << std::endl;
        exit(1);
    }
    else if(bytesToRead == 0){
        std::cerr << "Connection closed by server" << std::endl;
        exit(1);
    }
    return std::string(buffer, bytesToRead);
}