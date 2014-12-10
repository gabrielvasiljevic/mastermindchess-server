#ifndef _NET
#define _NET

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

#include "dataType.h"
#include "match.h"

#include "utility.h"

using namespace std;
using namespace sf;

class Network{
    public:

        vector<unique_ptr<TcpSocket>>     players;
        vector<userInfo>                  userlist;
        vector<Match>                     matches;
        vector<sf::Clock>                 playersTime;

        map<int, int>                     Opponents;
        map<int, string>                  accountsIDs;
        map<string, int>                  IDsAccounts;


        queue<int>                        classic5Queue;
        queue<int>                        capa5Queue;
        queue<int>                        random5Queue;

        queue<int>                        classic15Queue;
        queue<int>                        capa15Queue;
        queue<int>                        random15Queue;

        queue<int>                        classic30Queue;
        queue<int>                        capa30Queue;
        queue<int>                        random30Queue;

        TcpListener                       listener;
        Packet                            packet;

        bool sendList;
        bool sendMatches;

        int timeout;
        int numberOfUsers;
        int numberOfMatches;
        int onlineUsers;
        int playingUsers;
        int i;

        void acceptNewPlayers       ();
        void saveAccounts           ();
        void logHistory             (string action, string playerName);
        void logHistory             (string action, string playerName, int playerID);
        void registerAccount        (string username, string password, string nickname, string email);
        void loadUserInformation    ();
        void sendPlayerList         ();
        void sendMatchesList        ();
        void updateMatches          ();
        void updateServerInfo       ();
        void disconnectPlayer       (int playerID);
        void removeFromQueue        (int playerID);
        void updateBoard            (int playerID, string newBoard[8][10]);
        void sendFischerPiecesOrder (int playerID, string newBoard[][10]);
        void sendNumberWatching     (int playerID, int number);
        void inviteRequest          (int playerID, string name, int gameMode, int gameTime, bool isPublic);
        void gameRequest            (int playerID, int gameMode, int gameTime);
        void classicRequest         (int playerID, int gameTime);
        void capaRequest            (int playerID, int gameTime);
        void randomRequest          (int playerID, int gameTime);
        void capaGameRequest        (int playerID);
        void randomGameRequest      (int playerID);
        void watchRequest           (int playerID, int watchID);
        void processRequest         (int playerID, int enemyID, gameType gameMode, int gameTime, bool isPublic, bool ranked);
        void saveMatchHistory       (int playerID, string history);
        void sendLoginResponse      (int playerID, string ulogin, string upassword, string version);
        void sendRegisterResponse   (int fromPlayer, string uusername, string upassword, string unickname, string uemail);
        void sendOptions            (int playerID, int toPlayer, int gameColor, int gameMode, int gameTime, bool ranked);
        void sendMove               (int playerID, int toPlayer, int i, int j, int iP, int jP, bool check);
        void sendMoveLog            (int playerID, int toPlayer, string moveLog);
        void sendGameEnd            (int playerID, int toPlayer);
        void sendGiveUp             (int playerID, int toPlayer);
        void sendCheckMate          (int playerID, int toPlayer);
        void sendChatMessage        (int playerID, int toPlayer, string msg);
        void sendSpectatorChatMessage(int playerID, string msg);
        void sendGameRequest        (int playerID, int toPlayer);
        void sendCapaGameRequest    (int playerID, int toPlayer);
        void sendWatchState         (int playerID, int matchID);
        void sendInviteResponse     (int playerID, string invitee, int gameMode, int gameTime, bool isPublic, bool response);
        void sendInviteRejection    (int playerID, int toPlayer, string user);
        void checkTimeout           ();
        Network                     ();
};
#endif // _NET
