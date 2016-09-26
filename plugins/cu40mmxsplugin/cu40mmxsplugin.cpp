/* Author: Nicholas Nell
   email: nicholas.nell@colorado.edu

   Sampling thread for the CU40mmXS detector. 
*/

#include <QDebug>
#include <string.h>
#include <ctime>
#include <unistd.h>

#include "cu40mmxsplugin.h"
#include "socket.h"
#include "pak1.h"
#include "pak2.h"
#include "pak3.h"

#define CU40MMXS_ADDR "192.168.1.5"
#define CU40MMXS_PORT 60000
#define CU40MMXS_CONFIG_PORT 60001


// # of seconds to wait before attempting a reconfig if needed
#define CU40MMXS_CONFIG_REPEAT 10.0

/* Packet size in bytes */
#define CU40MMXS_PACKET_SIZE 1470 
/* config HK packet size in bytes */
#define CU40MMXS_HK_SIZE 1152
/* Each unit in the package is really a 16-bit uint */
#define CU40MMXS_PACKET_LEN CU40MMXS_PACKET_SIZE/2
#define CU40MMXS_PACKET_ROWS CU40MMXS_PACKET_LEN/3
#define CU40MMXS_PACKET_COLS 3

/* Column number of each word */
#define COLX 0
#define COLY 1
#define COLP 2


CU40MMXSPlugin::CU40MMXSPlugin() : SamplingThreadPlugin() {
    // QTime time = QTime::currentTime();
    // //qsrand((uint)time.msec());
    // unsigned int iseed;
    // iseed = time.msec();
    // srand(iseed);

    qDebug() << "Creating socket!";

    _p_count = 0;
    
    this->_packet_buf = new uint8_t[CU40MMXS_PACKET_SIZE];
    memset(_packet_buf, 0, CU40MMXS_PACKET_SIZE);

    // Configuration packet reply buffer
    this->_config_buf = new uint8_t[CU40MMXS_HK_SIZE];
    memset(_config_buf, 0, CU40MMXS_HK_SIZE);

    _s = new Socket();
    if (!_s->create()) {
        qDebug() << "could not create socket";
    }

    if (!_s->bind(CU40MMXS_PORT)) {
        qDebug() << "Could not bind";
    }

    _s->set_non_blocking(true);

    _config_s = new Socket();
    if (!_config_s->create()) {
        qDebug() << "could not create config socket";
    }

    if (!_config_s->bind(CU40MMXS_CONFIG_PORT)) {
        qDebug() << "Could not bind config socket";
    }

    _config_s->set_non_blocking(true);

    // Calibration and indices for housekeeping
    _t_ind[0] = 1026;
    _t_ind[1] = 1028;
    _t_ind[2] = 1030;

    _i_ind[0] = 1032;
    _i_ind[1] = 1034;
    _i_ind[2] = 1036;
    _i_ind[3] = 1038;

    _i_cal[0] = 1.611;
    _i_cal[1] = 1.611;
    _i_cal[2] = 1.611;
    _i_cal[3] = 0.856;

    _v_ind[0] = 1040;
    _v_ind[1] = 1042;
    _v_ind[2] = 1044;
    _v_ind[3] = 1046;

    _v_cal[0] = 3.295;
    _v_cal[1] = 2.498;
    _v_cal[2] = 1.813;
    _v_cal[3] = 1.007;


    abort = false;
    pauseSampling = false;
    _pc = new MDDASPlotConfig();
    _pc->setXMax(8192);
    _pc->setXMin(0);
    _pc->setYMax(8192);
    _pc->setYMin(0);
    _pc->setPMax(256);
    _pc->setPMin(0);
}

/* close thread */
CU40MMXSPlugin::~CU40MMXSPlugin() {
    //qDebug() << "" << QThread::currentThread();
    if (this->_packet_buf != 0) {
        delete[] this->_packet_buf;
    }

    if (this->_config_buf != 0) {
        delete[] this->_config_buf;
    }
}


