/*  Author: Jarrod Puseman
    email: jarrod.puseman@colorado.edu

    Author: Nicholas Nell
    email: nicholas.nell@colorado.edu

    Sampling thread for the digiatal 16-channel data aquisition card. 
*/

#include <QDebug>
#include <string.h>
#include <ctime>
#include <unistd.h>

#include "nicodaq.h"
#include "socket.h"


#define DETX 16384
#define DETY 8192
#define DETP 256


// Note we should be 192.168.1.110 for use with diodaq
#define DIODAQ_ADDR "192.168.1.100"
#define DIODAQ_PORT 60100
#define CONFIG_PORT 60101


// # of seconds to wait before attempting a reconfig if needed
#define DIODAQ_CONFIG_REPEAT 10.0

// Configuration packet size
#define NICODAQ_CFG_SIZE 256

/* Data packet size in bytes */
#define DIODAQ_PACKET_SIZE 1470 
/* Each unit in the package is really a 16-bit uint */
#define DIODAQ_PACKET_LEN DIODAQ_PACKET_SIZE/2

/* Code number of each word */
#define X_CODE 0x2000 //0b 0010 0000 0000 0000
#define Y_CODE 0x4000 //0b 0100 0000 0000 0000
#define P_CODE 0x6000 //0b 0110 0000 0000 0000

// TMIF photon word encoding...
#define NULL_WORD 0x0aaa
#define L_MASK 0x0000
#define R_MASK 0x8000

#define LX (X_CODE | L_MASK)
#define LY (Y_CODE | L_MASK)
#define LP (P_CODE | L_MASK)

#define RX (X_CODE | R_MASK)
#define RY (Y_CODE | R_MASK)
#define RP (P_CODE | R_MASK)

#define W_MASK 0x6000
#define RLW_MASK 0xe000
#define DATA_MASK 0x1fff

// Photon parsing state machinge information
#define IDLE 0
#define LEFTY 1
#define LEFTP 2
#define RIGHTY 3
#define RIGHTP 4

// DIODAQ config responses
#define ACK 0xFA
#define NACK 0x01

DIODAQPlugin::DIODAQPlugin() : SamplingThreadPlugin() {

    qDebug() << "Creating socket!";

    _p_count = 0;
    bad_words = 0;

    g_state = IDLE;
    g_x = 0;
    g_y = 0;
    g_p = 0;
    
    this->_packet_buf = new uint8_t[DIODAQ_PACKET_SIZE];
    memset(_packet_buf, 0, DIODAQ_PACKET_SIZE);

    // configuration packet buffer
    this->_c_pbuf = new uint8_t[NICODAQ_CFG_SIZE];
    memset(_c_pbuf, 0, NICODAQ_CFG_SIZE);

    this->_c_pbuf[0] = NACK;
    this->_c_pbuf[1] = 0x00;
    this->_c_pbuf[2] = 0x00;
    this->_c_pbuf[3] = 0x00;
    
    _s = new Socket();
    if (!_s->create()) {
        qDebug() << "could not create socket";
    }

    if (!_s->bind(DIODAQ_PORT)) {
        qDebug() << "Could not bind";
    }

    _s->set_non_blocking(true);


    abort = false;
    pauseSampling = false;
    _pc = new MDDASPlotConfig();
    _pc->setXMax(DETX);
    _pc->setXMin(0);
    _pc->setYMax(DETY);
    _pc->setYMin(0);
    _pc->setPMax(DETP);
    _pc->setPMin(0);
}

/* close thread */
DIODAQPlugin::~DIODAQPlugin() {
    //qDebug() << "" << QThread::currentThread();

    //Free the configuration and packet buffers
    if (this->_packet_buf != 0) {
        delete[] this->_packet_buf;
    }

    if (this->_c_pbuf != NULL) {
        delete[] this->_c_pbuf;
    }

}


