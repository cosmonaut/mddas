/* Simple script to make a test connection to the deweplugin */

#include <iostream>           // For cerr and cout
#include <cstdlib>            // For atoi()
#include <unistd.h>           // sleep()...
#include <stdint.h>

#include "socket.h"  // Gotta have socket magic

using namespace std;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0]
             << " <Server> <Server Port>" << endl;
        exit(1);
    }

    string servAddress = argv[1]; // First arg: server address
    unsigned short echoServPort = atoi(argv[2]); // second arg port

    cout << "server: " << servAddress << endl;
    cout << "port: " << echoServPort << endl;

    try {
        // Establish connection with the echo server
        TCPSocket sock(servAddress, echoServPort);
        /* packet buffer */
        uint8_t pak[1400];

        //(uint64_t)pak[0] = 0x0001020304050607;
        //((uint64_t *)pak)[0] = 0x0001020304050607;
        pak[0] = 0x00;
        pak[1] = 0x01;
        pak[2] = 0x02;
        pak[3] = 0x03;
        pak[4] = 0x04;
        pak[5] = 0x05;
        pak[6] = 0x06;
        pak[7] = 0x07;

        /* DEWEsoft packet size in bytes */
        *((int32_t *)(pak + 8)) = (int32_t)28;
        /* packet type */
        *((int32_t *)(pak + 12)) = (int32_t)0;
        /* samples per channel */
        *((int32_t *)(pak + 16)) = (int32_t)1;
        /* samples acquired so far ... */
        *((int64_t *)(pak + 20)) = (int64_t)1;
        /* absolute/relative time (days) */
        *((double *)(pak + 28)) = (double)3.14159265;


        *((uint64_t *)(pak + 36)) = (uint64_t)0x0001020304050607;

        sock.send(pak, 44);

        sleep(5);

        // pak[0] = 0x07;
        // pak[1] = 0x06;
        // pak[2] = 0x05;
        // pak[3] = 0x04;
        // pak[4] = 0x03;
        // pak[5] = 0x02;
        // pak[6] = 0x01;
        // pak[7] = 0x00;


        sleep(5);
        /* Destructor closes the socket */

    } catch(SocketException &e) {
        cerr << e.what() << endl;
        exit(1);
    }

    return 0;
}
