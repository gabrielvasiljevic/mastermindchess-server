#ifndef _MASTER
#define _MASTER
#include "network.h"


class GameMaster{
    public:
    TcpSocket               master;
    Network&                network;
    TcpListener             listener;
    Packet                  packet;

    string                  input;


    void connectToMaster    ();
    void sendLoginResponse  (string ulogin, string upassword);
    void sendMatchList      ();
    void handleMasterRequest(Packet packet, int pktID);
    string getInput         ();


    GameMaster(Network& network): network(network){
        this->input = "";
    }
};

#endif // _MASTER

