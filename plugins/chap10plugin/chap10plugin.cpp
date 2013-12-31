/* Author: Nicholas Nell
   email: nicholas.nell@colorado.edu

   Sampling thread for Chapter 10 Ethernet data
*/

#include <QDebug>
#include <stdint.h>

#include "socket.h"
#include "chap10plugin.h"

#define CH10_PORT 27500
/* Why? because quakeworld was awesome. */
#define CH10_PAK_SIZE 65536
//#define CH10_PAK_SIZE 1400
#define CH10_HD_OFST 4

#define CH10_UDP_HDR_SIZE 4
#define CH10_HDR_SIZE 24
/* Mode we've been using so far... */
#define CH10_PCM_MODE 0x04

Chap10Plugin::Chap10Plugin() : SamplingThreadPlugin() {
    qDebug() << "Loading Chapter 10 plugin...";
    _sock = NULL;

    try {
        _sock = new UDPSocket(CH10_PORT);
    } catch (SocketException &e) {
        qDebug() << e.what();
        _sock = NULL;
    }

    if (_sock != NULL) {
        _sock->setNonBlocking(true);
        qDebug() << "Socket non blocking...";
    }
}

Chap10Plugin::~Chap10Plugin() {
    /* Stop thread! */
    mutex.lock();

    abort = true;
    condition.wakeOne();
    mutex.unlock();

    wait();

    /* Destroy stuff! */
    qDebug() << "Closing chap10 plugin...";
    if (_sock) {
        qDebug() << "deleting socket...";
        delete _sock;
    }
    
    _sock = NULL;
}

