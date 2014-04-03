#include <SFML/Network.hpp>
#include <SFML/System.hpp>
#include <memory>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <chrono>
#include <ctime>

using namespace std;
using namespace sf;
                    //0     1     2       3       4         5            6          7          8            9           10     11
enum class packetID {None, Name, Move, Connect, Login, LoginResponse, GameEnd, Disconnect, Register, RegisterResponse, Chat, GameRequest,
                    Response, Options, Turn, MatchLog, Check, Checkmate};
enum class statusID {offline, online, playing};

struct userInfo {
    string id;
    string username;
    string nickname;
    string password;
    string email;
    string registerdata;
    string status;
    userInfo(string _id, string _username, string _nickname, string _password, string _email, string _registerdata){
        id           = _id;
        username     = _username;
        nickname     = _nickname;
        password     = _password;
        email        = _email;
        registerdata = _registerdata;
        status       = "1";
    }
    userInfo(){}
};

struct playerInfo {
    int id;
    std::string name;
    statusID status;
    playerInfo (int _id, std::string _name, statusID _status) {id = _id; name = _name; status = _status;}
    playerInfo () {}
};




TcpListener                       listener;
vector<unique_ptr<TcpSocket>>     players;
vector<int>                       playersEnemy;
vector<playerInfo>                playerlist;
vector<userInfo>                  userlist;
map<string, string>               accounts;
map<int, string>                  accountsIDs;
vector<sf::Clock>                 playersTime;

constexpr int timeout = 120;
int waitingPlayer;
int numberOfUsers = 0;
bool waiting = false;

void logHistory (string action, string playerName);
string getTimestamp();

void loadUserInformation(){
    ifstream input {"data/users2.dat"};
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
          getline(input, userlist[numberOfUsers].status,       ';');
          numberOfUsers++;
          if(input.peek() != EOF) userlist.emplace_back();
    }
}

void acceptNewPlayers() {
    if (listener.accept(*players.back()) == Socket::Done) {
        players.back()->setBlocking (false);
        playerlist.emplace_back (static_cast<int>(players.size() - 1), "unknown", statusID::offline);
        playersEnemy.emplace_back(-1);
        cout << "player " << static_cast<int> (players.size()) << " connected!\n";
        playersTime.emplace_back();
        players.emplace_back (new TcpSocket());
    }
}

void loadAccounts(){
    int i;
    for(i = 0; i < numberOfUsers; i++){
        accounts[userlist[i].username] = userlist[i].password;
    }
}


void saveAccounts() {
    ofstream output {"data/users2.dat"};

    for (auto& user : userlist)
        output << user.id << ";"
               << user.username     << ";"
               << user.nickname     << ";"
               << user.password     << ";"
               << user.email        << ";"
               << user.registerdata << ";"
               << user.status       << ";";
    output << std::flush;
}


void registerAccount2(string username, string nickname, string password, string id, string email) {
    userlist.emplace_back();
    userlist[numberOfUsers].id          = id;
    userlist[numberOfUsers].username    = username;
    userlist[numberOfUsers].nickname    = nickname;
    userlist[numberOfUsers].password    = password;
    userlist[numberOfUsers].email       = email;
    userlist[numberOfUsers].status      = "1";
    userlist[numberOfUsers].registerdata= getTimestamp();
    numberOfUsers++;
    accounts[username] = password;
    saveAccounts();
}

void registerAccount (string ulogin, string upassword) {
    accounts[ulogin] = upassword;
    saveAccounts();
}

void sendLoginResponse (int fromPlayer, string ulogin, string upassword) {
    Packet packet;
    int i;
    bool foundUser = false;
    for(i = 0; i < userlist.size(); i++){
        if(userlist[i].username == ulogin){
            foundUser = true;
            break;
        }
    }
    if(foundUser && userlist[i].password == upassword){
        playerlist[fromPlayer].name = userlist[i].username;
        playerlist[fromPlayer].status = statusID::online;
        accountsIDs[fromPlayer] = userlist[i].id;
        cout << "Successful login from player " << playerlist[fromPlayer].name << "\n";

        logHistory ("logon", playerlist[fromPlayer].name);
        packet << fromPlayer << static_cast<int> (packetID::LoginResponse) << true;
        players[fromPlayer]->send(packet);
        return;
    }
    //only goes through here in case of login failure
    packet << fromPlayer << static_cast<int> (packetID::LoginResponse) << false;
    players[fromPlayer]->send(packet);
}

