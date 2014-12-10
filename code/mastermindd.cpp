#include <SFML/Network.hpp>
#include <SFML/System.hpp>
#include <memory>
#include <omp.h>
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
#include <stdexcept>
#ifdef __cplusplus__
  #include <cstdlib>
#else
  #include <stdlib.h>
#endif



#include "dataType.h"
#include "network.h"
#include "utility.h"
#include "gamemaster.h"

#define SEND_LIST_DELAY_TIME 5


using namespace std;
using namespace sf;

std::map<std::string, std::string> options;


string logDirectory;
string dataDirectory;

Network     network;


sf::Clock   sendListTime;
sf::Clock   updateTime;


bool waiting = false;
int playerID;
int port;

sf::Packet& operator >>(sf::Packet& packet, char c){
    return packet >> c;
}

void logHistory (string action, string playerName);


void parse(std::ifstream & cfgfile){
    std::string id, eq, val;

    while(cfgfile >> id >> val){
      if (id[0] == '#') continue;  // skip comments
      //if (eq != "=") throw std::runtime_error("Parse error");

      options[id] = val;
    }
}

void loadConfiguration(){
   ifstream config {"mastermindd.conf"};
    parse(config);
    port = StringToNumber(options["Port"]);
    logDirectory = options["LogDirectory"];
    dataDirectory = options["DataDirectory"];
    //if (config.is_open())
       // config >> port;
}


void handleClientRequest(Packet packet, int pktID, int playerID, int toPlayer) {
    int i;

    string movelog;
    string msg;
    switch (static_cast<packetID> (pktID)) {
        case packetID::Login : {
            string ulogin, upassword, version;
            packet >> ulogin >> upassword >> version;
            if(ulogin.size() > 0 && upassword.size() > 0)
                network.sendLoginResponse(playerID, ulogin, upassword, version);
            break;
        }
        case packetID::Register : {
            string uusername, upassword, unickname, uemail;
            packet >> uusername >> upassword >> unickname >> uemail;
            network.sendRegisterResponse(playerID, uusername, upassword, unickname, uemail);
            break;
        }
        case packetID::Move: {
            int i, j, iP, jP, k, p;
            bool check;
            string newBoard[8][10];
            packet >> i >> j >> iP >> jP >> check;

            for(k = 0; k < 8; k++){
                for(p = 0; p < 10; p++){
                    packet >> newBoard[k][p];
                }
            }

            network.updateBoard(playerID, newBoard);
            network.sendMove(playerID, toPlayer, i, j, iP, jP, check);
        }
        break;

        case packetID::BoardStatus:{
            int k, p;
            string newBoard[8][10];
            for(k = 0; k < 8; k++){
                for(p = 0; p < 10; p++){
                    packet >> newBoard[k][p];
                }
            }
            network.updateBoard(playerID, newBoard);
        }
        break;

        case packetID::FischerPieceOrder:{
            int k, p;
            string newBoard[8][10];
            for(k = 0; k < 8; k++){
                for(p = 0; p < 10; p++){
                    packet >> newBoard[k][p];
                }
            }
            network.sendFischerPiecesOrder(playerID, newBoard);
        }

        case packetID::Chat:
            packet >> msg;
            network.sendChatMessage(playerID, toPlayer, msg);
        break;

        case packetID::SpectatorChat:
            packet >> msg;
            network.sendSpectatorChatMessage(playerID, msg);
        break;

        case packetID::MatchHistory:
            packet >> msg;
            network.saveMatchHistory(playerID, msg);
        break;

        case packetID::Checkmate: {
            network.sendCheckMate(playerID, toPlayer);
            break;
        }

        case packetID::GiveUp: {
            network.sendGiveUp(playerID, toPlayer);
            break;
        }

        case packetID::GameEnd : {
            network.sendGameEnd(playerID, toPlayer);
            break;
        }
        case packetID::GameEndTimeOut : {
            //cout << "GameEndTimeout from player " << playerID << endl;
            network.sendGameEnd(playerID, toPlayer);
        break;
        }

        case packetID::PlayerList:
            network.sendPlayerList();
        break;

        case packetID::Disconnect : {
            network.disconnectPlayer(playerID);
            break;
        }
        case packetID::GameRequest:{
            int gameTime, gameMode;
            packet >> gameMode >> gameTime;
            network.gameRequest(playerID, gameMode, gameTime);
        break;
        }
        case packetID::GameInvite: {
            string name;
            int gameMode, gameTime;
            bool isPublic;
            packet >> name >> gameMode >> gameTime >> isPublic;
            network.inviteRequest(playerID, name, gameMode, gameTime, isPublic);
        break;
        }

        case packetID::InviteResponse: {
            string name1;
            bool response;
            int gameMode, gameTime;
            bool isPublic;
            packet >> name1 >> response;
            if(response)
                packet >> gameMode >> gameTime >> isPublic;
            network.sendInviteResponse(playerID, name1, gameMode, gameTime, isPublic, response);
            //cout << "Invite Response from " << playerID << " to " << name1 << endl;
        break;
        }

        case packetID::WatchRequest:
            int watchID;
            packet >> watchID;
            network.watchRequest(playerID, watchID);
        break;

        case packetID::ExitQueue:
            network.removeFromQueue(playerID);
        break;

        case packetID::MatchLog :
        //    string log;
        //    string player1, player2;
        //    packet >> log;
        //    playersEnemy[fromPlayer];
        break;
        case packetID::MoveLog:
            packet >> movelog;
            network.sendMoveLog(playerID, toPlayer, movelog);
        break;
        case packetID::KeepAlive: {
            network.playersTime[playerID].restart();
        break;
        }
        default : { //redirect
            if (toPlayer < 0 || toPlayer > network.players.size() -1) return;
            network.players[toPlayer]->send(packet);

            return; //do not update player list
        }

    }
    //Since a new player might have entered, or 2 of them are now playing,
    //it's necessary to send a packet updating the Playerlist;
    if (playerID >= 0)
        network.playersTime[playerID].restart();

}



