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
enum class packetID {None, Name, Move, Connect, Login, LoginResponse, GameEnd, Disconnect, Register, RegisterResponse, Chat, GameRequest, Response, Options, Turn};
enum class statusID {offline, online, playing};


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
map<string, string>               accounts;

constexpr int timeout = 10;
int waitingPlayer;
bool waiting = false;

void acceptNewPlayers() {
    if (listener.accept(*players.back()) == Socket::Done) {
        players.back()->setBlocking (false);
        playerlist.emplace_back (static_cast<int>(players.size() - 1), "unknown", statusID::offline);
        playersEnemy.emplace_back(-1);
        cout << "player " << static_cast<int> (players.size()) << " connected!\n";

        players.emplace_back (new TcpSocket());
    }
}


void loadAccounts () {
    ifstream input {"data/users.dat"};
    string user, pass;
    if (input.is_open())
        while (getline (input, user))
            if (getline (input, pass))
                accounts[user] = pass;
}

void saveAccounts () {
    ofstream output {"data/users.dat"};
    for (auto& user : accounts)
        output << user.first << "\n" << user.second << "\n";
    output << std::flush;
}

void registerAccount (string ulogin, string upassword) {
    accounts[ulogin] = upassword;
    saveAccounts();
}

void sendLoginResponse (int fromPlayer, string ulogin, string upassword) {
    Packet packet;
    auto it = accounts.find (ulogin);
    if(it != accounts.end()){
        if(it->second == upassword){
            playerlist[fromPlayer].name = it->first;
            playerlist[fromPlayer].status = statusID::online;

            cout << "Successful login from player " << fromPlayer << "\n";

            packet << fromPlayer << static_cast<int> (packetID::LoginResponse) << true;
            players[fromPlayer]->send(packet);
            return;
        }
    }
    //only goes through here in case of login failure
    packet << fromPlayer << static_cast<int> (packetID::LoginResponse) << false;
    players[fromPlayer]->send(packet);
}

void sendRegisterResponse (int fromPlayer, string ulogin, string upassword) {
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

void sendMove(int fromPlayer, int toPlayer, int i, int j, int iP, int jP){
    Packet packet;
    packet << fromPlayer << static_cast<int> (packetID::Move);
    packet << i << j << iP << jP;
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
            sendLoginResponse (fromPlayer, ulogin, upassword);
            break;
        }
        case packetID::Register : {
            string ulogin, upassword;
            packet >> ulogin >> upassword;
            sendRegisterResponse (fromPlayer, ulogin, upassword);
            break;
        }
        case packetID::Move:
            int i, j, iP, jP;
            packet >> i >> j >> iP >> jP;
            cout << "Movement from player " << fromPlayer << endl;
            toPlayer = playersEnemy[fromPlayer];
            sendMove(fromPlayer, toPlayer, i, j, iP, jP);
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
        default : { //redirect
            if (toPlayer < 0 || toPlayer > players.size() -1) return;
            players[toPlayer]->send(packet);

            return; //do not update player list
        }

    }
    //Since a new player might have entered, or 2 of them are now playing,
    //it's necessary to send a packet updating the Playerlist;
}

int main(){
    int toPlayer, fromPlayer, pktID;
    Packet packet;
    listener.setBlocking(false);
    string message;

    loadAccounts();
    cout << "Server started!" << endl;
    ifstream config {"data/config.dat"};
    int port;
    if (config.is_open())
        config >> port;
    //int port = 14193;
    while (listener.listen (port) != Socket::Done);
    cout << "Successfully binded to port " << port << endl;
    players.emplace_back (new TcpSocket());
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
    }
    system("PAUSE");

}
