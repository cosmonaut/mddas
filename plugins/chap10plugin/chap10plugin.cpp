/* Author: Nicholas Nell
   email: nicholas.nell@colorado.edu

   Sampling thread for Chapter 10 Ethernet data
*/

#include <QDebug>
#include <stdint.h>

#include "socket.h"
#include "chap10plugin.h"

//#define CH10_PORT 27500
#define CH10_PORT 5555
/* Why? because quakeworld was awesome. */
#define CH10_PAK_SIZE 2048
//#define CH10_PAK_SIZE 1400
#define CH10_HD_OFST 4

#define CH10_UDP_HDR_SIZE 4
#define CH10_HDR_SIZE 24
/* Mode we've been using so far... */
//#define CH10_PCM_MODE 0x04 // Used this mode for CHESS 1 with Kyung
/* Mode being output for CHESS 2 */
#define CH10_PCM_MODE 0x04
/* Known Matrix size */
#define CH10_DATA_LEN 216

#define FRAME_SYNC 0xfe6b2840
#define MAJOR_MINOR_IND 24

#define X_CODE 0x2000
#define Y_CODE 0x4000
#define P_CODE 0x6000

Chap10Plugin::Chap10Plugin() : SamplingThreadPlugin() {
    qDebug() << "Loading Chapter 10 plugin...";
    
    bad_words = 0;
    udp_mismatch = 0;

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

    data_buf_ind = 0;
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
    //int ret = 0;
    /* Packet buffer */
    uint8_t pak_buf[CH10_PAK_SIZE];
    //uint16_t data_buf
    uint32_t *data_pt = 0;
    //uint16_t pak_size = 0;
    int32_t pak_size = 0;
    //uint8_t p_status = 1;

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

    uint32_t matrix_word = 0;
    uint16_t matrix_word_16 = 0;
    uint32_t fs = 0;

    uint16_t maj_f = 0;
    uint16_t min_f = 0;
    //uint8_t minf_of = 0;

    /* Switch for first packet check */
    uint8_t first_pak = 0;

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

    
    data_buf_ind = 0;


    /* Data loop. */
    forever {
        mutex.lock();
        if (pauseSampling) {
            qDebug() << "Parse Errors: " << bad_words;
            qDebug() << "UDP Missed: " << udp_mismatch;
            condition.wait(&mutex);
            /* Refresh data buffer index */
            data_buf_ind = 0;

            /* Refresh error counters */
            bad_words = 0;
            udp_mismatch = 0;
        }
        mutex.unlock();

        if (abort) {
            qDebug() << "called abort!" << QThread::currentThread();
            return;
        }

        
        while((pak_size = _sock->recvFrom(pak_buf, CH10_PAK_SIZE, srcAddr, srcPort)) > 0) {
            if (pak_size > 4) {
                udp_header = *((uint32_t *)(pak_buf + 0));

                // udpseq = (udpheader & 0xffffff00) >> 8
                // udpver = udpheader & 0x0000000f
                // udptype = udpheader & 0x000000f0
                
                // print("seq: %i" % udpseq)
                // print("ver: %i" % udpver)
                // print("type: %i" % udptype)

                //qDebug() << "udp";

                udp_seq = ((udp_header & 0xffffff00) >> 8);
                udp_type = ((udp_header & 0x000000f0) >> 4);
                //qDebug() << "udp seq: " << udp_seq << " udp_type: " << udp_type;
                if (udp_seq != (udp_seq_last + 1)) {
                    if (first_pak) {
                        //qDebug() << "UDP Seq mismatch, seq: " << udp_seq << " last: " << udp_seq_last;
                        udp_mismatch++;
                        //udp_missed = udp_seq - udp_seq_last
                    }
                }
                udp_seq_last = udp_seq;
                
                if (first_pak = 0) {
                    first_pak = 1;
                }

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
                        if ((ch10_pcm_mode == CH10_PCM_MODE) && (ch10_dat_len == CH10_DATA_LEN)) {
                            //qDebug() << "PARSING";
                            // parse data!
                            data_pt = (uint32_t *)(&pak_buf[0] + CH10_HD_OFST + 28);
                            
                            /* Check Frame Sync */
                            fs = data_pt[3];
                            if (fs != FRAME_SYNC) {
                                QString fsAsHex = QString("%1").arg(fs, 0, 16);
                                qDebug() << "BAD FRAME SYNC: " << fsAsHex;
                            }

                            /* Parse matrix */
                            for (int i = 0; i < 49; i++) {
                                //matrix_word = *(data_pt + i*4);
                                matrix_word = data_pt[4 + i];
                                
                                if (i != 24) {
                                    matrix_word_16 = ((matrix_word & 0xffff0000) >> 16);
                                    // UPDATE: Need to check for 0x0000 OR 0xEAAA here! (DIDIT)
                                    //if (matrix_word_16 != 0) {
                                    if ((matrix_word_16 != 0) && (matrix_word_16 != 0xEAAA)) {
                                        if (data_buf_ind < 4096) {
                                            data_buf[data_buf_ind] = matrix_word_16;
                                            data_buf_ind++;
                                        } else {
                                            qDebug() << "ERROR: data_buf overflow!";
                                        }
                                        //qDebug() << matrix_word_16;
                                    }
                                    matrix_word_16 = (matrix_word & 0x0000ffff);
                                    //if (matrix_word_16 != 0) {
                                    if ((matrix_word_16 != 0) && (matrix_word_16 != 0xEAAA)) {
                                        if (data_buf_ind < 4096) {
                                            data_buf[data_buf_ind] = matrix_word_16;
                                            data_buf_ind++;
                                        } else {
                                            qDebug() << "ERROR: data_buf overflow!";
                                        }
                                        //qDebug() << matrix_word_16;
                                    }

                                } else {
                                    matrix_word_16 = ((matrix_word & 0xffff0000) >> 16);
                                    if (matrix_word_16 != (min_f + 1)) {
                                        //qDebug() << "Skipped minor frame";
                                        qDebug() << "current minor: " << matrix_word_16 << " last minor: " << min_f;
                                    }
                                    min_f = matrix_word_16;
                                    
                                    matrix_word_16 = (matrix_word & 0x0000ffff);
                                    maj_f = matrix_word_16;
                                    // Don't really care about major
                                    // frame... if we skipped one of
                                    // these, yikes.
                                }

                            }

                            /* Parse data buffer */
                            v = parse_data();
                            /* pass data to gui */
                            if (v.size() > 0) {
                                _di.bufEnqueue(v);
                                v.clear();
                            }
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

/* Parse list of words from matrix for x, y, p detector data. */
QVector<MDDASDataPoint> Chap10Plugin::parse_data(void) {
    QVector<MDDASDataPoint> v;
    uint32_t i = 0;
    /* word search state! x: 0, y: 1, p: 2 */
    uint8_t state = 0;

    uint16_t x = 0;
    uint16_t y = 0;
    uint16_t p = 0;

    uint16_t badness = 0;

    uint32_t parsed_ind = 0;

    // if (data_buf_ind != 96) {
    //     qDebug() << "data buf ind... " << data_buf_ind;
    // }

    if (data_buf_ind > 2) {
        for (i = 0; i < data_buf_ind; i++) {
            if (state == 0) {
                if ((data_buf[i] & 0xe000) == X_CODE) {
                    x = (data_buf[i] & 0x1fff);
                    state = 1;
                } else {
                    // bad code
                    badness++;
                    state = 0;
                    parsed_ind = i;
                }
            } else if (state == 1) {
                if ((data_buf[i] & 0xe000) == Y_CODE) {
                    y = (data_buf[i] & 0x1fff);
                    state = 2;
                } else {
                    // bad code
                    badness++;
                    state = 0;
                    parsed_ind = i;
                }

            } else if (state == 2) {
                if ((data_buf[i] & 0xe000) == P_CODE) {
                    p = (data_buf[i] & 0x1fff);
                    state = 0;
                    v.append(MDDASDataPoint(x, y, p));
                    parsed_ind = i;
                } else {
                    // bad code
                    badness++;
                    state = 0;
                    parsed_ind = i;
                }

            } 

        }
    }

    /* Shift data buf */
    if (parsed_ind > 0) {
        /* Should memmove copy 2*index because we are working with 16 bit words?! */
        /* Also, data_buf+parsed_ind+1 == data_buf[parsed_ind + 1] ??  */
        memmove(data_buf, (data_buf + parsed_ind + 1), 2*(data_buf_ind - (parsed_ind + 1)));
        data_buf_ind = (data_buf_ind - (parsed_ind + 1));
    }

    //data_buf_ind = (data_buf_ind - (parsed_ind + 1));

    if (data_buf_ind > 4095) {
        qDebug() << "ERROR: data_buf_ind: " << data_buf_ind;
        qDebug() << "Parsed ind: " << parsed_ind;
    }

    if (badness) {
        //qDebug() << "Bad words: " << badness;
        bad_words += badness;
    }

    return v;
}

Q_EXPORT_PLUGIN2(chap10plugin, Chap10Plugin);
