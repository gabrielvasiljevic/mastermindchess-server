#ifndef _UTIL
#define _UTIL

#include <SFML/Network.hpp>
#include <SFML/System.hpp>


using namespace std;

bool isEmpty(std::ifstream& pFile);

int StringToNumber ( const string &Text );

string getDatestamp (char separator);

string getTimestamp ();

string getDatestamp (bool date, bool hour);


#endif // _UTIL