void sendRegisterResponse (int fromPlayer, string ulogin, string upassword) { //change to the new style
    Packet packet;
    auto it = accounts.find (ulogin);
    if (it == accounts.end()) {
        registerAccount (ulogin, upassword);
        cout << "Successful register from player " << fromPlayer << "\n";

        packet << fromPlayer << static_cast<int> (packetID::RegisterResponse) << true;
        players[fromPlayer]->send(packet);
        return;
    }
    //only goes through here in case of register failure
    packet << fromPlayer << static_cast<int> (packetID::RegisterResponse) << false;
    players[fromPlayer]->send(packet);
}

void sendOptions(int fromPlayer, int toPlayer){
    Packet packet;
    packet << fromPlayer << static_cast<int> (packetID::Options);
    players[toPlayer]->send(packet);
    cout << "Sended options to " << toPlayer << endl;
}

void sendMove(int fromPlayer, int toPlayer, int i, int j, int iP, int jP, bool check){
    Packet packet;
    packet << fromPlayer << static_cast<int> (packetID::Move);
    packet << i << j << iP << jP << check;
    players[toPlayer]->send(packet);
    cout << "Sended move to " << toPlayer << endl;
}

void sendGameEnd(int fromPlayer, int toPlayer){
    Packet packet;
    packet << fromPlayer << static_cast<int> (packetID::GameEnd);

    players[toPlayer]->send(packet);
    // if (playerlist[toPlayer].status != statusID::offline)
    //     playerlist[toPlayer].status = statusID::online;
    cout << "Sended GameEnd to " << toPlayer << endl;
}

void sendCheckMate(int fromPlayer, int toPlayer){
    Packet packet;
    packet << fromPlayer << static_cast<int> (packetID::Checkmate);

    players[toPlayer]->send(packet);

    cout << "Sended CheckMate to " << toPlayer << endl;
}

void sendGameRequest(int fromPlayer, int toPlayer){
    Packet packet;
    packet << fromPlayer << static_cast<int>(packetID::GameRequest);
    for(auto& player : playerlist){
        if(player.status == statusID::online && player.id != fromPlayer){
            toPlayer = player.id;
        }
    }
    players[toPlayer]->send(packet);
    cout << "Sended game request to " << toPlayer << endl;
}

