#ifndef _PLAYER
#define _PLAYER
#include <SFML/Network.hpp>
#include <SFML/System.hpp>
#include "network.h"


class Player{

    public:

    unique_ptr<sf::TcpSocket> connection;

    string username;
    string password;
    string nickname;
    string email;
    string registerdate;

    sf::Clock time;
    statusID status;


    int ID;
    int elo;
    int K;
    int normal_5_ELO , normal_15_ELO , normal_30_ELO;
    int capa_5_ELO   , capa_15_ELO   , capa_30_ELO;
    int fischer_5_ELO, fischer_15_ELO, fischer_30_ELO;
    int number_of_matches;
    int opponent;
    int matchID;
    bool active;


    Player(string username, string password, string nickname, string email);
    Player();

};

#endif // _PLAYER
