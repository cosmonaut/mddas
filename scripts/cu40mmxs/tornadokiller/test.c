#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


#include "qdbmp.h"


// gcc test.c qdbmp.c -Wall -o test
// NOTE TO SELF: SEE THE ntohs() function to swap bytes to proper endian order.
#define OSSY_PORT "60000"	// the port users will be connecting to
#define OSSY_ADDR "127.0.0.1"   // local test address
//#define OSSY_DETECTOR_SIZE 8192 // Max detector X and Y limits
#define OSSY_DETECTOR_SIZE 8192 // Max detector X and Y limits
#define OSSY_PHD_SIZE 255        // Max PHD limit

#define PACKET_SIZE 1470
#define PACKET_LEN PACKET_SIZE/2
#define PACKET_ROWS PACKET_LEN/3
#define PACKET_COLS 3

#define COLX 0
#define COLY 1
#define COLP 2



int main( int argc, char* argv[]) {
    BMP*    bmp;
    UCHAR   r, g, b;
    UINT    width, height;
    //UINT    x, y;
    UINT    x;
    int y = 0;

    int i = 0;
    int j = 0;
    int k = 0;
    int l = 0;
    uint16_t p_index = 0;
    uint16_t p_count = 0;

    uint16_t grey = 0;
    uint16_t gmagic = 0;

    uint32_t num_photons = 0;
    uint16_t pix_count = 0;

    uint16_t n_packets = 0 ;

    /* packet stuff */
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    uint16_t packet[PACKET_LEN];
    uint16_t phot_buf[65536];
    uint32_t phot_ind = 0;
    //uint16_t *pptr = packet;
    int rv;
    int numbytes = 0;

    
    if ( argc != 2 ) {
        fprintf( stderr, "Usage: %s <input file>\n", argv[ 0 ] );
        return 0;
    }

    memset(&packet, 0, sizeof(packet));


    /* Socket! */
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





    /* Read an image file */
    bmp = BMP_ReadFile( argv[ 1 ] );
    BMP_CHECK_ERROR( stderr, -1 ); /* If an error has occurred, notify and exit */

    /* Get image's dimensions */
    width = BMP_GetWidth( bmp );
    height = BMP_GetHeight( bmp );

    if (width != 1024) {
        printf("Image has bad width %i!\n", width);
        BMP_Free( bmp );
        
        freeaddrinfo(servinfo);
        close(sockfd);
        return -1;
    }

    if (height != 1024) {
        printf("Image has bad height %i!\n", height);
        BMP_Free( bmp );
        
        freeaddrinfo(servinfo);
        close(sockfd);
        return -1;
    }



    printf("width: %d, height: %d\n", width, height);

    /* Iterate through all the image's pixels */
    for ( x = 0 ; x < width; ++x ) {
        //printf("ASDF\n");
        for ( y = 0 ; y < height; ++y ) {
        /* Because bitmaps are lame and upside down */
        //for (y = (height - 1); y < 0; --y ) {
        //for (y = (height - 1); y --> 0; ) {
            //printf("ASDF\n");

            /* Get info for every pixel */
            BMP_GetPixelRGB( bmp, x, y, &r, &g, &b );
            /* Turn it into greyscale (counts) */
            grey = (uint16_t)(0.2126*((float)r) + 0.7152*((float)g) + 0.0722*((float)b));
            grey = grey/16;

            pix_count = grey;
            // if (pix_count != 15) {
            //     printf("WTFBBQ?\n");
            // }
            //printf("x: %i, y: %i, pix: %d\n", x, y, pix_count);

            /* rebin image (1024 -> 16384) */
            for (k = 1; k < 16; k += 2) {
                for (l = 1; l < 16; l += 2) {
                    for (j = 0; j < (pix_count); j++) {

                        if ((phot_ind + 4) > 65535) {
                            printf("FAIL HARD\n");
                        }
                        //printf("k: %i, l: %i\n", k, l);
                        phot_buf[phot_ind] = (uint16_t)((x*16) + k);
                        phot_ind++;
                        phot_buf[phot_ind] = (uint16_t)(((height - 1) - y)*16) + l;
                        //phot_buf[phot_ind] = (y*16) + k;
                        phot_ind++;
                        phot_buf[phot_ind] = 64;
                        phot_ind++;

                    }
                }
            }

            if (phot_ind > 732) {
                //n_packets = phot_ind/244;
                n_packets = (phot_ind - 1)/732;

                for (i = 0; i < n_packets; i++) {
                    packet[0] = 244;
                    packet[1] = p_count;
                    packet[2] = 0;
                    
                    memcpy(&packet[3], &phot_buf[i*(244*3)], 732*2);
                        
                    // printf("dumping packet...\n");
                    // for (j = 0; j < 735; j++) {
                    //     printf("j: %d, %u\n", j, packet[j]);
                    //     //printf("phot_buf: %d, %u\n", j, phot_buf[j]);
                    // }
                    // printf("pix_count: %i\n", pix_count);
                    // printf("phot_ind: %i\n", phot_ind);
                    
                    // return -1;

                    /* shoot it to the listener */
                    if ((numbytes = sendto(sockfd, packet, sizeof(packet), 0,
                                           p->ai_addr, p->ai_addrlen)) == -1) {
                        perror("talker: sendto");
                        //exit(1);
                    }
                    p_count++;
                    usleep(30);
                }

                //memcpy(&phot_buf[0], &phot_buf[n_packets*732], 2*((phot_ind - (n_packets*732))));
                memmove(&phot_buf,  &phot_buf[n_packets*732], 2*((phot_ind - (n_packets*732)) - 1));

                phot_ind = phot_ind - (n_packets * 732);
                //printf("phot_ind: %i\n", phot_ind);


                    // /* Get pixel's RGB values */
                    // BMP_GetPixelRGB( bmp, x, y, &r, &g, &b );
                    // /* Turn it into greyscale (counts) */
                    // grey = (uint16_t)(0.2126*((float)r) + 0.7152*((float)g) + 0.0722*((float)b));
                    // grey = grey/16;

                    // gmagic = (grey/244 + 1);
                    // for (i = 0; i < gmagic; i++) {
                    //     switch(i < (gmagic - 1)) {
                    //     case(0):
                    //         pix_count = 244;
                    //         break;
                    //     case(1):
                    //         printf("YEAH!\n");
                    //         pix_count = grey - (244*(gmagic - 1));
                    //         //printf("pixcount: %u\n", pix_count);
                    //         break;
                    //     default:
                    //         printf("ERROR!\n");
                    //     }

                    //     packet[0] = pix_count;
                    //     packet[1] = p_count;
                    //     packet[2] = 0;

                    //     for (j = 3; j < (3*pix_count + 3) ; j+=3) {
                    //         packet[j] = (uint16_t)((x << 1) + k);
                    //         packet[j + 1] = (uint16_t)((y << 1) + k);
                    //         packet[j + 2] = (uint16_t)64;
                    //     }

                    //     /* shoot it to the listener */
                    //     if ((numbytes = sendto(sockfd, packet, sizeof(packet), 0,
                    //                            p->ai_addr, p->ai_addrlen)) == -1) {
                    //         perror("talker: sendto");
                    //         //exit(1);
                    //     }
                    //     p_count++;
                    //     usleep(10);
                
                    // }
                
                    //pix_count = grey;
                    
                    

                    // packet[0] = pix_count;
                    // packet[1] = p_count;
                    // packet[2] = 0;

                    // //printf("pix_count: %u\n", pix_count);
                    // //printf(" x: %u y: %u\n", (uint16_t)((x << 1) + k), (uint16_t)((y << 1) + k));

                    // for (j = 3; j < (3*pix_count + 3) ; j+=3) {
                    //     packet[j] = (uint16_t)((x << 1) + k);
                    //     packet[j + 1] = (uint16_t)((y << 1) + k);
                    //     packet[j + 2] = (uint16_t)64;
                    // }

                    // /* shoot it to the listener */
                    // if ((numbytes = sendto(sockfd, packet, sizeof(packet), 0,
                    //                        p->ai_addr, p->ai_addrlen)) == -1) {
                    //     perror("talker: sendto");
                    //     //exit(1);
                    // }
                    // p_count++;
                    // usleep(10);
                
                    // memset(&packet, 0, sizeof(packet));

                    // num_photons += grey;
            
            
                    //printf("grey: %u\n", grey);            
                    //printf("r: %d, g: %d, b: %d\n", r, g, b);
            
                    //usleep(10);
            
            }
        }
    }
    //}
    
    BMP_CHECK_ERROR( stderr, -2 );

    printf("total photons: %u\n", num_photons);

    /* Free all memory allocated for the image */
    BMP_Free( bmp );

    freeaddrinfo(servinfo);
    close(sockfd);
    



    return 0;
}
