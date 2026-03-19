CXX = g++
CXXFLAGS = -std=c++11 -pthread
NCURSES_FLAGS = -L/opt/homebrew/opt/ncurses/lib -I/opt/homebrew/opt/ncurses/include -lncurses

all: server client

server: chatServer.cpp socketListen.cpp
	$(CXX) $(CXXFLAGS) chatServer.cpp socketListen.cpp -o chatServer

client: chatClient.cpp socketClient.cpp
	$(CXX) $(CXXFLAGS) $(NCURSES_FLAGS) chatClient.cpp socketClient.cpp -o chatClient

clean:
	rm -f chatServer chatClient
