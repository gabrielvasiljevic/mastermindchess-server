#include <SFML/Network.hpp>
#include <SFML/System.hpp>
#include <memory>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <queue>
#include <map>
#include <chrono>
#include <ctime>

#define GAME_VERSION "0.1.5"

using namespace std;
using namespace sf;
                    //0     1     2       3       4         5            6          7          8            9           10     11
enum class packetID {None, Name, Move, Connect, Login, LoginResponse, GameEnd, Disconnect, Register, RegisterResponse, Chat, GameRequest,
                    Response, Options, Turn, MatchLog, Check, Checkmate, KeepAlive, MoveLog, WrongVersion, PlayerList, CapaGameRequest, CapaConnect, CapaOptions,
                    GameEndTimeOut};
enum class statusID {offline, online, playing};

struct userInfo {
    string id;
    string username;
    string nickname;
    string password;
    string email;
    string registerdata;
    string active;
    string matches;
    statusID status;
    userInfo(string _id, string _username, string _nickname, string _password, string _email, string _registerdata){
        id           = _id;
        username     = _username;
        nickname     = _nickname;
        password     = _password;
        email        = _email;
        registerdata = _registerdata;
        active       = "1";
        status       = statusID::offline;
    }
    userInfo(){status = statusID::offline;}
};




TcpListener                       listener;
vector<unique_ptr<TcpSocket>>     players;
vector<userInfo>                  userlist;
vector<sf::Clock>                 playersTime;

map<int, int>                     Opponents;
map<int, string>                  accountsIDs;
map<string, int>                  IDsAccounts;


queue<int>                        classicQueue;
queue<int>                        capaQueue;

sf::Clock sendListTime;

constexpr int timeout = 120;
int waitingPlayer;
int numberOfUsers = 0;
bool waiting = false;
int playerID;



void logHistory (string action, string playerName);
string getTimestamp();

void loadUserInformation(){
    ifstream input {"data/users.dat"};
    string user;
    int i;
    userlist.emplace_back();
   // input.ignore('\n');
    while(getline(input, userlist[numberOfUsers].id,           ';')){
          getline(input, userlist[numberOfUsers].username,     ';');
          getline(input, userlist[numberOfUsers].nickname,     ';');
          getline(input, userlist[numberOfUsers].password,     ';');
          getline(input, userlist[numberOfUsers].email,        ';');
          getline(input, userlist[numberOfUsers].registerdata, ';');
          getline(input, userlist[numberOfUsers].active,       ';');
          numberOfUsers++;
          if(input.peek() != EOF) userlist.emplace_back();
    }
}

void acceptNewPlayers() {
    if (listener.accept(*players.back()) == Socket::Done) {
        players.back()->setBlocking (false);
        cout << "player " << static_cast<int> (players.size()) << " connected!\n";
        playersTime.emplace_back();
        players.emplace_back (new TcpSocket());
    }
}


void saveAccounts() {
    cout << "Saving users..." << endl;
    ofstream output {"data/users.dat"};

    for (auto& user : userlist)
        output << user.id << ";"
               << user.username     << ";"
               << user.nickname     << ";"
               << user.password     << ";"
               << user.email        << ";"
               << user.registerdata << ";"
               << user.active       << ";\n";
    output << std::flush;
    cout << "Users data saved!" << endl;
}


void registerAccount(string username, string password, string nickname, string email) {
    userlist.emplace_back();
    userlist[numberOfUsers].id          = std::to_string(numberOfUsers);
    userlist[numberOfUsers].username    = username;
    userlist[numberOfUsers].nickname    = nickname;
    userlist[numberOfUsers].password    = password;
    userlist[numberOfUsers].email       = email;
    userlist[numberOfUsers].active      = "1";
    userlist[numberOfUsers].registerdata= getTimestamp();
    numberOfUsers++;
    saveAccounts();
}