void Chap10Plugin::run() {
    QMutex sleepM;
    QVector<MDDASDataPoint> v;
    sleepM.lock();
    int ret = 0;
    /* Packet buffer */
    uint8_t pak_buf[65536];
    //uint16_t data_buf
    uint32_t *data_pt = 0;
    //uint16_t pak_size = 0;
    int32_t pak_size = 0;
    uint8_t p_status = 1;

    std::string srcAddr;
    short unsigned srcPort;


    /* Chapter 10 UDP Header */
    uint32_t udp_header = 0;
    uint32_t udp_seq = 0;
    uint32_t udp_seq_last = 0;
    uint8_t udp_type = 0;

    /* Chapter 10 header items */
    uint32_t ch10_pak_len = 0;
    uint32_t ch10_dat_len = 0;
    uint32_t ch10_hd_word = 0;

    uint8_t ch10_dtype = 0;

    uint32_t ch10_rel_word = 0;

    uint16_t ch10_chksum = 0;
    uint16_t chksum = 0;

    /* Chapter 10 PCM HEADER */
    uint32_t ch10_ch_spec = 0;
    uint8_t ch10_pcm_mode = 0;


    /* packet internals */
    // int32_t psize = 0;
    // int32_t ptype = 0;
    // int32_t psamps = 0;
    // int64_t ptsamps = 0;
    // double pts = 0.0;
    // uint64_t pstart = 0;
    // uint64_t pend = 0;
    // /* pointers for casts... */
    // uint64_t* p_start_p;


    /* Data loop. */
    forever {
        mutex.lock();
        if (pauseSampling) {
            condition.wait(&mutex);
        }
        mutex.unlock();

        if (abort) {
            qDebug() << "called abort!" << QThread::currentThread();
            return;
        }

        
        while((pak_size = _sock->recvFrom(pak_buf, CH10_PAK_SIZE, srcAddr, srcPort)) > 0) {
            if (pak_size > 4) {
                udp_header = *((uint32_t *)(pak_buf + 0));
                //             udpseq = (udpheader & 0xffffff00) >> 8
                // udpver = udpheader & 0x0000000f
                // udptype = udpheader & 0x000000f0
                
                // print("seq: %i" % udpseq)
                // print("ver: %i" % udpver)
                // print("type: %i" % udptype)

                udp_seq = ((udp_header & 0xffffff00) >> 8);
                udp_type = ((udp_header & 0x000000f0) >> 4);
                //qDebug() << "udp seq: " << udp_seq << " udp_type: " << udp_type;
                if (udp_seq != (udp_seq_last + 1)) {
                    qDebug() << "UDP Seq mismatch, seq: " << udp_seq << " last: " << udp_seq_last;
                }
                udp_seq_last = udp_seq;
                

                /* UDP type 0 -- not fragmented */
                if ((udp_type == 0x00) && (pak_size > (CH10_HDR_SIZE + CH10_UDP_HDR_SIZE))) {
                    // parse away!
                    //plen = struct.unpack('I', p[HD_OFST + 4: HD_OFST + 8])
                    //  dlen = struct.unpack('I', p[HD_OFST + 8: HD_OFST + 12])

                    ch10_pak_len = *((uint32_t *)(pak_buf + CH10_HD_OFST + 4));
                    ch10_dat_len = *((uint32_t *)(pak_buf + CH10_HD_OFST + 8));
                    
                    //qDebug() << "pak_len: " << ch10_pak_len;
                    //qDebug() << ch10_dat_len;

                    // hd_word = struct.unpack('I', p[HD_OFST + 12: HD_OFST + 16])
                    // hd_word = hd_word[0]
                    // # dtype = struct.unpack('B', p[HD_OFST + 12: HD_OFST + 13])
                    // # pflag = struct.unpack('B', p[HD_OFST + 13: HD_OFST + 14])
                    // # seqn =  struct.unpack('B', p[HD_OFST + 14: HD_OFST + 15])
                    // # dtypever = struct.unpack('B', p[HD_OFST + 15: HD_OFST + 16])

                    // dtype = (hd_word & 0xff000000) >> 24
                    // pflag = (hd_word & 0x00ff0000) >> 16
                    // seqn = (hd_word & 0x0000ff00) >> 8
                    // dtypever = (hd_word & 0x000000ff)
                    
                    ch10_hd_word = *((uint32_t *)(pak_buf + CH10_HD_OFST + 12));

                    ch10_dtype = ((ch10_hd_word & 0xff000000) >> 24);

                    ch10_rel_word = *((uint32_t *)(pak_buf + CH10_HD_OFST + 20));
                    ch10_chksum = ((ch10_rel_word & 0xffff0000) >> 16);

                    for (uint8_t i = 0; i < 11; i++) {
                        chksum += *((uint16_t *)(pak_buf + CH10_HD_OFST + i*2));
                    }
                    if (ch10_chksum != chksum) {
                        qDebug() << "BAD CHECKSUM!";
                        qDebug() << "CH10 CHKSUM: " << ch10_chksum << " calc: " << chksum;
                    }
                    
                    chksum = 0;

                    //qDebug() << "ch10_dtype: " << ch10_dtype;
                    //qDebug() << "ch10_dat_len: " << ch10_dat_len << " test: " << (pak_size - 28);
                    
                    if ((ch10_dtype == 0x09) && (ch10_dat_len == (pak_size - 28))) {
                        /* PCM data */
                        ch10_ch_spec = *((uint32_t *)(pak_buf + CH10_HD_OFST + 24));
                        //chmode = ((chspec & 0x001e0000) >> 17)
                        ch10_pcm_mode = ((ch10_ch_spec & 0x001e0000) >> 17);
                        //qDebug() << "ch10_pcm_mode: " << ch10_pcm_mode;
                        if (ch10_pcm_mode == CH10_PCM_MODE) {
                            // parse data!
                            //qDebug() << "fake data parse!";
                            //data_pt = (uint32_t *)(pak_buf + CH10_HD_OFST + 28);
                        }
                    }

                    
                } else {
                    qDebug() << "UDP FRAG PACKET?!";
                    qDebug() << "Packet Size: " <<  pak_size;
                    qDebug() << "UDP_TYPE: " << udp_type;
                }



            } else {
                /* Small packet?! */
                qDebug() << "Packet too small to parse";
            }



        }



        condition.wait(&sleepM, 0);
    }
}

Q_EXPORT_PLUGIN2(chap10plugin, Chap10Plugin);
