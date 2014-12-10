#include "network.h"
#include <iostream>
#include <math.h>

using namespace std;

//    auto it = accountsIDs.find(toPlayer);
//    auto myID = accountsIDs.find(playerID);


extern string logDirectory;
extern string dataDirectory;

Network::Network(){
    timeout = 30;
    numberOfUsers = 0;
    numberOfMatches = 0;
    onlineUsers = 0;
    playingUsers = 0;
    sendList = false;
    sendMatches = false;


}


/* ~~~~~~~~~~~~~~~~~~~~~~ Utility functions ~~~~~~~~~~~~~~~~~~~~~~ */

int calculateELO(int Ra, int Rb, double Sa){ //Elo from player 1 and player 2, and result of the match
    int K = 15;
    double Ea = 1/(1 + pow(10,  ( (Rb - Ra)/400 ) ));
    return (Ra + (int)(K*(Sa - Ea)));
}


void Network::logHistory (string action, string playerName) {
    ofstream output {logDirectory + "mastermind_" + getDatestamp('-') + ".log", ios::app};
    output <<  getTimestamp() << " " << playerName << " " << action  << "\n";
    output << std::flush;
}

void Network::logHistory (string action, string playerName, int playerID) {
    ofstream output {logDirectory + "mastermind_" + getDatestamp('-') + ".log", ios::app};
    output <<  getTimestamp()  << " " << playerName << " " << action << " " << players[playerID]->TcpSocket::getRemoteAddress().toString() << "\n";
    output << std::flush;

}


void logMatch(Match match) {
    string PGN;
    PGN = "[Event MasterMind Chess Ranked Match]\n[Date " + getDatestamp(true,true) +
            "]\n[White " + match.white.nickname + "]\n[Black " + match.black.nickname +
            "]\n[Winner " + to_string(match.winner) + "]\n\n";
    ofstream output {dataDirectory + "matches", ios::app};
    ofstream outmatch {dataDirectory + "matches_log/match_" + to_string(match.ID+1) + ".pgn", ios::app};
    output  << match.ID                 << ";"
            << match.white.id           << ";"
            << match.black.id           << ";"
            << match.winner             << ";"
            << match.eloWon             << ";"
            << match.loser              << ";"
            << match.eloLost            << ";"
            //<< match.startTime          << ";"
            << getDatestamp(true,true)  << ";"
            << match.gameMode           << ";"
            << match.gameTime           << ";\n";
    output << std::flush;
    outmatch << PGN << match.match_history;
    outmatch << std::flush;

}


/* ~~~~~~~~~~~~~~~~~~~~~~ Configuration functions ~~~~~~~~~~~~~~~~~~~~~~ */
void Network::acceptNewPlayers() {
    if (listener.accept(*players.back()) == Socket::Done) {
        players.back()->setBlocking (false);
        playersTime.emplace_back();
        players.emplace_back (new TcpSocket());
    }
}

void Network::saveAccounts() {
    //cout << "Saving users..." << endl;
    ofstream outputData {dataDirectory + "users"};
    ofstream outputElo {dataDirectory + "elos"};

    for (auto& user : userlist){
        outputData  << user.id           << ";"
                    << user.username     << ";"
                    << user.nickname     << ";"
                    << user.password     << ";"
                    << user.email        << ";"
                    << user.registerdata << ";"
                    << user.active       << ";\n";
    }
    outputData << std::flush;
    for (auto& user : userlist){
        outputElo << user.id << ";";
        for(int i = 0; i < 3; i++)
            for(int j = 0; j < 3; j++){
                outputElo   << user.elo[i][j]       << ";"
                            << user.victories[i][j] << ";"
                            << user.defeats[i][j]   << ";"
                            << user.draws[i][j]     << ";";
            }
        outputElo << "\n";
    }
    outputElo << std::flush;
    //cout << "Users data saved!" << endl;
}