void sendLoginResponse (int playerID, string ulogin, string upassword, string version) {
    Packet packet;
    int i;
    bool foundUser = false;
    if(version == GAME_VERSION){
       for(i = 0; i < userlist.size(); i++){
            if(userlist[i].username == ulogin){
                foundUser = true;
                break;
            }
        }
        if(!foundUser){
            logHistory ("not found", ulogin);
            packet << playerID << static_cast<int> (packetID::LoginResponse) << false;
            players[playerID]->send(packet);
            return;
        }
        if(foundUser && userlist[i].password == upassword){
            accountsIDs[playerID] = userlist[i].username;
            IDsAccounts[userlist[i].username] = playerID;
            userlist[i].status = statusID::online;
            cout << "Successful login from player " << userlist[i].username << "\n";

            logHistory ("logon", userlist[i].username);
            packet << playerID << static_cast<int> (packetID::LoginResponse) << true << userlist[i].username;
            players[playerID]->send(packet);
            return;
        }
        logHistory ("incorrect password", userlist[i].username);
        packet << playerID << static_cast<int> (packetID::LoginResponse) << false;
        players[playerID]->send(packet);
    }
    else{
        packet << playerID << static_cast<int> (packetID::WrongVersion);
        players[playerID]->send(packet);
    }
}

void sendRegisterResponse (int fromPlayer, string uusername, string upassword, string unickname, string uemail) { //change to the new style
    Packet packet;
    int i;
    for(i = 0; i < userlist.size(); i++){
        if(userlist[i].username == uusername ||
           userlist[i].nickname == unickname ||
           userlist[i].email    == uemail){
            packet << fromPlayer << static_cast<int> (packetID::RegisterResponse) << false;
            players[fromPlayer]->send(packet);
            return;
        }
    }
    registerAccount(uusername, upassword, unickname, uemail);
    cout << "Successful register from player " << unickname << "\n";
    logHistory ("new account", unickname);
    packet << fromPlayer << static_cast<int> (packetID::RegisterResponse) << true;
    players[fromPlayer]->send(packet);
    return;
}

void sendPlayerList(){
    Packet packet;
    packet << -1 << static_cast<int> (packetID::PlayerList);

    packet << static_cast<int> (userlist.size());
    for (auto& user : userlist) {
        packet << user.nickname << static_cast<int> (user.status);
    }
    for(auto& player : players)
        player->send(packet);

    cout << "Sended player list!\n";
}

void sendOptions(int playerID, int toPlayer){
    Packet packet;
    packet << playerID << static_cast<int> (packetID::Options);
    players[toPlayer]->send(packet);
    cout << "Sended options to " << toPlayer << endl;
}

void sendCapaOptions(int playerID, int toPlayer){
    Packet packet;
    packet << playerID << static_cast<int> (packetID::CapaOptions);
    players[toPlayer]->send(packet);
    cout << "Sended options to " << toPlayer << endl;
}

void sendMove(int playerID, int toPlayer, int i, int j, int iP, int jP, bool check){
    Packet packet;
    packet << playerID << static_cast<int> (packetID::Move);
    packet << i << j << iP << jP << check;
    players[toPlayer]->send(packet);
    cout << "Sended move to " << toPlayer << endl;
}

void sendMoveLog(int playerID, int toPlayer, string moveLog){
    Packet packet;
    packet << playerID << static_cast<int> (packetID::MoveLog);
    packet << moveLog;
    players[toPlayer]->send(packet);
    cout << "Sended movelog to " << toPlayer << endl;

}

void sendGameEnd(int playerID, int toPlayer){
    Packet packet;
    packet << playerID << static_cast<int> (packetID::GameEnd);

    players[toPlayer]->send(packet);
    cout << "Sended GameEnd to " << toPlayer << endl;
}

void sendCheckMate(int playerID, int toPlayer){
    Packet packet;
    packet << playerID << static_cast<int> (packetID::Checkmate);

    players[toPlayer]->send(packet);

    cout << "Sended CheckMate to " << toPlayer << endl;
}