void DIODAQPlugin::run() {
    // int i = 0;
    int j = 0;
    uint16_t packet_number;

    QMutex sleepM;
    QVector<MDDASDataPoint> v;
    sleepM.lock();

    // Used to calculate time passed for reconfig attemps
    time_t t1;

    // Initialize t1
    time(&t1);


    // Loop indefinitely (until stopped from gui)
    forever {
        mutex.lock();
        if (pauseSampling) {
            // Stop Strobe
            config(0x00);
            
            qDebug() << "Parse Errors: " << bad_words;
            condition.wait(&mutex);
            //Refresh Error Counter
            bad_words = 0;

            // refresh parse state
            g_state = IDLE;
            g_x = 0;
            g_y = 0;
            g_p = 0;

            if (!abort) {
                // start strobe again?
                config(0x01);
            }
        }
        mutex.unlock();

        if (abort) {
            qDebug() << "called abort!" << QThread::currentThread();
            return;
        }

        // Receive and parse packets from DAQ
        while(_s->recvFrom(_packet_buf, DIODAQ_PACKET_SIZE) == DIODAQ_PACKET_SIZE) {

            // TODO: Clean up packet number counter
            v = parse_data();
            packet_number = ntohs((_packet_buf[0]<<8) + _packet_buf[1]); //Packet number is the first 16-bit word
            j = packet_number - _p_count;
            if (j != 0) {
                qDebug() << "missed some packet(s): " << j;
                _p_count += j;
            }
            _p_count++;
            _di.bufEnqueue(v);
            v.clear();
        } 


        //qDebug() << "blip" << QThread::currentThread();
        /* Use a wait condition instead of sleep() so that the thread
           can be awoken. */
        condition.wait(&sleepM, 1);
    }
}

QVector<MDDASDataPoint> DIODAQPlugin::parse_data(){
    QVector<MDDASDataPoint> v;
    uint32_t i = 0;
    /* word search state */
    uint8_t state = 0;
    uint16_t entry;
    uint16_t x = 0;
    uint16_t y = 0;
    uint16_t p = 0;
    uint16_t badness = 0;
    //uint32_t parsed_ind = 0;

    uint16_t *pbuf;

    // 16-bit wide pointer to packet buffer
    pbuf = (uint16_t *)_packet_buf;


    // Setup state variable
    if (g_state != IDLE) {
        state = g_state;
        x = g_x;
        y = g_y;
        p = g_p;
    } else {
        // state var -- IDLE, LEFT, RIGHT
        state = IDLE;
    }

    for (i = 3; i < DIODAQ_PACKET_LEN; i++) {
        entry = pbuf[i];

        if (entry == NULL_WORD) {
            // skip
            continue;
        }

        //qDebug() << "a word... " << entry << endl;

        switch (state) {
        case IDLE:
            // look for xr or xl
            if ((entry & W_MASK) == X_CODE) {
                x = (entry & DATA_MASK);
                if (entry & R_MASK) {
                    //x += 8192;
                    state = RIGHTY;
                } else {
                    state = LEFTY;
                }
            } else {
                badness++;
                continue;
            }
            break;
        case LEFTY:
            if ((entry & RLW_MASK) == LY) {
                y = entry & DATA_MASK;
                //qDebug() << "Y: " << entry;
                state = LEFTP;
            } else {
                // badness
                badness++;
                state = IDLE;
            }
            break;
        case LEFTP:
            if ((entry & RLW_MASK) == LP) {
                p = entry & DATA_MASK;
                v.append(MDDASDataPoint(x, y, p)); //complete photon event
                state = IDLE;
            } else {
                // badness
                badness++;
                state = IDLE;
            }
            break;
        case RIGHTY:
            if ((entry & RLW_MASK) == RY) {
                y = entry & DATA_MASK;
                state = RIGHTP;
            } else {
                badness++;
                state = IDLE;
            }
            break;
        case RIGHTP:
            if ((entry & RLW_MASK) == RP) {
                p = entry & DATA_MASK;
                v.append(MDDASDataPoint(x, y, p)); //complete photon event
                state = IDLE;
            } else {
                badness++;
                state = IDLE;
            }
            break;
        default:
            // how even?!
            state = IDLE;
            break;
        }
    }

    // preserve state across calls
    g_state = state;
    g_x = x;
    g_y = y;
    g_p = p;
    

    if (badness) {
        //qDebug() << "Bad words: " << badness;
        bad_words += badness;
    }
    return v;
}

// Configure the DAQ module
void DIODAQPlugin::config(uint8_t timer) {
    bool status = false;

    qDebug() << "CONFIG\n";
    
    this->_c_pbuf[0] = NACK;
    if (timer != 0x00) {
        this->_c_pbuf[1] = 0x01;
    } else {
        this->_c_pbuf[1] = 0x00;
    }

    std::string p(((const char *)this->_c_pbuf), 256);
    status = _s->send_to(p, DIODAQ_ADDR, CONFIG_PORT);

    if (status == false) {
        qDebug() << "CONFIG PACKET FAILED\n";
    }
}


