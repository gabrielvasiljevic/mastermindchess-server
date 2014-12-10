#include <iostream>
#include <math.h>
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
#include "utility.h"

using namespace std;

bool isEmpty(std::ifstream& pFile){
    return pFile.peek() == std::ifstream::traits_type::eof();
}

int StringToNumber ( const string &Text ){  //Text not by const reference so that the function can be used with a
	stringstream ss(Text);  //character array as argument
	int result;
	return ss >> result ? result : 0;
}


string getDatestamp(char separator){
	using namespace std::chrono;
    #define padIt(_x_) (now_in._x_ < 10 ? "0" : "") << now_in._x_

    ostringstream output;
    system_clock::time_point now = system_clock::now();
    time_t now_c = system_clock::to_time_t(now);
    tm now_in = *(localtime(&now_c));
    // output <<  now_in.tm_year + 1900 << "-" << padIt(tm_mon + 1) << "-" << padIt(tm_mday);
    if (separator=='-') {
        output << now_in.tm_year + 1900 << separator << padIt(tm_mon + 1) << separator << padIt(tm_mday);
        return output.str();
    }
    output << padIt(tm_mday) << separator << padIt(tm_mon + 1) << separator << now_in.tm_year + 1900;
    return output.str();
}

string getTimestamp () {
    using namespace std::chrono;
    #define padIt(_x_) (now_in._x_ < 10 ? "0" : "") << now_in._x_

    ostringstream output;
    system_clock::time_point now = system_clock::now();
    time_t now_c = system_clock::to_time_t(now);
    tm now_in = *(localtime(&now_c));
    output <<  padIt(tm_hour) << ":" << padIt(tm_min) << ":" << padIt(tm_sec)
           << "";
    return output.str();
}


string getDatestamp (bool date, bool hour) {
    using namespace std::chrono;
    #define padIt(_x_) (now_in._x_ < 10 ? "0" : "") << now_in._x_

    ostringstream output;
    system_clock::time_point now = system_clock::now();
    time_t now_c = system_clock::to_time_t(now);
    tm now_in = *(localtime(&now_c));
    output << "";
        if(date){
            output  <<  padIt(tm_mday)    << "/"
                    <<  padIt(tm_mon + 1) << "/"
                    <<  now_in.tm_year + 1900 << " ";
        }
        if(hour){
            output  <<  padIt(tm_hour) << ":" << padIt(tm_min) << ":" << padIt(tm_sec)
                    << "";
        }
    return output.str();
}
