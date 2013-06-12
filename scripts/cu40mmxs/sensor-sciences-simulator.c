#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


// NOTE TO SELF: SEE THE ntohs() function to swap bytes to proper endian order.
#define OSSY_PORT "60000"	// the port users will be connecting to
#define OSSY_ADDR "127.0.0.1"   // local test address
//#define OSSY_DETECTOR_SIZE 8192 // Max detector X and Y limits
#define OSSY_DETECTOR_SIZE 16384 // Max detector X and Y limits
#define OSSY_PHD_SIZE 64        // Max PHD limit

#define PACKET_SIZE 1470
#define PACKET_LEN PACKET_SIZE/2
#define PACKET_ROWS PACKET_LEN/3
#define PACKET_COLS 3

#define COLX 0
#define COLY 1
#define COLP 2


int main(int argc, char *argv[])
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    uint16_t packet[PACKET_LEN];
    int i = 0;
    int j = 0;
    int k = 0;
    int go = 1;
    unsigned int iseed = 0;
    uint16_t p_count = 0;


    /* seed with the time */
    iseed = (unsigned int)time(NULL);
    srand(iseed);

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


    printf("About to loop the datas\n");
    printf("PACKET ROWS:%i \n", PACKET_ROWS);
    while(go) {

        for (j = 0; j < OSSY_DETECTOR_SIZE; j++) {
            for (k = 0; k < OSSY_DETECTOR_SIZE; k++) {
                
                if (i == 0) {

                    packet[0*PACKET_COLS + 0] = 244;
                    packet[0*PACKET_COLS + 1] = p_count;
                    packet[0*PACKET_COLS + 2] = 0x0000;
                    i = 1;
                }

                

                /* generate fake data */
                //for (i = 1; i < PACKET_ROWS; i++) {
                if ((j)%2 == 0) {
                    if (k%2 == 0) {
                        packet[i*PACKET_COLS + COLX] = j;
                        packet[i*PACKET_COLS + COLY] = k;
                        packet[i*PACKET_COLS + COLP] = rand()%OSSY_PHD_SIZE;
                        //printf("j: %i k: %i\n", j, k);
                        
                        //}
                        i++;
                    }
                } else {
                    if (k%2 == 1) {
                        packet[i*PACKET_COLS + COLX] = j;
                        packet[i*PACKET_COLS + COLY] = k;
                        packet[i*PACKET_COLS + COLP] = rand()%OSSY_PHD_SIZE;
                        //printf("j: %i k: %i\n", j, k);
                        
                        //}
                        i++;
                    }
                }
                //printf("i: %i\n", i);

                if (i == 245) {
                    /* shoot it to the listener */
                    if ((numbytes = sendto(sockfd, packet, sizeof(packet), 0,
                                           p->ai_addr, p->ai_addrlen)) == -1) {
                        perror("talker: sendto");
                        exit(1);
                    }
                    
                    p_count++;
                    
                    usleep(100);
                    //usleep(500000);
                    i = 0;
                }

            }
        }
    }



    freeaddrinfo(servinfo);

    printf("talker: sent %d bytes to %s\n", numbytes, OSSY_ADDR);
    close(sockfd);

    return 0;
}