void Network::registerAccount(string username, string password, string nickname, string email) {
    userlist.emplace_back();
    userlist[numberOfUsers].id          = numberOfUsers;
    userlist[numberOfUsers].username    = username;
    userlist[numberOfUsers].nickname    = nickname;
    userlist[numberOfUsers].password    = password;
    userlist[numberOfUsers].email       = email;
    userlist[numberOfUsers].active      = "1";
    for(int i = 0; i < 3; i++){
        for(int j = 0; j < 3; j++){
            userlist[numberOfUsers].elo[i][j]  = 950;
            userlist[numberOfUsers].victories[i][j]  = 0;
            userlist[numberOfUsers].defeats[i][j]  = 0;
            userlist[numberOfUsers].draws[i][j]  = 0;
        }
    }
    userlist[numberOfUsers].registerdata = getDatestamp('/') + " " + getTimestamp();
    numberOfUsers++;
    saveAccounts();
    sendPlayerList();
}

void Network::loadUserInformation(){
    ifstream matchInput {dataDirectory + "matches"};
    ifstream input      {dataDirectory + "users"};
    ifstream eloInput   {dataDirectory + "elos"};
    string aux;

    if(!isEmpty(matchInput)){
        while(getline(matchInput, aux)){
            numberOfMatches++;
        }
    }

    int i;
    string id, extraId, tempElo, tempVic, tempDef, tempDraw;
    if(!isEmpty(input)){
        userlist.emplace_back();
        while(getline(input, id, ';')){
            if(id != "\n"){
                  getline(input, userlist[numberOfUsers].username,     ';');
                  getline(input, userlist[numberOfUsers].nickname,     ';');
                  getline(input, userlist[numberOfUsers].password,     ';');
                  getline(input, userlist[numberOfUsers].email,        ';');
                  getline(input, userlist[numberOfUsers].registerdata, ';');
                  getline(input, userlist[numberOfUsers].active,       ';');
                  getline(input, aux);
                  userlist[numberOfUsers].id           = StringToNumber(id);
                  getline(eloInput, extraId, ';');
                  for(int i = 0; i < 3; i++){
                    for(int j = 0; j < 3; j++){
                        getline(eloInput, tempElo,                        ';');
                        userlist[numberOfUsers].elo[i][j] = StringToNumber(tempElo);
                        getline(eloInput, tempVic,                        ';');
                        userlist[numberOfUsers].victories[i][j] = StringToNumber(tempVic);
                        getline(eloInput, tempDef,                        ';');
                        userlist[numberOfUsers].defeats[i][j] = StringToNumber(tempDef);
                        getline(eloInput, tempDraw,                       ';');
                        userlist[numberOfUsers].draws[i][j] = StringToNumber(tempDraw);
                    }
                  }
                  numberOfUsers++;
                  if(input.peek() != EOF && input.peek() != 0) userlist.emplace_back();
            }
        }
    }

}

void Network::sendLoginResponse (int playerID, string ulogin, string upassword, string version) {
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
            logHistory ("not found", ulogin, playerID);
            packet << playerID << static_cast<int> (packetID::LoginResponse) << false;
            players[playerID]->send(packet);
            return;
        }
        if(foundUser && userlist[i].password == upassword){
            if(userlist[i].status == statusID::offline){
                accountsIDs[playerID] = userlist[i].username;
                IDsAccounts[userlist[i].username] = playerID;
                userlist[i].status = statusID::online;
                //cout << "Successful login from player " << userlist[i].username << "\n";

                logHistory ("logon", userlist[i].username, playerID);
                packet << playerID << static_cast<int> (packetID::LoginResponse) << true << userlist[i].nickname;
                for(int j = 0; j < 3; j++){
                    for(int k = 0; k < 3; k++)
                    packet << userlist[i].elo[j][k];
                }
                players[playerID]->send(packet);
                sendList = true;
                sendMatches = true;
                return;
            }
            else{
                //cout << userlist[i].username << " already connected" << endl;
                logHistory ("already connected", userlist[i].username, playerID);
                packet << playerID << static_cast<int> (packetID::AlreadyConnected);
                players[playerID]->send(packet);
                return;
            }

        }
        logHistory ("incorrect password", userlist[i].username, playerID);
        packet << playerID << static_cast<int> (packetID::LoginResponse) << false;
        players[playerID]->send(packet);
    }
    else{
        packet << playerID << static_cast<int> (packetID::WrongVersion);
        players[playerID]->send(packet);
    }
}

