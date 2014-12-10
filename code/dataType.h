#ifndef _DATA
#define _DATA

#include <SFML/Network.hpp>
#include <SFML/System.hpp>
#include <sstream>




using namespace std;

const std::string GAME_VERSION = "0.1.9";

                    //0     1     2       3       4         5            6          7          8            9           10     11
enum class packetID {None, Name, Move, Connect, Login, LoginResponse, GameEnd, Disconnect, Register, RegisterResponse, Chat, GameRequest,
                    Response, Options, Elo, MatchLog, Check, Checkmate, KeepAlive, MoveLog, WrongVersion, PlayerList, GameEndTimeOut, GiveUp,
                    MatchList, WatchRequest, WatchState, ExitQueue, BoardStatus, GameInvite, InviteResponse, FischerPieceOrder, SpectatorChat,
                    MatchHistory, numberWatching, InviteRejection, AlreadyConnected};

enum class masterPacketID {None, Login, LoginResponse, Disconnect, MatchList};

enum class statusID {offline = 0, online, playing, watching};

enum class gameType {Classic = 0, Fischer, Capa};


class userInfo {
    public:
    int id;
    string username;
    string nickname;
    string password;
    string email;
    string registerdata;
    string active;

    int K;
    int number_of_matches;
    int elo[3][3];
    int victories[3][3];
    int defeats[3][3];
    int draws[3][3];
    int matchID;

    statusID status;
    userInfo(int _id, string _username, string _nickname, string _password, string _email, string _registerdata){
        id           = _id;
        username     = _username;
        nickname     = _nickname;
        password     = _password;
        email        = _email;
        registerdata = _registerdata;
        active       = "1";
        status       = statusID::offline;

        for(int i = 0; i < 3; i++){
            for(int j = 0; j < 3; j++){
                elo[i][j] = 950;
                victories[i][j] = 0;
                defeats[i][j] = 0;
                draws[i][j] = 0;
            }
        }

        number_of_matches = 0;
        K = 30;
    }
    userInfo(){status = statusID::offline;}
};


#endif // _DATA
