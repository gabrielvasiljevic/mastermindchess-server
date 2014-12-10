#include "gamemaster.h"
#include <iostream>
#include <math.h>

using namespace std;

void GameMaster::connectToMaster(){
    if (listener.accept(master) == Socket::Done) {
        master.setBlocking (false);
        cout << "Game Master connected! Behold!\n";
    }
}

void GameMaster::sendLoginResponse (string ulogin, string upassword) {
    Packet packet;
    int i;
    bool foundUser = false;

    if(ulogin == "MASTER"){
        foundUser = true;
    }
    if(!foundUser){
//        network.logHistory ("Invalid Access from GM", "");
        packet << static_cast<int> (masterPacketID::LoginResponse) << false;
        master.send(packet);
        return;
    }
    if(foundUser && upassword == "MASTER"){

     //   network.logHistory ("Access Granted to GM.", "");
        packet << static_cast<int> (masterPacketID::LoginResponse) << true;
        master.send(packet);
        return;
    }
   // logHistory ("Invalid Access from GM", "");
    packet << static_cast<int> (masterPacketID::LoginResponse) << false;
    master.send(packet);
}

void GameMaster::sendMatchList () {
    Packet packet;
    int i;
    packet << static_cast<int> (masterPacketID::MatchList) << static_cast<int>(network.matches.size());
    for(i = 0; i < network.matches.size(); i++){
        packet << network.matches[i].ID
               << network.matches[i].white.nickname
               << network.matches[i].black.nickname
               << network.matches[i].gameMode
               << network.matches[i].gameTime
               << network.matches[i].status;
    }
   // logHistory ("Invalid Access from GM", "");

    master.send(packet);
}

void GameMaster::handleMasterRequest(Packet packet, int pktID) {

    switch (static_cast<masterPacketID> (pktID)) {
        case masterPacketID::Login : {
            string ulogin, upassword;
            packet >> ulogin >> upassword;
            if(ulogin.size() > 0 && upassword.size() > 0)
                sendLoginResponse(ulogin, upassword);
            break;
        }
        case masterPacketID::MatchList : {
            cout << "Sending match list to Master." << endl;
            sendMatchList();
            break;
        }

    }

}