void Network::sendRegisterResponse (int fromPlayer, string uusername, string upassword, string unickname, string uemail) { //change to the new style
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
    //cout << "Successful register from player " << unickname << "\n";
    logHistory ("new account", unickname, fromPlayer);
    packet << fromPlayer << static_cast<int> (packetID::RegisterResponse) << true;
    players[fromPlayer]->send(packet);
    return;
}

void Network::sendPlayerList(){
    Packet packet;
    packet << -1 << static_cast<int> (packetID::PlayerList);

    packet << static_cast<int> (userlist.size());
    for (auto& user : userlist) {
        packet << user.nickname;
        for(int i = 0; i < 3; i++)
            for(int j = 0; j < 3; j++){
                packet << user.elo[i][j]
                       << user.victories[i][j]
                       << user.defeats[i][j]
                       << user.draws[i][j];
        }
        packet << static_cast<int> (user.status);
    }
    for(auto& player : players)
        player->send(packet);

    //cout << getTimestamp() << " Sended player list!\n";
}

void Network::sendMatchesList(){
    Packet packet;
    int i;
    packet << -1 << static_cast<int> (packetID::MatchList);

    packet << (int) matches.size();
    for (i = 0; i < matches.size(); i++) {
        packet << static_cast<int> (matches[i].ID)
               << matches[i].white.nickname
               << matches[i].black.nickname
               << static_cast<int> (matches[i].gameMode)
               << static_cast<int> (matches[i].gameTime)
               << static_cast<int> (matches[i].isPublic)
               << static_cast<int> (matches[i].ranked)
               << static_cast<int> (matches[i].status);
    }
    for(auto& player : players)
        player->send(packet);

    //cout << getTimestamp() << " Sended match list!\n";
}

void Network::updateServerInfo(){
    int i, on = 0, pl = 0;
    for(i = 0; i < userlist.size(); i++){
        if(userlist[i].status == statusID::online || userlist[i].status == statusID::playing){
            on++;
            if(userlist[i].status == statusID::playing)
                pl++;
        }
    }
    onlineUsers = on;
    playingUsers = pl;
}

/* ~~~~~~~~~~~~~~~~~~~~~~ Connect functions ~~~~~~~~~~~~~~~~~~~~~~ */

void Network::disconnectPlayer(int playerID){
    auto findDisconnect = accountsIDs.find(Opponents[playerID]);
    auto myID           = accountsIDs.find(playerID);
    int i;
    if(myID->second.size() > 0){
        for(i = 0; i < userlist.size(); i++){
            if(userlist[i].username == myID->second){
                //cout << "Player " << userlist[i].username << " disconnected\n";
                if (userlist[i].status == statusID::playing) {
                    sendGameEnd(playerID, Opponents[playerID]);
                }

                accountsIDs[playerID] = "";
                IDsAccounts[userlist[i].username] = -1;
                userlist[i].status = statusID::offline;
                players[playerID]->disconnect();
                logHistory ("logout", userlist[i].username);
            }
            if(userlist[i].username == findDisconnect->second){
                userlist[i].status = statusID::online;
            }
        }
    }

    sendList = true;
}

void Network::inviteRequest(int playerID, string name, int gameMode, int gameTime, bool isPublic){
    Packet packet;
    int i;
    string myName;
    auto myID = accountsIDs.find(playerID);

    for(i = 0; i < userlist.size(); i++){
        if(userlist[i].username == myID->second){
            myName = userlist[i].nickname; //Use the nickname to send the invite.
        }
    }

    for(int i = 0; i < userlist.size(); i++){
        if(userlist[i].nickname == name){ //locate the invited user by its nickname
            auto inviteID = IDsAccounts.find(userlist[i].username); //find his ID by his username
            if(userlist[i].status == statusID::online){ //only if he is online,
                packet << playerID << static_cast<int> (packetID::GameInvite) << myName << gameMode << gameTime << isPublic; // send him the invitation.
                players[inviteID->second]->send(packet);
            }
            else{
                //send negative response back to the inviting player
            }
        }
    }

}

