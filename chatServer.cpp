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
#include <memory>

std::map<int, std::string> clientFd;
std::mutex mtx;

struct tictactoe{
    char board[3][3];
    int player1Fd;
    int player2Fd;
    int currentPlayerFd;
    bool gameOver;
    bool draw;

    public:
    tictactoe(int p1, int p2) : player1Fd(p1), player2Fd(p2), currentPlayerFd(p1), gameOver(false), draw(false){
        for(int i = 0; i < 3; i++){
            for(int j = 0; j < 3; j++){
                int position = i * 3 + j + 1;
                std::string posStr = std::to_string(position);
                board[i][j] = (posStr.size() == 1) ? posStr[0] : ' '; // Fill with position numbers for easier debugging
            }
        }
    }

    std::string drawSplashScreen(int state){
        if(state == 1){
            return "+------------+\n"
                "|  YOU WIN!  |\n"
                "+------------+\n";
        }
        else if(state == 2){
            return "+------------+\n"
                "|  YOU LOSE! |\n"
                "+------------+\n";
        }
        else{
            return "+------------+\n"
                "|   DRAW!    |\n"
                "+------------+\n";
        }
    }

    std::string move(int fd, int position){
        position -= 1;
        if(gameOver) return "Game is already over.\n";
        if(fd != currentPlayerFd) return "It's not your turn.\n";
        int x = position / 3;
        int y = position % 3;
        if(x < 0 || x > 2 || y < 0 || y > 2) return "Invalid move. Coordinates must be between 0 and 2.\n";
        if(board[x][y] != '1' && board[x][y] != '2' && board[x][y] != '3' && board[x][y] != '4' && board[x][y] != '5' && board[x][y] != '6' && board[x][y] != '7' && board[x][y] != '8' && board[x][y] != '9') return "Invalid move. Cell is already occupied.\n";

        char mark = (fd == player1Fd) ? 'X' : 'O';
        board[x][y] = mark;

        // Check for win
        bool won = false;
        for(int i = 0; i < 3; i++){
            if(board[i][0] == mark && board[i][1] == mark && board[i][2] == mark){ won = true; break; }
            if(board[0][i] == mark && board[1][i] == mark && board[2][i] == mark){ won = true; break; }
        }
        if(!won && board[0][0] == mark && board[1][1] == mark && board[2][2] == mark) won = true;
        if(!won && board[0][2] == mark && board[1][1] == mark && board[2][0] == mark) won = true;

        if(won){
            gameOver = true;
            // winnerSplash and loserSplash are sent separately per-player in fdHandler
            return "WIN:" + std::string(1, mark) + "\n";
        }

        // Check for draw
        draw = true;
        for(int i = 0; i < 3; i++){
            for(int j = 0; j < 3; j++){
                if(board[i][j] == '1' || board[i][j] == '2' || board[i][j] == '3' || board[i][j] == '4' || board[i][j] == '5' || board[i][j] == '6' || board[i][j] == '7' || board[i][j] == '8' || board[i][j] == '9'){ draw = false; break; }
            }
            if(!draw) break;
        }
        if(draw){
            gameOver = true;
            return "DRAW\n";
        }

        // Switch turns
        currentPlayerFd = (currentPlayerFd == player1Fd) ? player2Fd : player1Fd;
        return "Move accepted. Next player's turn.\n";
    }

    std::string renderBoard(){
        std::string rendered = "Current board:\n";
        for(int i = 0; i < 3; i++){
            for(int j = 0; j < 3; j++){
                rendered += board[i][j];
                if(j < 2) rendered += " | ";
            }
            rendered += "\n";
            if(i < 2) rendered += "---------\n";
        }
        return rendered;
    }
};
std::map<int, std::shared_ptr<tictactoe>> games;
std::map<int, int> pendingChallenges;




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

bool acceptHandeler(int fd){
    std::lock_guard<std::mutex> lock(mtx);
    auto it = pendingChallenges.find(fd);
    if(it == pendingChallenges.end()){
        std::string errorMessage = "No pending challenge found.\n";
        send(fd, errorMessage.c_str(), errorMessage.size(), 0);
        return false;
    }
    int challengerFd = it->second;
    games[fd] = std::make_shared<tictactoe>(challengerFd, fd);
    games[challengerFd] = games[fd];
    pendingChallenges.erase(fd);
    std::string acceptMessage = "Challenge accepted! Starting tic tac toe game...\n";
    send(challengerFd, acceptMessage.c_str(), acceptMessage.size(), 0);
    send(fd, acceptMessage.c_str(), acceptMessage.size(), 0);
    return true;
}



