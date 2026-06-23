#include <cstring>
#include <iostream>
#include <fstream>
#include <vector>
using namespace std;

int main(int argc, char* argv[]){
    // cerr << "\nStarted" << endl;

    string file_name = argv[1];
    string cmd = argv[2];

    ifstream dbfile(file_name,ios::binary);
    dbfile.seekg(16);

    char buffer[2];
    dbfile.read(buffer,2);

    int pg_size = (static_cast<unsigned char>(buffer[1]) | static_cast<unsigned char>(buffer[0])<<8);

    cout << "database page size: "<< pg_size << endl;

    dbfile.seekg(100+3);
    char buffer2[2];
    dbfile.read(buffer2,2);

    int rows = (static_cast<unsigned char>(buffer2[1]) | static_cast<unsigned char>(buffer2[0])<<8);

    cout << "number of tables: " << rows << endl;

    return 0;
}