#include <iostream>
#include <fstream>
using namespace std;
void logger(string s)
{
    ofstream file;
    file.open("logger.txt", ios_base::app);
    file << s;
    file << "\n";
    file.close();
    return;
}