void Network::gameRequest(int playerID, int gameMode, int gameTime){
    switch(gameMode){
        case 0:
            classicRequest(playerID, gameTime);
        break;
        case 1:
            randomRequest(playerID, gameTime);
        break;
        case 2:
            capaRequest(playerID, gameTime);
        break;
        default:
            cout << "Something went very wrong..." << endl;
        break;
    }
}

void Network::classicRequest(int playerID, int gameTime){
    switch(gameTime){
        case 300:
            if(classic5Queue.size() > 0){
                int enemy = classic5Queue.front();
                classic5Queue.pop();
                processRequest(playerID, enemy, gameType::Classic, gameTime, 1, 1);
            }
            else classic5Queue.push(playerID);
        break;
        case 900:
            if(classic15Queue.size() > 0){
                int enemy = classic15Queue.front();
                classic15Queue.pop();
                processRequest(playerID, enemy, gameType::Classic, gameTime, 1, 1);
            }
            else classic15Queue.push(playerID);
        break;
        case 1800:
            if(classic30Queue.size() > 0){
                int enemy = classic30Queue.front();
                classic30Queue.pop();
                processRequest(playerID, enemy, gameType::Classic, gameTime, 1, 1);
            }
            else classic30Queue.push(playerID);
        break;
    }
}

void Network::capaRequest(int playerID, int gameTime){
    switch(gameTime){
        case 300:
            if(capa5Queue.size() > 0){
                int enemy = capa5Queue.front();
                capa5Queue.pop();
                processRequest(playerID, enemy, gameType::Capa, gameTime, 1, 1);
            }
            else capa5Queue.push(playerID);
        break;
        case 900:
            if(capa15Queue.size() > 0){
                int enemy = capa15Queue.front();
                capa15Queue.pop();
                processRequest(playerID, enemy, gameType::Capa, gameTime, 1, 1);
            }
            else capa15Queue.push(playerID);
        break;
        case 1800:
            if(capa30Queue.size() > 0){
                int enemy = capa30Queue.front();
                capa30Queue.pop();
                processRequest(playerID, enemy, gameType::Capa, gameTime, 1, 1);
            }
            else capa30Queue.push(playerID);
        break;
    }
}

void Network::randomRequest(int playerID, int gameTime){
    switch(gameTime){
        case 300:
            if(random5Queue.size() > 0){
                int enemy = random5Queue.front();
                random5Queue.pop();
                processRequest(playerID, enemy, gameType::Fischer, gameTime, 1, 1);
            }
            else random5Queue.push(playerID);
        break;
        case 900:
            if(random15Queue.size() > 0){
                int enemy = random15Queue.front();
                random15Queue.pop();
                processRequest(playerID, enemy, gameType::Fischer, gameTime, 1, 1);
            }
            else random15Queue.push(playerID);
        break;
        case 1800:
            if(random30Queue.size() > 0){
                int enemy = random30Queue.front();
                random30Queue.pop();
                processRequest(playerID, enemy, gameType::Fischer, gameTime, 1, 1);
            }
            else random30Queue.push(playerID);
        break;
    }
}


void Network::watchRequest(int playerID, int watchID){
    int i, j;
    auto findPlayer = accountsIDs.find(playerID);
    for(j = 0; j < matches.size(); j++){
        if(matches[j].ID == watchID){
            for(i = 0; i < userlist.size(); i++){
                if(userlist[i].username == findPlayer->second && userlist[i].status == statusID::online){
                    if(matches[j].isPublic){
                        matches[j].spectators[matches[j].numberWatching++] = userlist[i];
                        sendWatchState(playerID, watchID);
                        sendNumberWatching(playerID, matches[j].numberWatching);
                        sendNumberWatching(matches[j].white.id, matches[j].numberWatching);
                        sendNumberWatching(matches[j].black.id, matches[j].numberWatching);
                    }
                }
            }
        }
    }

}

