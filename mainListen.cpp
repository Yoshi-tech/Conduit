#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include "socketClient.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include "socketListen.h"

int main(){
    socketListen server(9999);
    int clientSockfd = server.myAccept();
    char buffer[1024];
    ssize_t bytesToRead = recv(clientSockfd, buffer, 1024, 0);
    if(bytesToRead < 0){
        std::cerr << "Error receiving message" << std::endl;
        exit(1);
    }
    else if(bytesToRead == 0){
        std::cerr << "Connection closed by client" << std::endl;
        exit(1);
    }
    std::string message(buffer, bytesToRead);
    std::cout << "Received message: " << message << std::endl;
    close(clientSockfd);

}