void sendChatMessage(int playerID, int toPlayer, string msg){
    Packet packet;
    packet << playerID << static_cast<int> (packetID::Chat);
    auto it = accountsIDs.find(playerID);
    string name = it->second;
    packet << name + ": " + msg;
    players[toPlayer]->send(packet);
}

void sendGameRequest(int playerID, int toPlayer){
    int i;
    string name;
    Packet packet;
    packet << playerID << static_cast<int>(packetID::GameRequest);
    auto it = accountsIDs.find(playerID);
    name = it->second;
    for(auto& player : userlist){
        if(player.status == statusID::online){
            if(player.username != name){
                auto it2 = IDsAccounts.find(player.username);
                toPlayer = it2->second;
            }

        }
    }
    players[toPlayer]->send(packet);
    cout << "Sended game request to " << toPlayer << endl;
}

void sendCapaGameRequest(int playerID, int toPlayer){
    int i;
    string name;
    Packet packet;
    packet << playerID << static_cast<int>(packetID::CapaGameRequest);
    auto it = accountsIDs.find(playerID);
    name = it->second;
    for(auto& player : userlist){
        if(player.status == statusID::online){
            if(player.username != name){
                auto it2 = IDsAccounts.find(player.username);
                toPlayer = it2->second;
            }

        }
    }
    players[toPlayer]->send(packet);
    cout << "Sended capagame request to " << toPlayer << endl;
}

