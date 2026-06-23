// #include <cstring>
// #include <iostream>
// #include <fstream>
// using namespace std;

// int main(int argc, char* argv[]) {
//     // Flush after every cout / cerr
//     cout << unitbuf;
//     cerr << unitbuf;

//     // You can use print statements as follows for debugging, they'll be visible when running tests.
//     cerr << "Logs from your program will appear here" << endl;

//     if (argc != 3) {
//         cerr << "Expected two arguments" << endl;
//         return 1;
//     }

//     string file_name = argv[1];
//     string command = argv[2];  

//     if (command == ".dbinfo") {
//         ifstream dbfile(file_name, ios::binary);
//         if (!dbfile) {
//             cerr << "Failed to open the database file" << endl;
//             return 1;
//         }

//         // TODO: Uncomment the code below to pass the first stage
//         dbfile.seekg(16);  // Skip the first 16 bytes of the header
        
//         char buffer[2];
//         dbfile.read(buffer, 2);
        
//         unsigned short pg_size = (static_cast<unsigned char>(buffer[1]) | (static_cast<unsigned char>(buffer[0]) << 8));
        
//         cout << "database page size: " << pg_size << endl;

//         dbfile.seekg(100+3);
//         char buffer2[2];
//         dbfile.read(buffer2,2);

//         int rows = (static_cast<unsigned char>(buffer2[1]) | static_cast<unsigned char>(buffer2[0])<<8);

//         cout << "number of tables: " << rows << endl;
//     }

//     return 0;
// }
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