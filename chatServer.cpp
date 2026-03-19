#include "socketListen.h"
#include <thread>
#include <mutex>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <unistd.h>
#include <algorithm>
#include <cerrno>
#include <map>

std::map<int, std::string> clientFd;
std::mutex mtx;

void broadcast(int fd, const std::string& message){
    std::lock_guard<std::mutex> lock(mtx);
    std::string messageNew = clientFd[fd] + ": " + message;
    for(const auto& pair : clientFd){
        int c = pair.first;
        if(c != fd){
            std::cerr << "Broadcasting to client " << c << std::endl;
            ssize_t result = send(c, messageNew.c_str(), messageNew.size(), 0);
            std::cerr << pair.second << ": " << result << " errno: " << errno << std::endl;
        }
    }
}

void fdHandler(int fd){
    {
        std::lock_guard<std::mutex> lock(mtx);
        clientFd[fd] = "";
    }
    std::string ready = "Connected to chat!\n";
    send(fd, ready.c_str(), ready.size(), 0);

    char buffer[1024];
    ssize_t userSize = recv(fd, buffer, sizeof(buffer), 0);
    if(userSize <= 0){
        std::lock_guard<std::mutex> lock(mtx);
        clientFd.erase(fd);
        close(fd);
        return;
    }
    std::string username(buffer, userSize);
    {
        std::lock_guard<std::mutex> lock(mtx);
        clientFd[fd] = username;
    }
    while(true){
        ssize_t n = recv(fd, buffer, sizeof(buffer), 0);
        if(n <= 0){
            std::lock_guard<std::mutex> lock(mtx);
            clientFd.erase(fd);
            close(fd);
            break;
        }
        std::string message(buffer, n);
        std::cerr << "Received from " << fd << ": " << message << std::endl;
        broadcast(fd, message);
    }
}

int main(){
    std::cout << "\n"
    " ██████╗ ██████╗ ███╗   ██╗██████╗ ██╗   ██╗██╗████████╗\n"
    "██╔════╝██╔═══██╗████╗  ██║██╔══██╗██║   ██║██║╚══██╔══╝\n"
    "██║     ██║   ██║██╔██╗ ██║██║  ██║██║   ██║██║   ██║   \n"
    "██║     ██║   ██║██║╚██╗██║██║  ██║██║   ██║██║   ██║   \n"
    "╚██████╗╚██████╔╝██║ ╚████║██████╔╝╚██████╔╝██║   ██║   \n"
    " ╚═════╝ ╚═════╝ ╚═╝  ╚═══╝╚═════╝  ╚═════╝ ╚═╝   ╚═╝  \n"
    "\n" << std::flush;
    socketListen server(9999);
    std::cerr << "Server listening on port 9999..." << std::endl;
    while(true){
        int client = server.myAccept();
        std::cerr << "Client connected: " << client << std::endl;
        std::thread t(fdHandler, client);
        t.detach();
    }
}