void handleClientRequest(Packet packet, int pktID, int playerID, int toPlayer) {
    int i;
    auto it = accountsIDs.find(toPlayer);
    auto myID = accountsIDs.find(playerID);
    string movelog;
    string msg;
    switch (static_cast<packetID> (pktID)) {
        case packetID::Connect :
            cout << it->first << " : " << it->second << endl;
            cout << "Estabilishing connection to player " << it->second << "....\n";
            for(i = 0; i < userlist.size(); i++){
                if(it->second == userlist[i].username && userlist[i].status == statusID::online){
                   packet.clear();
                    packet << playerID << static_cast<int> (packetID::Connect);

                    players[toPlayer]->send(packet);
                    cout << "Connect from player " << playerID << " to player " << toPlayer << "\n";
                    userlist[i].status = userlist[toPlayer].status = statusID::playing;
                    packet.clear();
                    packet << playerID << static_cast<int> (packetID::Turn);
                    players[playerID]->send(packet);

                    Opponents[playerID] = toPlayer;
                    Opponents[toPlayer] = playerID;
                }
            }

            break;
        case packetID::Login : {
            string ulogin, upassword, version;
            packet >> ulogin >> upassword >> version;
            if(ulogin.size() > 0 && upassword.size() > 0)
                sendLoginResponse (playerID, ulogin, upassword, version);
            break;
        }
        case packetID::Register : {
            string uusername, upassword, unickname, uemail;
            packet >> uusername >> upassword >> unickname >> uemail;
            sendRegisterResponse(playerID, uusername, upassword, unickname, uemail);
            break;
        }
        case packetID::Move:
            int i, j, iP, jP;
            bool check;
            packet >> i >> j >> iP >> jP >> check;
            cout << "Movement from player " << playerID << endl;
            toPlayer = Opponents[playerID];
            sendMove(playerID, toPlayer, i, j, iP, jP, check);
        break;

        case packetID::Chat:
            packet >> msg;
            toPlayer = Opponents[playerID];
            sendChatMessage(playerID, toPlayer, msg);
        break;

        case packetID::Checkmate: {
                toPlayer = Opponents[playerID];
                auto it2 = accountsIDs.find(toPlayer);
                cout << "Checkmate from player " << playerID << endl;
                sendCheckMate(playerID, toPlayer);
                for(i = 0; i < userlist.size(); i++){
                    if(userlist[i].username == myID->second){
                        userlist[i].status = statusID::online;
                    }
                    if(userlist[i].username == it2->second){
                        userlist[i].status = statusID::online;
                    }
                }
            }
        break;

        case packetID::GameEnd : {
            cout << "GameEnd from player " << playerID << endl;
            for(i = 0; i < userlist.size(); i++){
                if(userlist[i].username == myID->second){
                    userlist[i].status = statusID::online;
                }
            }

            sendGameEnd(playerID, toPlayer);
        break;
        }
        case packetID::GameEndTimeOut : {
            int loserID;
            packet >> loserID;
            toPlayer = loserID;
            cout << "GameEndTimeout from player " << playerID << endl;
            for(i = 0; i < userlist.size(); i++){
                if(userlist[i].username == myID->second){
                    userlist[i].status = statusID::online;
                }
            }

            sendGameEnd(playerID, toPlayer);
        break;
        }

        case packetID::PlayerList:
            sendPlayerList();
        break;

        case packetID::Disconnect : {
            auto findDisconnect = accountsIDs.find(playerID);
            for(i = 0; i < userlist.size(); i++){
                if(userlist[i].username == findDisconnect->second){
                    cout << "Player " << userlist[i].username << " disconnected\n";
                    if (userlist[i].status == statusID::playing) {
                        sendGameEnd(playerID, Opponents[playerID]);
                    }
                    userlist[i].status = statusID::offline;
                    players[playerID]->disconnect();
                    //accountsIDs.erase(playerID);
                    //IDsAccounts.erase(findDisconnect->second);
                    logHistory ("logout", userlist[i].username);
                }
            }
            break;
        }
        case packetID::GameRequest:
            if(classicQueue.size() > 0){
                int enemy = classicQueue.front();
                classicQueue.pop();
                cout << "Estabilishing connection to player " << enemy << "...\n";
                auto findEnemy = accountsIDs.find(enemy);
                auto findNew = accountsIDs.find(playerID);
                cout << "enemy name:" << findEnemy->second << endl;
                for(i = 0; i < userlist.size(); i++){
                    if(userlist[i].username == findEnemy->second && userlist[i].status == statusID::online){
                        packet.clear();
                        packet << playerID << static_cast<int> (packetID::Connect);
                        players[enemy]->send(packet);
                        sendOptions(enemy, playerID);

                        packet.clear();
                        packet << enemy << static_cast<int> (packetID::Connect);
                        players[playerID]->send(packet);
                        for(int j = 0; j < userlist.size(); j++){
                            if(findNew->second == userlist[j].username && userlist[j].status == statusID::online){
                                cout << "Connect from player " << userlist[j].username << "(" << playerID << ")"
                                << " to player " << findEnemy->second << "(" << enemy << ")" << "\n";
                                userlist[j].status = userlist[i].status = statusID::playing;
                            }
                        }

                        Opponents[playerID] = enemy;
                        Opponents[enemy] = playerID;
                        waiting = false;
                    }
                }
                cout << classicQueue.front() << endl;
            }
            else classicQueue.push(playerID);
        break;
        case packetID::CapaGameRequest:
            if(capaQueue.size() > 0){
                int enemy = capaQueue.front();
                capaQueue.pop();
                cout << "Estabilishing connection to player " << enemy << "...\n";
                auto findEnemy = accountsIDs.find(enemy);
                auto findNew = accountsIDs.find(playerID);
                cout << "enemy name:" << findEnemy->second << endl;
                for(i = 0; i < userlist.size(); i++){
                    if(userlist[i].username == findEnemy->second && userlist[i].status == statusID::online){
                        packet.clear();
                        packet << playerID << static_cast<int> (packetID::CapaConnect);
                        players[enemy]->send(packet);
                        sendCapaOptions(enemy, playerID);

                        packet.clear();
                        packet << enemy << static_cast<int> (packetID::CapaConnect);
                        players[playerID]->send(packet);
                        for(int j = 0; j < userlist.size(); j++){
                            if(findNew->second == userlist[j].username && userlist[j].status == statusID::online){
                                cout << "Connect from player " << userlist[j].username << "(" << playerID << ")"
                                << " to player " << findEnemy->second << "(" << enemy << ")" << "\n";
                                userlist[j].status = userlist[i].status = statusID::playing;
                            }
                        }

                        Opponents[playerID] = enemy;
                        Opponents[enemy] = playerID;
                        waiting = false;
                    }
                }
            }
            else capaQueue.push(playerID);
        break;
        case packetID::MatchLog :
        //    string log;
        //    string player1, player2;
        //    packet >> log;
        //    playersEnemy[fromPlayer];
        break;
        case packetID::MoveLog:
            packet >> movelog;
            cout << "Movement log from player " << playerID << endl;
            toPlayer = Opponents[playerID];
            sendMoveLog(playerID, toPlayer, movelog);
        break;
        default : { //redirect
            if (toPlayer < 0 || toPlayer > players.size() -1) return;
            players[toPlayer]->send(packet);

            return; //do not update player list
        }

    }
    //Since a new player might have entered, or 2 of them are now playing,
    //it's necessary to send a packet updating the Playerlist;
    if (playerID >= 0)
        playersTime[playerID].restart();

}