void CU40MMXSPlugin::run() {
    int i = 0;
    int j = 0;
    int num_photons = 0;
    QMutex sleepM;
    QVector<MDDASDataPoint> v;
    sleepM.lock();

    std::string pak1_pak((char *)pak1, PAK1_SIZE);
    std::string pak2_pak((char *)pak2, PAK2_SIZE);
    std::string pak3_pak((char *)pak3, PAK3_SIZE);

    bool status;
    // Tracks whether or not detector has been successfully configured
    bool config_success = false;

    // Used to calculate time passed for reconfig attemps
    time_t t1;
    time_t t2;
    double s_diff = 0.0;

    double temp = 0.0;
    double mvolts = 0.0;
    double mamps = 0.0;

    // Initialize t1
    time(&t1);

    // Try first config outside of main loop, repeat attempts until we
    // read return packet and confirm config
    status = _config_s->send_to(pak1_pak, CU40MMXS_ADDR, CU40MMXS_CONFIG_PORT);
    if (!status) {
        qDebug() << "CONFIG PAK1 FAIL!";
        perror("pak1 error: ");
    }
    usleep(40);
    status = _config_s->send_to(pak2_pak, CU40MMXS_ADDR, CU40MMXS_CONFIG_PORT);
    if (!status) {
        qDebug() << "CONFIG PAK2 FAIL!";
        perror("pak2 error: ");
    }
    usleep(40);
    status = _config_s->send_to(pak3_pak, CU40MMXS_ADDR, CU40MMXS_CONFIG_PORT);
    if (!status) {
        qDebug() << "CONFIG PAK3 FAIL!";
        perror("pak3 error: ");
    }

    usleep(500000);

    // attempt to read config packet
    if ((_config_s->recvFrom(_config_buf, CU40MMXS_HK_SIZE)) == CU40MMXS_HK_SIZE) {
        config_success = true;        

        for (i = 0; i < PAK1_SIZE; i++) {
            if (_config_buf[i] != pak1[i]) {
                config_success = false;
                qDebug() << "MEGAFAIL";
                break;
            }
        }

        if (_config_buf[1024] != 0xff) {
            qDebug() << "index 1024 not 0xff";
        }

        if (_config_buf[1025] != 0xff) {
            qDebug() << "index 1025 not 0xff";
        }

        // print HK stuff
        for (i = 0; i < 3; i++) {
            temp = ((double)(_config_buf[_t_ind[i]] | (_config_buf[_t_ind[i] + 1] << 8)))/128.0;
            qDebug() << "TEMP: " << temp;
        }

        for (i = 0; i < 4; i++) {
            mvolts = (double)( _config_buf[_v_ind[i]] | (_config_buf[_v_ind[i] + 1] << 8 ) )*_v_cal[i];
            qDebug() << "V: " << mvolts << " mV";
        }

        for (i = 0; i < 4; i++) {
            mamps = (double)( _config_buf[_i_ind[i]] | (_config_buf[_i_ind[i] + 1] << 8 ) )*_i_cal[i];
            qDebug() << "I: " << mamps << " mA";
        }

        //qDebug() << "test1: " << _config_buf[1024];
        //qDebug() << "test2: " << _config_buf[1025];
    } else {
        qDebug() << "First config attempt failed...";
        config_success = false;
    }


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

        // Attempt repeat configures if the first one didn't return a good packet
        if (!config_success) {
            time(&t2);
            s_diff = difftime(t2, t1);
            if (s_diff > CU40MMXS_CONFIG_REPEAT) {
                //qDebug() << s_diff;
                // Send configuration packet!
                status = _config_s->send_to(pak1_pak, CU40MMXS_ADDR, CU40MMXS_CONFIG_PORT);
                if (!status) {
                    qDebug() << "CONFIG PAK1 FAIL!";
                    perror("pak1 error: ");
                }
                usleep(20);
                status = _config_s->send_to(pak2_pak, CU40MMXS_ADDR, CU40MMXS_CONFIG_PORT);
                if (!status) {
                    qDebug() << "CONFIG PAK2 FAIL!";
                    perror("pak2 error: ");
                }
                usleep(20);
                status = _config_s->send_to(pak3_pak, CU40MMXS_ADDR, CU40MMXS_CONFIG_PORT);
                if (!status) {
                    qDebug() << "CONFIG PAK3 FAIL!";
                    perror("pak3 error: ");
                }

                time(&t1);
            }

            // Give detector 0.5s to respond
            usleep(500000);

            // attempt to read config packet
            if ((_config_s->recvFrom(_config_buf, CU40MMXS_HK_SIZE)) == CU40MMXS_HK_SIZE) {
                config_success = true;        

                for (i = 0; i < PAK1_SIZE; i++) {
                    if (_config_buf[i] != pak1[i]) {
                        config_success = false;
                        qDebug() << "MEGAFAIL";
                        break;
                    }
                }

                if (_config_buf[1024] != 0xff) {
                    qDebug() << "index 1024 not 0xff";
                }

                if (_config_buf[1025] != 0xff) {
                    qDebug() << "index 1025 not 0xff";
                }

                // print HK stuff
                for (i = 0; i < 3; i++) {
                    temp = ((double)(_config_buf[_t_ind[i]] | (_config_buf[_t_ind[i] + 1] << 8)))/128.0;
                    qDebug() << "TEMP: " << temp;
                }

                for (i = 0; i < 4; i++) {
                    mvolts = (double)( _config_buf[_v_ind[i]] | (_config_buf[_v_ind[i] + 1] << 8 ) )*_v_cal[i];
                    qDebug() << "V: " << mvolts << " mV";
                }

                for (i = 0; i < 4; i++) {
                    mamps = (double)( _config_buf[_i_ind[i]] | (_config_buf[_i_ind[i] + 1] << 8 ) )*_i_cal[i];
                    qDebug() << "I: " << mamps << " mA";
                }

            } else {
                qDebug() << "config attempt failed...";
                config_success = false;
            }


        }

        //_di.addCounts(1);
        //for (i = 0; i < 50000; i++) {
        // for (i = 0; i < 3000; i++) {
        //     //v.append(MDDASDataPoint(0,0,10));
        //     v.append(MDDASDataPoint(rand() % _pc->getXMax(),rand() % _pc->getYMax(),10));
        // }
        // _di.bufEnqueue(v);
        // v.clear();

        //qDebug() << _s->recvFrom(_packet_buf, CU40MMXS_PACKET_SIZE);
        //if (_s->recvFrom(_packet_buf, CU40MMXS_PACKET_SIZE) == CU40MMXS_PACKET_SIZE) {

        while(_s->recvFrom(_packet_buf, CU40MMXS_PACKET_SIZE) == CU40MMXS_PACKET_SIZE) {
            /* Row 0: num_photons, packet_count, 0x0000 */
            num_photons = ((uint16_t *)_packet_buf)[0];
            //for (i = 1; i < CU40MMXS_PACKET_ROWS; i++) {
            /* This is the magic */
            for (i = 1; i < (num_photons + 1); i++) {
                // v.append(MDDASDataPoint(((uint16_t *)_packet_buf)[i*CU40MMXS_PACKET_COLS + COLX],
                //                         ((uint16_t *)_packet_buf)[i*CU40MMXS_PACKET_COLS + COLY],
                //                         ((uint16_t *)_packet_buf)[i*CU40MMXS_PACKET_COLS + COLP]));
                v.append(MDDASDataPoint((0x1fff & (((uint16_t *)_packet_buf)[i*CU40MMXS_PACKET_COLS + COLX]) >> 1),
                                        (0x1fff & (((uint16_t *)_packet_buf)[i*CU40MMXS_PACKET_COLS + COLY]) >> 1),
                                        ((uint16_t *)_packet_buf)[i*CU40MMXS_PACKET_COLS + COLP]));
                
            }

            j = (uint16_t)(((uint16_t *)_packet_buf)[1] - _p_count);
            //if (((uint16_t *)_packet_buf)[1] != _p_count) {
            if (j != 0) {
                qDebug() << "missed some packet(s): " << j;
                _p_count += j;
            }
            _p_count++;

            //_p_count++;
            _di.bufEnqueue(v);
            v.clear();

        } 

        //else {
        condition.wait(&sleepM, 1);
        //qDebug() << "hi";
        //}
            

        //qDebug() << "blip" << QThread::currentThread();
        //sleep();
        /* Use a wait condition instead of sleep() so that the thread
           can be awoken. */
        //condition.wait(&sleepM, 100);

    }
}

Q_EXPORT_PLUGIN2(cu40mmxsplugin, CU40MMXSPlugin);