int main(){
    int toPlayer, fromPlayer, pktID;
    loadConfiguration();
    Packet packet;
    //GameMaster  gameMaster(network);
    network.listener.setBlocking(false);
    //gameMaster.listener.setBlocking(false);
    string message;
    string input = "";

    int masterPort = 14093;

    while (network.listener.listen (port) != Socket::Done);
    //while (gameMaster.listener.listen (masterPort) != Socket::Done);


    network.players.emplace_back (new TcpSocket());
    network.loadUserInformation();

    cout << "---------------- MasterMind Chess Server v0.1.7 ---------------- \n\n";
    /*cout << "Open connections: "    << network.players.size()   << endl;
    cout << "Number of users: "     << network.numberOfUsers    << endl;
    cout << "Online users: "        << network.onlineUsers      << endl;
    cout << "Players in a match: "  << network.playingUsers     << endl;
    cout << "\n\nServer events: "                               << endl;*/
    cout << "\nSuccessfully binded to port " << port            << endl;
    cout << "Server started!"                                   << endl;

    network.logHistory ("System initialized at port " + to_string(port), IpAddress::getPublicAddress().toString());
    while (true) {
        if(updateTime.getElapsedTime().asSeconds() >= 16){
            network.updateServerInfo();
            updateTime.restart();
            network.sendMatchesList();
            /*if (system("CLS")) system("clear");
            cout << "---------------- MasterMind Chess Server v0.1.7 ---------------- \n\n";
            cout << "Open connections: "    << network.players.size()   << endl;
            cout << "Number of users: "     << network.numberOfUsers    << endl;
            cout << "Online users: "        << network.onlineUsers      << endl;
            cout << "Players in a match: "  << network.playingUsers     << endl;
            cout << "\n\nServer events: \n"                             << endl;*/
        }
        network.acceptNewPlayers();
        //gameMaster.connectToMaster();
        playerID = 0;
        /*if(gameMaster.master.receive(packet) == Socket::Done){
            packet >> pktID;
            gameMaster.handleMasterRequest(packet, pktID);
        }*/

        for (auto& player : network.players){
            if (player->receive(packet) == Socket::Done) {
                packet >> toPlayer >> pktID;
                handleClientRequest(packet, pktID, playerID, toPlayer);
            }
            playerID++;
        }
        if(network.sendMatches){
            network.sendMatchesList();
            network.sendMatches = false;
        }
        if(sendListTime.getElapsedTime().asSeconds() >= SEND_LIST_DELAY_TIME){
            network.sendPlayerList();
            sendListTime.restart();
        }
        sf::sleep(sf::milliseconds(500));
        network.checkTimeout();
    }
    system("PAUSE");

}