void handleClientRequest(Packet packet, int pktID, int fromPlayer, int toPlayer) {
    switch (static_cast<packetID> (pktID)) {
        case packetID::Connect :
            cout << "Estabilishing connection to player " << toPlayer << "...\n";
            if (playerlist[toPlayer].status == statusID::online) {
                packet.clear();
                packet << fromPlayer << static_cast<int> (packetID::Connect);
                playerlist[fromPlayer].status = playerlist[toPlayer].status = statusID::playing;
                players[toPlayer]->send(packet);
                cout << "Connect from player " << fromPlayer << " to player " << toPlayer << "\n";

                packet.clear();
                packet << fromPlayer << static_cast<int> (packetID::Turn);
                players[fromPlayer]->send(packet);

                playersEnemy[fromPlayer] = toPlayer;
                playersEnemy[toPlayer] = fromPlayer;
            }
            break;
        case packetID::Login : {
            string ulogin, upassword;
            packet >> ulogin >> upassword;
            if(ulogin.size() > 0 && upassword.size() > 0)
                sendLoginResponse (fromPlayer, ulogin, upassword);
            break;
        }
        case packetID::Register : {
            string ulogin, upassword;
            packet >> ulogin >> upassword;
            sendRegisterResponse(fromPlayer, ulogin, upassword);
            break;
        }
        case packetID::Move:
            int i, j, iP, jP;
            bool check;
            packet >> i >> j >> iP >> jP >> check;
            cout << "Movement from player " << fromPlayer << endl;
            toPlayer = playersEnemy[fromPlayer];
            sendMove(fromPlayer, toPlayer, i, j, iP, jP, check);
        break;
        case packetID::Checkmate:
            toPlayer = playersEnemy[fromPlayer];
            cout << "Checkmate from player " << fromPlayer << endl;
            sendCheckMate(fromPlayer, toPlayer);
        break;
        case packetID::GameEnd : {
            cout << "GameEnd from player " << fromPlayer << endl;
            playerlist[fromPlayer].status = statusID::online;
            sendGameEnd(fromPlayer, toPlayer);
        break;
        }
        case packetID::Disconnect : {
            cout << "Player " << playerlist[fromPlayer].name << " disconnected\n";
            if (playerlist[fromPlayer].status == statusID::playing) {
                sendGameEnd(fromPlayer, playersEnemy[fromPlayer]);
            }
            playerlist[fromPlayer].status = statusID::offline; //not ideal... should remove from playerlist
            players[fromPlayer]->disconnect();
            logHistory ("logout", playerlist[i].name);
            break;
        }
        case packetID::GameRequest:
            if(!waiting){
                waiting = true;
                waitingPlayer = fromPlayer;
            }
            else{
                cout << "Estabilishing connection to player " << waitingPlayer << "...\n";
                if (playerlist[waitingPlayer].status == statusID::online) {
                    packet.clear();
                    packet << fromPlayer << static_cast<int> (packetID::Connect);
                    playerlist[fromPlayer].status = playerlist[waitingPlayer].status = statusID::playing;
                    players[waitingPlayer]->send(packet);
                    cout << "Connect from player " << fromPlayer << " to player " << waitingPlayer << "\n";
                    sendOptions(waitingPlayer, fromPlayer);
                    packet.clear();
                    packet << fromPlayer << static_cast<int> (packetID::Connect);
                    players[fromPlayer]->send(packet);
                    playersEnemy[fromPlayer] = waitingPlayer;
                    playersEnemy[waitingPlayer] = fromPlayer;
                    waiting = false;
                }
            }
        break;
        case packetID::MatchLog :
        //    string log;
        //    string player1, player2;
        //    packet >> log;
        //    playersEnemy[fromPlayer];
        break;
        default : { //redirect
            if (toPlayer < 0 || toPlayer > players.size() -1) return;
            players[toPlayer]->send(packet);

            return; //do not update player list
        }

    }
    //Since a new player might have entered, or 2 of them are now playing,
    //it's necessary to send a packet updating the Playerlist;
    if (fromPlayer >= 0)
        playersTime[fromPlayer].restart();
}

string getTimestamp () {
    using namespace std::chrono;
    #define padIt(_x_) (now_in._x_ < 10 ? "0" : "") << now_in._x_

    ostringstream output;
    system_clock::time_point now = system_clock::now();
    time_t now_c = system_clock::to_time_t(now);
    tm now_in = *(localtime(&now_c));
    output << "["
           <<  padIt(tm_hour) << ":" << padIt(tm_min) << " "
           <<  padIt(tm_mday)    << "-"
           <<  padIt(tm_mon + 1) << "-"
           <<  now_in.tm_year + 1900
           << "]";
    return output.str();
}

void logHistory (string action, string playerName) {
    ofstream output {"data/log.dat", ios::app};
    output << playerName << " " << action << " " << getTimestamp() << "\n";
    output << std::flush;
}

void logMatches(string log, string player1, string player2){

}

void checkTimeout(){
    int i;
    for (i = 0; i < playersTime.size(); i++) {
        if((playersTime[i].getElapsedTime().asSeconds() >= timeout) && (playerlist[i].status != statusID::offline)) {
            if(playerlist[i].status == statusID::playing)
                sendGameEnd(i, playersEnemy[i]);
            if(playerlist[playersEnemy[i]].status == statusID::playing)
                sendGameEnd(playersEnemy[i], i);

            logHistory ("disconected(t.o.)", playerlist[i].name);
            playerlist[i].status = statusID::offline;
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
    loadAccounts();
    while (true) {
        acceptNewPlayers();
        fromPlayer = 0;
        for (auto& player : players) {
            if (player->receive(packet) == Socket::Done) {
                packet >> toPlayer >> pktID;
                handleClientRequest(packet, pktID, fromPlayer, toPlayer);
            }
            ++fromPlayer;
        }
       // checkTimeout();
    }
    system("PAUSE");

}