void Network::sendNumberWatching(int playerID, int number){
    packet.clear();
    packet << playerID << static_cast<int> (packetID::numberWatching);
    packet << number;
    players[playerID]->send(packet);
}

void Network::processRequest(int playerID, int enemy, gameType gameMode, int gameTime, bool isPublic, bool ranked){
    int i, j, foundI, foundJ;
    //cout << "Estabilishing connection to player " << enemy << "...\n";
    auto findEnemy = accountsIDs.find(enemy);
    auto findNew = accountsIDs.find(playerID);
    //cout << "enemy name: " << findEnemy->second << endl;
    for(i = 0; i < userlist.size(); i++){
        if(userlist[i].username == findEnemy->second && userlist[i].status == statusID::online){
            packet.clear();
            foundI = i;

            sendOptions(enemy, playerID, 0, static_cast<int>(gameMode), gameTime, ranked);
            sendOptions(playerID, enemy, 1, static_cast<int>(gameMode), gameTime, ranked);

            packet.clear();

            for(j = 0; j < userlist.size(); j++){
                if(findNew->second == userlist[j].username && userlist[j].status == statusID::online){
                    foundJ = j;
                    //cout << "Connect from player " << userlist[j].username << "(" << playerID << ")"
                    //<< " to player " << findEnemy->second << "(" << enemy << ")" << "\n";
                    userlist[j].status = userlist[i].status = statusID::playing;
                    Match newMatch(userlist[i], userlist[j], static_cast<int>(gameMode), gameTime);
                    newMatch.ID = numberOfMatches++;
                    newMatch.isPublic = isPublic;
                    newMatch.ranked = ranked;
                    userlist[i].matchID = userlist[j].matchID = newMatch.ID;
                    matches.push_back(newMatch);
                }
            }
            packet.clear();
            packet << playerID << static_cast<int> (packetID::Connect);
            players[enemy]->send(packet);

            packet << enemy << static_cast<int> (packetID::Connect);
            players[playerID]->send(packet);

            packet.clear();
            packet << enemy << static_cast<int> (packetID::Name);
            packet << userlist[foundI].nickname;
            players[playerID]->send(packet);

            packet.clear();
            packet << playerID << static_cast<int> (packetID::Name);
            packet << userlist[foundJ].nickname;
            players[enemy]->send(packet);
            if(ranked){
                packet.clear();
                packet << enemy << static_cast<int> (packetID::Elo);
                packet << static_cast<int>(userlist[foundI].elo[static_cast<int>(gameMode)][gameTime/900]);
                players[playerID]->send(packet);

                packet.clear();
                packet << playerID << static_cast<int> (packetID::Elo);
                packet << static_cast<int>(userlist[foundJ].elo[static_cast<int>(gameMode)][gameTime/900]);
                players[enemy]->send(packet);
            }


            Opponents[playerID] = enemy;
            Opponents[enemy] = playerID;
        }
    }
    sendMatches = true;
}


void Network::saveMatchHistory(int playerID, string history){
    auto it = accountsIDs.find(playerID);
    int i, k, p;
    for(auto& player : userlist){
        if(it->second == player.username){
            for(i = 0; i < matches.size(); i++){
                if(it->second == matches[i].white.username || it->second == matches[i].black.username){
                    matches[i].match_history = history;
                }
            }
        }
    }
}

void Network::sendOptions(int playerID, int toPlayer, int gameColor, int gameMode, int gameTime, bool ranked){
    Packet packet;
    packet << playerID << static_cast<int> (packetID::Options);
    packet << gameColor << gameMode << gameTime << ranked;
    players[toPlayer]->send(packet);
    //cout << "Sended options to " << toPlayer << endl;
}


