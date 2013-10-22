/* Author: Nicholas Nell
   email: nicholas.nell@colorado.edu

   Sampling thread for the CU40mmXS detector. 
*/

#include <QDebug>

#include "cu40mmxsplugin.h"
#include "socket.h"

#define CU40MMXS_PORT "60000"
#define CU40MMXS_ADDR "192.168.1.5"

/* Packet size in bytes */
#define CU40MMXS_PACKET_SIZE 1470 
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

    _s = new Socket();
    if (!_s->create()) {
        qDebug() << "could not create socket";
    }

    if (!_s->bind(60000)) {
        qDebug() << "Could not bind";
    }

    _s->set_non_blocking(true);

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
}


void CU40MMXSPlugin::run() {
    int i = 0;
    int j = 0;
    int num_photons = 0;
    QMutex sleepM;
    QVector<MDDASDataPoint> v;
    sleepM.lock();



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

