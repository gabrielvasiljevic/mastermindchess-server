#include "player.h"


 Player::Player(string username, string password, string nickname, string email){
        this->username = username;
        this->password = password;
        this->nickname = nickname;
        this->email = email;

        normal_5_ELO = normal_15_ELO = normal_30_ELO = 950;
        capa_5_ELO   = capa_15_ELO   = capa_30_ELO   = 950;
        fischer_5_ELO= fischer_15_ELO= fischer_30_ELO= 950;

        number_of_matches = 0;
        K = 30;

        active = true;
}


 Player::Player(){

        normal_5_ELO = normal_15_ELO = normal_30_ELO = 950;
        capa_5_ELO   = capa_15_ELO   = capa_30_ELO   = 950;
        fischer_5_ELO= fischer_15_ELO= fischer_30_ELO= 950;

        number_of_matches = 0;
        K = 30;

        active = true;
}
