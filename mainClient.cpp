#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include "socketClient.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include "socketListen.h"

int main(){
    socketClient client("127.0.0.1", 9999);
    std::string message = "Hello, Server!";
    client.sendMessage(message);
}