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


#define OSSY_PORT "60000"	// the port users will be connecting to
#define OSSY_ADDR "127.0.0.1"   // local test address
#define OSSY_DETECTOR_SIZE 16384 // Max detector X and Y limits
#define OSSY_PHD_SIZE 256        // Max PHD limit

#define PACKET_SIZE 1470
#define PACKET_LEN PACKET_SIZE/2
#define PACKET_ROWS PACKET_LEN/3
#define PACKET_COLS 3

#define COLX 0
#define COLY 1
#define COLP 2


int main(void) {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    uint16_t packet[PACKET_LEN];
    int i = 0;
    int k = 0;
    uint16_t p_count = 0;

    /* Hold the whole data chunk */
    //uint16_t megadata[5996544]; // too big for stack!
    uint16_t *megadata;
    uint32_t m_ind = 0;


    megadata = malloc(5996544*sizeof(uint16_t));
    if (megadata == NULL) {
        printf("MALLOC FAIL!\n");
        return -1;
    }


    memset(&hints, 0, sizeof(hints));
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

    /* I AM THE TABLE! */
    for (k = 0; k < 244; k++) {
        for (i = 1; i < 16384; i+=2) {
            megadata[m_ind] = (uint16_t)i;
            m_ind++;
            megadata[m_ind] = (uint16_t)i;
            m_ind++;
            megadata[m_ind] = (uint16_t)k;
            m_ind++;
        }        
    }

    printf("MEGADATA INDEX: %i\n", m_ind);
    printf("TOTAL PHOTONS: %i\n\n", m_ind/3);

    printf("Sending packets...\n");
    for (i = 0; i < 8192; i++) {
        
        packet[0] = (uint16_t)244;
        packet[1] = p_count;
        packet[2] = (uint16_t)0x0000;
        p_count++;

        memcpy(&packet[3], &megadata[i*732], sizeof(uint16_t)*732);

        /* shoot it to the listener */
        if ((numbytes = sendto(sockfd, packet, sizeof(packet), 0,
                               p->ai_addr, p->ai_addrlen)) == -1) {
            perror("talker: sendto");
            exit(1);
        }
        
        usleep(1000);
    }

    
    printf("All packets sent\n");


    free(megadata);

    freeaddrinfo(servinfo);

    printf("END OF LINE\n");
    close(sockfd);

    return 0;
}
