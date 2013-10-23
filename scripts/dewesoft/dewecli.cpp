/* Simple script to make a connection to the deweplugin */

#include <iostream>           // For cerr and cout
#include <cstdlib>            // For atoi()

#include "socket.h"  // Gotta have socket magic

using namespace std;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] 
             << " <Server> <Server Port>" << endl;
        exit(1);
    }

    string servAddress = argv[1]; // First arg: server address
    unsigned short echoServPort = atoi(argv[2]);

    cout << "server: " << servAddress << endl;
    cout << "port: " << echoServPort << endl;

    try {
        // Establish connection with the echo server
        TCPSocket sock(servAddress, echoServPort);
  
        // Destructor closes the socket

    } catch(SocketException &e) {
        cerr << e.what() << endl;
        exit(1);
    }

    return 0;
}