void Network::sendMove(int playerID, int toPlayer, int i, int j, int iP, int jP, bool check){
    Packet packet;
    auto myID = accountsIDs.find(playerID);
    int k, p;
    packet << playerID << static_cast<int> (packetID::Move);
    //cout << "Movement from player " << playerID << endl;
    toPlayer = Opponents[playerID];
    packet << i << j << iP << jP << check;
    players[toPlayer]->send(packet);
    for(k = 0; k < matches.size(); k++){
        if(myID->second == matches[k].white.username || myID->second == matches[k].black.username){
            for(p = 0; p < matches[k].numberWatching; p++){
                auto watcher = IDsAccounts.find(matches[k].spectators[p].username);
                players[watcher->second]->send(packet);
            }
        }
    }
    //cout << "Sended move to " << toPlayer << endl;
}

void Network::sendMoveLog(int playerID, int toPlayer, string moveLog){
    Packet packet;
    //cout << "Movement log from player " << playerID << endl;
    toPlayer = Opponents[playerID];
    packet << playerID << static_cast<int> (packetID::MoveLog);
    packet << moveLog;
    players[toPlayer]->send(packet);
    //cout << "Sended movelog to " << toPlayer << endl;

}

void Network::sendGameEnd(int playerID, int toPlayer){
    Packet packet;
    packet << playerID << static_cast<int> (packetID::GameEnd);
    toPlayer = Opponents[playerID];
    int pA, pB;
    auto it2 = accountsIDs.find(toPlayer);
    auto myID = accountsIDs.find(playerID);
    //cout << "GameEnd from player " << playerID << endl;
    for(i = 0; i < userlist.size(); i++){
        if(userlist[i].username == myID->second){
            pB = i;
            userlist[i].status = statusID::online;
        }
        if(userlist[i].username == it2->second){
            pA = i;
            userlist[i].status = statusID::online;
        }
    }
    players[toPlayer]->send(packet);
    packet.clear();
    packet << toPlayer << static_cast<int> (packetID::GameEnd);
    players[playerID]->send(packet);
    //cout << "Sended GameEnd to " << toPlayer << endl;
    for(i = 0; i < matches.size(); i++){
        if(myID->second == matches[i].white.username || myID->second == matches[i].black.username){

            matches[i].status = 0;
            matches[i].winner = pA;
            matches[i].loser  = pB;

            if(matches[i].ranked){
                int mode = matches[i].gameMode;
                int time = matches[i].gameTime/900;
                int eloWon  = calculateELO(userlist[pA].elo[mode][time], userlist[pB].elo[mode][time], 1);
                int eloLost = calculateELO(userlist[pB].elo[mode][time], userlist[pA].elo[mode][time], 0);

                matches[i].eloWon = eloWon - userlist[pA].elo[mode][time];
                matches[i].eloLost= eloLost - userlist[pB].elo[mode][time];

                userlist[pA].elo[mode][time] = eloWon;
                userlist[pB].elo[mode][time] = eloLost;

                userlist[pA].victories[mode][time]++;
                userlist[pB].defeats[mode][time]++;

                logMatch(matches[i]);
            }

        }
    }
    sendMatches = true;
    updateMatches();
    saveAccounts();
}

void Network::sendGiveUp(int playerID, int toPlayer){
    Packet packet;
    packet << playerID << static_cast<int> (packetID::GiveUp);
    toPlayer = Opponents[playerID];
    int pA, pB;
    auto it2 = accountsIDs.find(toPlayer);
    auto myID = accountsIDs.find(playerID);

    //cout << "GiveUp from player " << playerID << endl;
    players[toPlayer]->send(packet);

    for(i = 0; i < userlist.size(); i++){
        if(userlist[i].username == myID->second){
            pB = i;
            userlist[i].status = statusID::online;
        }
        if(userlist[i].username == it2->second){
            pA = i;
            userlist[i].status = statusID::online;
        }
    }
    for(i = 0; i < matches.size(); i++){
        if(myID->second == matches[i].white.username || myID->second == matches[i].black.username){

            matches[i].status = 0;
            matches[i].winner = pA;
            matches[i].loser  = pB;

            if(matches[i].ranked){
                int mode = matches[i].gameMode;
                int time = matches[i].gameTime/900;
                int eloWon  = calculateELO(userlist[pA].elo[mode][time], userlist[pB].elo[mode][time], 1);
                int eloLost = calculateELO(userlist[pB].elo[mode][time], userlist[pA].elo[mode][time], 0);

                matches[i].eloWon = eloWon - userlist[pA].elo[mode][time];
                matches[i].eloLost= eloLost - userlist[pB].elo[mode][time];

                userlist[pA].elo[mode][time] = eloWon;
                userlist[pB].elo[mode][time] = eloLost;

                userlist[pA].victories[mode][time]++;
                userlist[pB].defeats[mode][time]++;

                logMatch(matches[i]);
            }

        }
    }
    sendMatches = true;
    updateMatches();
    saveAccounts();
    //cout << "Sended GiveUp to " << toPlayer << endl;
}