void fdHandler(int fd){
    {
        std::lock_guard<std::mutex> lock(mtx);
        clientFd[fd] = "";
    }
    std::string ready = "Connected to chat!\n";
    send(fd, ready.c_str(), ready.size(), 0);

    char buffer[65536];
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
        if(message.substr(0, 5) == "/move"){
            std::lock_guard<std::mutex> lock(mtx);
            auto it = games.find(fd);
            if(it == games.end()){
                std::string errorMessage = "You are not currently in a game.\n";
                send(fd, errorMessage.c_str(), errorMessage.size(), 0);
                continue;
            }
            if(it->second->gameOver){
                std::string errorMessage = "Game is already over.\n";
                send(fd, errorMessage.c_str(), errorMessage.size(), 0);
                continue;
            }
            int position;
            try{
                position = std::stoi(message.substr(6));
            }
            catch(const std::exception& e){
                std::string errorMessage = "Invalid move command. Usage: /move <position>\n";
                send(fd, errorMessage.c_str(), errorMessage.size(), 0);
                continue;
            }
            std::string result = it->second->move(fd, position);

            // Send board to both players first
            std::string board = it->second->renderBoard();
            send(it->second->player1Fd, board.c_str(), board.size(), 0);
            send(it->second->player2Fd, board.c_str(), board.size(), 0);

            // Handle end-of-game splash screens
            if(result.substr(0, 4) == "WIN:"){
                char winMark = result[4];
                int winnerFd = (winMark == 'X') ? it->second->player1Fd : it->second->player2Fd;
                int loserFd  = (winMark == 'X') ? it->second->player2Fd : it->second->player1Fd;
                std::string winMsg  = it->second->drawSplashScreen(1);
                std::string loseMsg = it->second->drawSplashScreen(2);
                send(winnerFd, winMsg.c_str(),  winMsg.size(),  0);
                send(loserFd,  loseMsg.c_str(), loseMsg.size(), 0);
                games.erase(winnerFd);
                games.erase(loserFd);
            }
            else if(result == "DRAW\n"){
                std::string drawMsg = it->second->drawSplashScreen(0);
                send(it->second->player1Fd, drawMsg.c_str(), drawMsg.size(), 0);
                send(it->second->player2Fd, drawMsg.c_str(), drawMsg.size(), 0);
            }
            else{
                // Normal move — only notify the current player
                send(fd, result.c_str(), result.size(), 0);
            }
        }
        else if(message[0] == '/'){
            std::string command = message.substr(1);
            command.find_last_of("\n\r") != std::string::npos ? command.erase(command.find_last_of("\n\r")) : command;
            if(command == "users"){
                std::lock_guard<std::mutex> lock(mtx);
                std::string userList = "Users:\n";
                for(const auto& pair : clientFd){
                    userList += "- " + pair.second + "\n";  
                }
                send(fd, userList.c_str(), userList.size(), 0);
            }
            else if(command == "help"){
                std::string helpMessage = "Commands:\n/users - list users\n/tictactoe <username> - challenge a player\n/accept - accept a challenge\n/move <1-9> - make a move\n/help - show this message\n";
                send(fd, helpMessage.c_str(), helpMessage.size(), 0);
            }
            else if(command.substr(0, 9) == "tictactoe"){
                if(command.size() <= 10){
                    std::string errorMessage = "Usage: /tictactoe <username>\n";
                    send(fd, errorMessage.c_str(), errorMessage.size(), 0);
                    continue;
                }
                std::string opponent = command.substr(10);
                std::lock_guard<std::mutex> lock(mtx);
                auto it = std::find_if(clientFd.begin(), clientFd.end(), [&](const std::pair<int, std::string>& pair){
                    return pair.second == opponent;
                });
                if(it == clientFd.end()){
                    std::string errorMessage = "User not found: " + opponent + "\n";
                    send(fd, errorMessage.c_str(), errorMessage.size(), 0);
                }
                else{
                    int opponentFd = it->first;
                    pendingChallenges[opponentFd] = fd;
                    std::string challengeMessage = "You have been challenged to tic tac toe by " + clientFd[fd] + ". Type /accept to play.\n";
                    send(opponentFd, challengeMessage.c_str(), challengeMessage.size(), 0);
                }
            }
            else if(command == "accept"){
                acceptHandeler(fd);
            }
            else{
                std::string unknownCommand = "Unknown command. Type /help for a list of commands.\n";
                send(fd, unknownCommand.c_str(), unknownCommand.size(), 0);
            }
        }
        else{broadcast(fd, message);}
        
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