string getTimestamp () {
    using namespace std::chrono;
    #define padIt(_x_) (now_in._x_ < 10 ? "0" : "") << now_in._x_

    ostringstream output;
    system_clock::time_point now = system_clock::now();
    time_t now_c = system_clock::to_time_t(now);
    tm now_in = *(localtime(&now_c));
    output << ""
           <<  padIt(tm_mday)    << "/"
           <<  padIt(tm_mon + 1) << "/"
           <<  now_in.tm_year + 1900 << " "
           <<  padIt(tm_hour) << ":" << padIt(tm_min) << ":" << padIt(tm_sec)
           << "";
    return output.str();
}

void logHistory (string action, string playerName) {
    ofstream output {"logs/log.dat", ios::app};
    output <<  getTimestamp() << " " << playerName << " " << action  << "\n";
    output << std::flush;
}

void logMatches(string log, string player1, string player2){

}

void checkTimeout(){
    int i;
    for (i = 0; i < playersTime.size(); i++) {
        if((playersTime[i].getElapsedTime().asSeconds() >= timeout) && (userlist[i].status != statusID::offline)) {
            if(userlist[i].status == statusID::playing)
                sendGameEnd(i, Opponents[i]);
            if(userlist[Opponents[i]].status == statusID::playing)
                sendGameEnd(Opponents[i], i);

            logHistory ("disconected(t.o.)", userlist[i].username);
            userlist[i].status = statusID::offline;
            players[i]->disconnect();
        }
    }
}


int main(){
    int toPlayer, fromPlayer, pktID;
    Packet packet;
    listener.setBlocking(false);
    string message;
    cout << "Server started!" << endl;
    ifstream config {"data/config.dat"};
    int port;
    if (config.is_open())
        config >> port;
    //int port = 14193;
    while (listener.listen (port) != Socket::Done);
    cout << "Successfully binded to port " << port << endl;
    players.emplace_back (new TcpSocket());
    loadUserInformation();
    logHistory ("System initialized at port " + to_string(port), "");
    while (true) {
        acceptNewPlayers();
        playerID = 0;
        for (auto& player : players){
            if (player->receive(packet) == Socket::Done) {
                packet >> toPlayer >> pktID;
                handleClientRequest(packet, pktID, playerID, toPlayer);
            }
            playerID++;
        }
        if(sendListTime.getElapsedTime().asSeconds() >= 10){
            sendPlayerList();
            sendListTime.restart();
        }
       // checkTimeout();
    }
    system("PAUSE");

}