void Network::sendCheckMate(int playerID, int toPlayer){
    Packet packet;
    packet << playerID << static_cast<int> (packetID::Checkmate);
    toPlayer = Opponents[playerID];
    int pA, pB;
    auto it2 = accountsIDs.find(toPlayer);
    auto myID = accountsIDs.find(playerID);

    //cout << "Checkmate from player " << playerID << endl;
    players[toPlayer]->send(packet);
    for(i = 0; i < userlist.size(); i++){
        if(userlist[i].username == myID->second){
            pA = i;
            userlist[i].status = statusID::online;
        }
        if(userlist[i].username == it2->second){
            pB = i;
            userlist[i].status = statusID::online;
        }
    }
    for(i = 0; i < matches.size(); i++){
        if(myID->second == matches[i].white.username || myID->second == matches[i].black.username){
            int mode = matches[i].gameMode;
            int time = matches[i].gameTime/900;
            int eloWon  = calculateELO(userlist[pA].elo[mode][time], userlist[pB].elo[mode][time], 1);
            int eloLost = calculateELO(userlist[pB].elo[mode][time], userlist[pA].elo[mode][time], 0);
            matches[i].status = 0;
            matches[i].winner = pA;
            matches[i].loser  = pB;
            matches[i].eloWon = eloWon - userlist[pA].elo[mode][time];
            matches[i].eloLost= eloLost - userlist[pB].elo[mode][time];
            logMatch(matches[i]);
            userlist[pA].elo[mode][time] = eloWon;
            userlist[pB].elo[mode][time] = eloLost;

            userlist[pA].victories[mode][time]++;
            userlist[pB].defeats[mode][time]++;
        }
    }

    saveAccounts();
    updateMatches();
    sendMatches = true;
    //cout << "Sended CheckMate to " << toPlayer << endl;
}

void Network::sendChatMessage(int playerID, int toPlayer, string msg){
    Packet packet;
    packet << playerID << static_cast<int> (packetID::Chat);
    toPlayer = Opponents[playerID];
    auto it = accountsIDs.find(playerID);
    string name = it->second;
    packet << name + ":\n" + msg;
    players[toPlayer]->send(packet);
}

void Network::sendSpectatorChatMessage(int playerID, string msg){
    Packet packet;
    int i, j, p;
    packet << playerID << static_cast<int> (packetID::Chat);
    auto it = accountsIDs.find(playerID);
    string name = it->second;
    packet << name + ":\n" + msg;

    for(i = 0; i < matches.size(); i++){
        for(j = 0; j < matches[i].numberWatching; j++){
            if(it->second == matches[i].spectators[j].username){
                for(p = 0; p < matches[i].numberWatching; p++){
                    auto watcher = IDsAccounts.find(matches[i].spectators[p].username);
                    if(name != matches[i].spectators[p].username)
                        players[watcher->second]->send(packet);
                }
            }
        }
    }
}

void Network::sendGameRequest(int playerID, int toPlayer){
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
    //cout << "Sended game request to " << toPlayer << endl;
}

