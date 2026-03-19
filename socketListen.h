#pragma once


#include <iostream>
#include <string>

class socketListen {
    public:
    int myAccept();
    socketListen(int port);
    ~socketListen();

    private:
    int sockfd;
    int port;
    void myBind();
    void myListen();
};
