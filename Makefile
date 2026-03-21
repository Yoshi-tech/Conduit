CXX = g++
CXXFLAGS = -std=c++11 -pthread
NCURSES_FLAGS = $(shell pkg-config --cflags --libs ncurses)
CURL_FLAGS = -I/opt/homebrew/opt/curl/include -L/opt/homebrew/opt/curl/lib -lcurl
all: server client

server: chatServer.cpp socketListen.cpp
	$(CXX) $(CXXFLAGS) chatServer.cpp socketListen.cpp -o chatServer

client: chatClient.cpp socketClient.cpp
	$(CXX) $(CXXFLAGS) chatClient.cpp socketClient.cpp $(NCURSES_FLAGS) $(CURL_FLAGS) -o chatClient

clean:
	rm -f chatServer chatClient
