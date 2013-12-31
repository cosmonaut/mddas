/* A Quick and hacky script to replay chapter 10 data from a pcap
   file... 


   gcc -Wall replay.c -o replay -lpcap
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcap.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <net/if.h>
#include <stdint.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


#define CH10_ADDR "127.0.0.1"
#define CH10_PORT_INFO "27500"
#define CH10_PORT 27500
#define SIZE_ETHERNET 14
#define SIZE_UDP 8

struct sniff_ip {
    u_char ip_vhl;		/* version << 4 | header length >> 2 */
    u_char ip_tos;		/* type of service */
    u_short ip_len;		/* total length */
    u_short ip_id;		/* identification */
    u_short ip_off;		/* fragment offset field */
#define IP_RF 0x8000		/* reserved fragment flag */
#define IP_DF 0x4000		/* dont fragment flag */
#define IP_MF 0x2000		/* more fragments flag */
#define IP_OFFMASK 0x1fff	/* mask for fragmenting bits */
    u_char ip_ttl;		/* time to live */
    u_char ip_p;		/* protocol */
    u_short ip_sum;		/* checksum */
    struct in_addr ip_src,ip_dst; /* source and dest address */
};

struct sniff_udp {
    u_short uh_sport;               /* source port */
    u_short uh_dport;               /* destination port */
    u_short uh_ulen;                /* udp length */
    u_short uh_sum;                 /* udp checksum */
};

#define IP_HL(ip)		(((ip)->ip_vhl) & 0x0f)

int replay_packet(const unsigned char *packet, unsigned int capture_len, uint32_t port, uint16_t *buffer, uint16_t *buf_len);


int main(int argc, char **argv) { 
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    
    // unsigned int pkt_counter=0;   // packet counter 
    // unsigned long byte_counter=0; //total bytes seen in entire trace 
    // unsigned long cur_counter=0; //counter for current 1-second interval 
    // unsigned long max_volume = 0;  //max value of bytes in one-second interval 
    // unsigned long current_ts=0; //current timestamp 
    int i = 0;
    int status = 0;

    pcap_t *handle; 

    //temporary packet buffers 
    struct pcap_pkthdr header; // The header that pcap gives us 
    const u_char *packet; // The actual packet 

    uint16_t buffer[10000];
    uint16_t *buf_pt = buffer;
    uint16_t buf_len = 0;
    uint32_t udp_port = 0;

    char errbuf[PCAP_ERRBUF_SIZE]; //not sure what to do with this, oh well 
  
    //check command line arguments 
    if (argc < 3) { 
        fprintf(stderr, "Usage: %s <pcap_file> <udp_dest_port>\n", argv[0]); 
        //exit(1); 
        return -1;
    } 

    printf("opening file %s...\n", argv[1]);

    udp_port = (uint32_t)atoi(argv[2]);

    handle = pcap_open_offline(argv[1], errbuf);
    if (handle == NULL) { 
        fprintf(stderr,"Couldn't open pcap file %s: %s\n", argv[1], errbuf); 
        return(2); 
    } 


    /* udp stuff */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(CH10_ADDR, CH10_PORT_INFO, &hints, &servinfo)) != 0) {
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
        return -2;
        pcap_close(handle);
        freeaddrinfo(servinfo);
            
    }



    
    while ((packet = pcap_next(handle, &header)) != NULL) {
        status = replay_packet(packet, header.caplen, udp_port, buf_pt, &buf_len);
        if (status < 0) {
            printf("ignoring a packet...\n");
        } else {
            if (buf_len != 244) {
                printf("buflen: %i\n", buf_len);
            }
            /* shoot it to the listener */
            if ((numbytes = sendto(sockfd, buffer, buf_len, 0,
                                   p->ai_addr, p->ai_addrlen)) == -1) {
                perror("talker: sendto");
                exit(1);
            }
        
            usleep(10);

        }
            
    }

    pcap_close(handle);
    freeaddrinfo(servinfo);
    close(sockfd);

    printf("END OF LINE\n");

    return 0;
}

int replay_packet(const unsigned char *packet, unsigned int capture_len, uint32_t port, uint16_t *buffer, uint16_t *buf_len) {
    //int replay_packet(const unsigned char *packet, unsigned int capture_len) {
    //struct ip *ip;
    const struct sniff_ip *ip; /* The IP header */
    const struct sniff_udp *udp;            /* The UDP header */
    int size_ip;
    int size_payload;
    const char *payload;                    /* Packet payload */
    int i = 0;

    if (capture_len < sizeof(struct ether_header)) {
        /* We didn't even capture a full Ethernet header, so we
         * can't analyze this any further.
         */
        //too_short(ts, "Ethernet header");
        printf("too short\n");
        return -1;
    }

    ip = (struct sniff_ip*)(packet + SIZE_ETHERNET);
    size_ip = IP_HL(ip)*4;
    if (size_ip < 20) {
        printf("   * Invalid IP header length: %u bytes\n", size_ip);
        return -1;
    }
    
    /* determine protocol */	
    switch(ip->ip_p) {
    case IPPROTO_UDP:
        //printf("   Protocol: UDP\n");
        break;
    default:
        printf("   Protocol: unknown\n");
        return -1;
    }

    if (capture_len < (SIZE_ETHERNET + size_ip + SIZE_UDP)) {
        printf("packet not long enough!\n");
        return -1;
    }

    udp = (struct sniff_udp*)(packet + SIZE_ETHERNET + size_ip);
    // printf("   Dst port: %d\n", ntohs(udp->uh_dport));
    // printf("   Src port: %i\n", ntohs(udp->uh_sport));
    // printf("   ulen port: %d\n", ntohs(udp->uh_ulen));
    // printf("   sum port: %d\n", ntohs(udp->uh_sum));
        
    //printf("udp port: %i", udp->uh_dport);
    if (ntohs(udp->uh_dport) != port) {
        return -1;
        //printf("other port: %d", ntohs(udp->uh_dport));
    }

    /* define/compute udp payload (segment) offset */
    payload = (u_char *)(packet + SIZE_ETHERNET + size_ip + SIZE_UDP);
    
    /* compute udp payload (segment) size */
    size_payload = ntohs(ip->ip_len) - (size_ip + SIZE_UDP);
    // if (size_payload > ntohs(udp->uh_ulen))
    //     size_payload = ntohs(udp->uh_ulen);
    if (size_payload < 1) {
        printf("SIZE: %i\n", size_payload);
        return -1;
    } else {
        *buf_len = size_payload;
    }
    // for (i = 0; i < size_payload; i++) {
    //     printf("%i\n", *((uint8_t *)(payload + i)));

    // }
    
    memcpy(buffer, payload, size_payload);
    //buffer = (uint16_t *)payload;

    
    return 0;
}
