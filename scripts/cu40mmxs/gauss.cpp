// #include <boost/math/distributions/normal.hpp> // for normal_distribution
// using boost::math::normal; // typedef provides default type is double.

// #include <iostream>


// int main() {
//     normal s;

    
// }



#include <boost/random.hpp>
#include <boost/random/normal_distribution.hpp>

#include <sys/types.h>
#include <sys/socket.h>
#include <ctime>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


// NOTE TO SELF: SEE THE ntohs() function to swap bytes to proper endian order.
#define OSSY_PORT "60000"	// the port users will be connecting to
#define OSSY_ADDR "127.0.0.1"   // local test address
//#define OSSY_DETECTOR_SIZE 8192 // Max detector X and Y limits
#define OSSY_DETECTOR_SIZE 16384 // Max detector X and Y limits
#define OSSY_PHD_SIZE 256        // Max PHD limit

#define PACKET_SIZE 1470
#define PACKET_LEN PACKET_SIZE/2
#define PACKET_ROWS PACKET_LEN/3
#define PACKET_COLS 3

#define COLX 0
#define COLY 1
#define COLP 2



int main() {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    uint16_t packet[PACKET_LEN];
    int i = 0;
    //int j = 0;
    //int k = 0;
    int go = 1;
    //unsigned int iseed = 0;
    uint16_t p_count = 0;



    boost::mt19937 rng; // I don't seed it on purpouse (it's not relevant)
    rng.seed(static_cast<unsigned int>(std::time(0)));
    boost::normal_distribution<> nd(8192.0, 400.0);
    boost::normal_distribution<> phd(120.0, 30.0);
    
    boost::variate_generator<boost::mt19937&, 
                             boost::normal_distribution<> > var_nor(rng, nd);

    boost::variate_generator<boost::mt19937&, 
                             boost::normal_distribution<> > var_phd(rng, phd);
    
    //std::cout << nd.max() << std::endl;


    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(OSSY_ADDR, OSSY_PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    /* loop through all the results and make a socket */
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("talker: socket");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "talker: failed to bind socket\n");
        return 2;
    }




    
    // int i = 0; 
    // for (; i < 10; ++i) {
    //     double d = var_nor();
    //     std::cout << d << std::endl;
    // }



    printf("About to loop the datas\n");
    printf("PACKET ROWS:%i \n", PACKET_ROWS);
    while(go) {

        packet[0*PACKET_COLS + 0] = 244;
        packet[0*PACKET_COLS + 1] = p_count;
        packet[0*PACKET_COLS + 2] = 0x0000;

                
        /* generate fake data */
        for (i = 1; i < PACKET_ROWS; i++) {
            double x = var_nor();
            double y = var_nor();
            double p = var_phd();
            packet[i*PACKET_COLS + COLX] = ((uint16_t)x)%OSSY_DETECTOR_SIZE;
            packet[i*PACKET_COLS + COLY] = ((uint16_t)y)%OSSY_DETECTOR_SIZE;
            packet[i*PACKET_COLS + COLP] = ((uint16_t)p)%OSSY_PHD_SIZE;
        }

        /* shoot it to the listener */
        if ((numbytes = sendto(sockfd, packet, sizeof(packet), 0,
                               p->ai_addr, p->ai_addrlen)) == -1) {
            perror("talker: sendto");
            exit(1);
        }
        
        p_count++;
        
        usleep(80);
        //usleep(500000);
        //i = 0;
    }




    freeaddrinfo(servinfo);

    printf("talker: sent %d bytes to %s\n", numbytes, OSSY_ADDR);
    close(sockfd);

    return 0;




}





