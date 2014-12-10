#ifndef _MATCH
#define _MATCH
#include "player.h"
#include "utility.h"

class Match{
    public:
    userInfo white, black;
    userInfo spectators[100];

    sf::Clock whiteClock, blackClock;
    double whiteTime, blackTime;
    string startTime, endTime;
    int gameTime, gameMode;
    int winner, loser;
    int eloWon, eloLost;
    int ID;
    int status;
    int numberWatching;
    bool isPublic;
    bool ranked;
    string board[8][10];
    string match_history;

    Match(userInfo white, userInfo black, int gameMode, int gameTime){
        this->white = white;
        this->black = black;
        this->gameMode = gameMode;
        this->gameTime = gameTime;
        this->startTime = getDatestamp(false, true);
        numberWatching = 0;
        status = 1;
        isPublic = true;
        for(int i = 0; i < 8; i++){
            for(int j = 0; j < 10; j++){
                board[i][j] = " ";
            }
        }
    }
};

#endif // _MATCH
