#include "socketClient.h"
#include <thread>
#include <unistd.h>
#include <sys/socket.h>
#include <ncurses.h>
#include <mutex>
#include <string>
#include <cstring>
#include <iostream>

std::mutex winMtx;

void recieveThread(int sockfd, WINDOW* chatWin){
    char buffer[1024];
    while(true){
        ssize_t n = recv(sockfd, buffer, sizeof(buffer), 0);
        if(n <= 0){
            std::lock_guard<std::mutex> lock(winMtx);
            wprintw(chatWin, "Disconnected\n");
            wrefresh(chatWin);
            break;
        }
        std::lock_guard<std::mutex> lock(winMtx);
        wprintw(chatWin, "%.*s", (int)n, buffer);
        wrefresh(chatWin);
    }
}

void sendThread(int sockfd, WINDOW* chatWin, WINDOW* inputWin){
    char message[1024];
    while(true){
        {
            std::lock_guard<std::mutex> lock(winMtx);
            wclear(inputWin);
            box(inputWin, 0, 0);
            mvwprintw(inputWin, 0, 2, "[ message ]");
            wmove(inputWin, 1, 1);
            wrefresh(inputWin);
        }
        echo();
        wgetnstr(inputWin, message, COLS - 3);
        noecho();

        if(strlen(message) == 0) continue;

        std::string msg = std::string(message) + "\n";
        send(sockfd, msg.c_str(), msg.size(), 0);
    }
}

int main(int argc, char *argv[]){
    if(argc < 2){
        write(STDERR_FILENO, "Usage: ./chatClient <hostname>\n", 31);
        return 1;
    }

    // show banner before ncurses takes over
    std::cout << "\033[2J\033[H"; // clear terminal
    std::cout << "\033[38;5;39m"; // blue color
    std::cout << "\n";
    std::cout << "  ██████╗ ██████╗ ███╗   ██╗██████╗ ██╗   ██╗██╗████████╗\n";
    std::cout << " ██╔════╝██╔═══██╗████╗  ██║██╔══██╗██║   ██║██║╚══██╔══╝\n";
    std::cout << " ██║     ██║   ██║██╔██╗ ██║██║  ██║██║   ██║██║   ██║   \n";
    std::cout << " ██║     ██║   ██║██║╚██╗██║██║  ██║██║   ██║██║   ██║   \n";
    std::cout << " ╚██████╗╚██████╔╝██║ ╚████║██████╔╝╚██████╔╝██║   ██║   \n";
    std::cout << "  ╚═════╝ ╚═════╝ ╚═╝  ╚═══╝╚═════╝  ╚═════╝ ╚═╝   ╚═╝  \n";
    std::cout << "\033[0m"; // reset color
    std::cout << "\n";
    std::cout << "\033[38;5;245m  local terminal chat · connecting to " << argv[1] << "\033[0m\n";
    std::cout << "\n";
    std::cout.flush();

    // get username before ncurses
    std::cout << "\033[38;5;39m  username: \033[0m";
    std::string username;
    std::getline(std::cin, username);
    std::cout << "\n  connecting...\n";
    std::cout.flush();

    // connect
    socketClient client(argv[1], 9999);
    send(client.sockfd, username.c_str(), username.size(), 0);

    sleep(1); // brief pause so user sees the banner

    // now start ncurses
    initscr();
    cbreak();
    noecho();
    curs_set(1);
    start_color();

    // colors
    init_pair(1, COLOR_CYAN, COLOR_BLACK);    // messages
    init_pair(2, COLOR_WHITE, COLOR_BLACK);   // input
    init_pair(3, COLOR_BLUE, COLOR_BLACK);    // border

    WINDOW* chatWin = newwin(LINES - 3, COLS, 0, 0);
    WINDOW* inputWin = newwin(3, COLS, LINES - 3, 0);

    scrollok(chatWin, TRUE);
    keypad(inputWin, TRUE);

    // draw chat window border
    wattron(chatWin, COLOR_PAIR(3));
    box(chatWin, 0, 0);
    mvwprintw(chatWin, 0, 2, "[ conduit ]");
    wattroff(chatWin, COLOR_PAIR(3));
    wrefresh(chatWin);

    // welcome message
    wattron(chatWin, COLOR_PAIR(1));
    mvwprintw(chatWin, 1, 2, "connected as: %s", username.c_str());
    mvwprintw(chatWin, 2, 2, "------------------------------");
    wattroff(chatWin, COLOR_PAIR(1));
    wrefresh(chatWin);

    // create inner chat window for scrolling messages
    WINDOW* msgWin = newwin(LINES - 7, COLS - 2, 3, 1);
    scrollok(msgWin, TRUE);
    wrefresh(msgWin);

    std::thread r(recieveThread, client.sockfd, msgWin);
    std::thread s(sendThread, client.sockfd, msgWin, inputWin);
    r.join();
    s.join();

    endwin();
    return 0;
}