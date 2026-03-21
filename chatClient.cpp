#include "socketClient.h"
#include <thread>
#include <unistd.h>
#include <sys/socket.h>
#include <ncurses.h>
#include <mutex>
#include <string>
#include <cstring>
#include <iostream>   
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <curl/curl.h>
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

struct memoryBuffer {
    unsigned char* data;
    size_t size;
    public:
    int init(){
        data = nullptr;
        size = 0;
        return 0;
    }
};

// Must be a plain function pointer — curl's C API cannot accept a lambda
// passed through curl_easy_setopt's variadic argument on ARM64 (bus error).
static size_t curlWriteCallback(void* contents, size_t size, size_t nmemb, void* userp){
    size_t totalSize = size * nmemb;
    memoryBuffer* memBuffer = (memoryBuffer*)userp;
    unsigned char* newData = (unsigned char*)realloc(memBuffer->data, memBuffer->size + totalSize);
    if(newData == nullptr){
        std::cerr << "Error reallocating memory" << std::endl;
        return 0;
    }
    memBuffer->data = newData;
    memcpy(memBuffer->data + memBuffer->size, contents, totalSize);
    memBuffer->size += totalSize;
    return totalSize;
}

void fetchImage(std::string url, memoryBuffer* buffer){
    buffer->init();
    CURL* curl = curl_easy_init();
    if(curl){
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, buffer);
        CURLcode res = curl_easy_perform(curl);
        if(res != CURLE_OK){
            std::cerr << "Error fetching image: " << curl_easy_strerror(res) << std::endl;
        }
        curl_easy_cleanup(curl);
    }
}

std::mutex winMtx;
// display ASCII images
void imageHandler(int fd, WINDOW* chatWin, std::string address){
    memoryBuffer buffer;
    buffer.init();
    fetchImage(address, &buffer);
    if(buffer.size == 0){
        std::string errorMessage = "Error fetching image.\n";
        send(fd, errorMessage.c_str(), errorMessage.size(), 0);
        return;
    }
    int width, height, channels;
    //scale image down to fit in terminal
    stbi_uc* imgData = stbi_load_from_memory(buffer.data, buffer.size, &width, &height, &channels, 3);
    free(buffer.data); // no longer needed once decoded
    buffer.data = nullptr;
    if(imgData == nullptr){
        std::string errorMessage = "Error loading image.\n";
        send(fd, errorMessage.c_str(), errorMessage.size(), 0);
        return;
    }
    int maxWidth = COLS - 2;
    int maxHeight = LINES - 4;
    float aspectRatio = (float)width / height;
    int newWidth = width;
    int newHeight = height;
    if(newWidth > maxWidth){
        newWidth = maxWidth;
        newHeight = (int)(newWidth / aspectRatio);
    }
    if(newHeight > maxHeight){
        newHeight = maxHeight;
        newWidth = (int)(newHeight * aspectRatio);
    }
    std::cerr << "dims: " << width << "x" << height << " -> " << newWidth << "x" << newHeight << std::endl;
    if(newWidth <= 0 || newHeight <= 0){
        std::string errorMessage = "Image too small to display.\n";
        send(fd, errorMessage.c_str(), errorMessage.size(), 0);
        stbi_image_free(imgData);
        return;
    }
    // Use explicit stride and add 64-byte padding so stb's SIMD over-reads
    // don't walk off the end of the buffer (bus error on misaligned access).
    int outStride = newWidth * 3;
    stbi_uc* resizedData = (stbi_uc*)malloc(outStride * newHeight + 64);
    stbir_resize_uint8_linear(imgData, width, height, width * 3,
                              resizedData, newWidth, newHeight, outStride,
                              STBIR_RGB);
    std::lock_guard<std::mutex> lock(winMtx);
    wprintw(chatWin, "Image from %s:\n", address.c_str());
    std::string result = "";
    for(int i = 0; i < newHeight; i++){
        for(int j = 0; j < newWidth; j++){

            int index = (i * newWidth + j) * 3;
            unsigned char r = resizedData[index];
            unsigned char g = resizedData[index + 1];
            unsigned char b = resizedData[index + 2];
            int gray = (r + g + b) / 3;
            char asciiChar;
            if(gray < 51) asciiChar = '@';
            else if(gray < 102) asciiChar = '#';
            else if(gray < 153) asciiChar = '*';
            else if(gray < 204) asciiChar = '.';
            else asciiChar = ' ';
            result += asciiChar;
            wprintw(chatWin, "%c", asciiChar);  
        }
        result += "\n";
        wprintw(chatWin, "\n");
    }
    send(fd, result.c_str(), result.size(), 0);
    stbi_image_free(imgData);
    free(resizedData);
    wprintw(chatWin, "\n");
    wrefresh(chatWin);
}
void recieveThread(int sockfd, WINDOW* chatWin){
    char buffer[65536];
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
    char message[65536];
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
        if(message[0] == '/' && strncmp(message, "/image ", 7) == 0){
            std::string address(message + 7, strlen(message) - 7);
            imageHandler(sockfd, chatWin, address);
            continue;
        }
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