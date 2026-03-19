#include <iostream>
#include <string>
#include <cstring>
#include "socketListen.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>


socketListen::socketListen(int port){
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        std::cerr << "Error creating socket" << std::endl;
        exit(1);
    }
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    this->port = port;
    myBind();
    myListen();
}

socketListen::~socketListen(){
    close(sockfd);
}


void socketListen::myBind(){
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr)); // zero the struct
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(port);
    if(bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        std::cerr << "Error binding socket" << std::endl;
        exit(1);
    }

}

void socketListen::myListen(){
    if(listen(sockfd, 5) < 0){
        std::cerr << "Error listening on socket" << std::endl;
        exit(1);
    }
}

int socketListen::myAccept(){
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int newsockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
    if(newsockfd < 0){
        std::cerr << "Error accepting connection" << std::endl;
        exit(1);
    }
    return newsockfd;
}