void Network::sendWatchState(int playerID, int matchID){
    int i, j, k;
    Packet packet;
    packet << playerID << static_cast<int>(packetID::WatchState);
    for(i = 0; i < matches.size(); i++){
        if(matches[i].ID == matchID){
            packet << matches[i].white.nickname << matches[i].black.nickname
                   << matches[i].white.elo[matches[i].gameMode][matches[i].gameTime/900]
                   << matches[i].black.elo[matches[i].gameMode][matches[i].gameTime/900];
            for(j = 0; j < 8; j++){
                for(k = 0; k < 10; k++){
                    packet << matches[i].board[j][k];
                }
            }
            packet << matches[i].gameMode;
            players[playerID]->send(packet);
            break;
        }
    }
}

void Network::removeFromQueue(int playerID){
    auto player = accountsIDs.find(playerID);
    if(classic5Queue.front() == playerID){
        classic5Queue.pop();
    }
    if(classic15Queue.front() == playerID){
        classic15Queue.pop();
    }
    if(classic30Queue.front() == playerID){
        classic30Queue.pop();
    }

    if(random5Queue.front() == playerID){
        random5Queue.pop();
    }
    if(random15Queue.front() == playerID){
        random15Queue.pop();
    }
    if(random30Queue.front() == playerID){
        random30Queue.pop();
    }

    if(capa5Queue.front() == playerID){
        capa5Queue.pop();
    }
    if(capa15Queue.front() == playerID){
        capa15Queue.pop();
    }
    if(capa30Queue.front() == playerID){
        capa30Queue.pop();
    }
}

void Network::checkTimeout(){
    int i;
    for (i = 0; i < playersTime.size(); i++) {
        if((playersTime[i].getElapsedTime().asSeconds() >= timeout) && (userlist[i].status != statusID::offline)) {
            if(userlist[i].status == statusID::playing)
                sendGameEnd(i, Opponents[i]);
            if(userlist[Opponents[i]].status == statusID::playing)
                sendGameEnd(Opponents[i], i);

            logHistory ("disconected(t.o.)", userlist[i].username);
            userlist[i].status = statusID::offline;
            //players[i]->disconnect();
        }
    }
}

void Network::updateBoard(int playerID, string newBoard[][10]){
    auto it = accountsIDs.find(playerID);
    int i, k, p;
    for(auto& player : userlist){
        if(it->second == player.username){
            for(i = 0; i < matches.size(); i++){
                if(it->second == matches[i].white.username || it->second == matches[i].black.username){
                    for(k = 0; k < 8; k++){
                        for(p = 0; p < 10; p++){
                            matches[i].board[k][p] = newBoard[k][p];
                        }
                    }
                }
            }
        }
    }
}

void Network::sendFischerPiecesOrder(int playerID, string newBoard[][10]){
    auto it = accountsIDs.find(playerID);
    int k, p, toPlayer;
    Packet packet;
    packet << playerID << static_cast<int>(packetID::FischerPieceOrder);

    toPlayer = Opponents[playerID];
    for(k = 0; k < 8; k++){
        for(p = 0; p < 10; p++){
            packet << newBoard[k][p];
        }
    }
    players[toPlayer]->send(packet);
}

void Network::updateMatches(){
    int i;
    vector<Match> aux;
    for(i = 0; i < matches.size(); i++){
        if(matches[i].status){
            aux.emplace_back(matches[i]);
        }
    }
    matches.clear();
    for(i = 0; i < aux.size(); i++){
        matches.emplace_back(aux[i]);
    }
}

void Network::sendInviteResponse(int playerID, string invitee, int gameMode, int gameTime, bool isPublic, bool response){
    string user;

    for(int i = 0; i < userlist.size(); i++){
        if(userlist[i].nickname == invitee){
            user = userlist[i].username;
        }
    }
    auto it = IDsAccounts.find(user);

    if(response)
        processRequest(playerID, it->second, static_cast<gameType>(gameMode), gameTime, isPublic, 0);
    else
        sendInviteRejection(playerID, it->second, user);
}

void Network::sendInviteRejection(int playerID, int toPlayer, string user){
    Packet packet;
    packet << playerID << static_cast<int>(packetID::InviteRejection) << user;

    players[toPlayer]->send(packet);
}
