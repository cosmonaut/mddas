/* Author: Nicholas Nell
   email: nicholas.nell@colorado.edu

   Sampling thread for NSROC's DEWESOFT
*/

#include <QDebug>
#include <stdint.h>

#include "socket.h"
#include "deweplugin.h"

#define DEWESOFT_PORT 27500
/* Why? because quakeworld was awesome. */

DewePlugin::DewePlugin() : SamplingThreadPlugin() {
    qDebug() << "Loading dewesoft plugin...";
    _sock = NULL;

    try {
        _serv_sock = new TCPServerSocket(DEWESOFT_PORT, 1);
        qDebug() << "Listening on port " << DEWESOFT_PORT;
    } catch (SocketException &e) {
        qDebug() << e.what();
        _serv_sock = NULL;
    }
}

DewePlugin::~DewePlugin() {
    /* Stop thread! */
    mutex.lock();

    abort = true;
    condition.wakeOne();
    mutex.unlock();

    wait();

    /* Destroy stuff! */
    qDebug() << "Closing dewesoft plugin...";
    if (_sock) {
        qDebug() << "deleting socket...";
        delete _sock;
    }
    
    if (_serv_sock) {
        qDebug() << "Deleting TCPServerSocket...";
        delete _serv_sock;
    }
    _sock = NULL;
}

void DewePlugin::run() {
    QMutex sleepM;
    QVector<MDDASDataPoint> v;
    sleepM.lock();
    int ret = 0;
    /* Packet buffer */
    uint8_t pak_buf[1400];
    uint8_t p_status = 1;

    /* packet internals */
    int32_t psize = 0;
    int32_t ptype = 0;
    int32_t psamps = 0;
    int64_t ptsamps = 0;
    double pts = 0.0;
    uint64_t pstart = 0;
    uint64_t pend = 0;
    /* pointers for casts... */
    uint64_t* p_start_p;

    qDebug() << "Waiting for client connection...";
    /* Loop waiting for a client to connect */
    forever {
        if (_serv_sock) {
            ret = _serv_sock->select();
            if (ret == 1) {
                try {
                    _sock = _serv_sock->accept();
                    qDebug() << "Client connected";
                    QString addrstr = QString::fromStdString(_sock->getForeignAddress());
                    qDebug() << "Client Address: " << addrstr;
                    qDebug() << "Client Port: " << _sock->getForeignPort();

                } catch (SocketException &e) {
                    qDebug() << "FAILED TO ACCEPT?";
                    qDebug() << e.what();
                }

                break;
            }
        } else {
            /* Quit because we don't have a socket... */
            qDebug() << "DewePlugin closing due to socket error";
            abort = true;
        }

        if (abort) {
            qDebug() << "DewePlugin: ABORT WITHOUT SOCKET!" << QThread::currentThread();
            return;
        }

        /* Check 10 times per second */
        condition.wait(&sleepM, 100);
    }

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

        /* Get data from DeweTron DeweSoft. */
        ret = _sock->select();
        if (ret > 0) {
            ret = _sock->recv(pak_buf, sizeof(pak_buf));
            if (ret < 0) {
                qDebug() << "recv error: " << ret;
            } else if (ret == 0) {
                /* TCP connection closed, abort so the plugin can be
                   reloaded... */
                qDebug() << "CONNECTION CLOSED!";
                abort = true;
            } else {
                // header: 36 bytes
                // body: depends on # of channels
                // footer: 8 bytes
                qDebug() << "read " << ret << " bytes";
                
                /* From Kyung: They are synchronous parallel data with
                   16 channels all in 16bit counts (integer) */
                /* We need to have some minimum size to read basica
                   parameters... */
                if (ret > 43) {

                    /* point to packet start string */
                    p_start_p = ((uint64_t *)(pak_buf + 0));
                    /* deref packet start string... */
                    pstart = *p_start_p;
                    //qDebug() << pstart;

                    if (pstart != (uint64_t)0x0706050403020100) {
                        qDebug() << "Bad packet start string!";
                        p_status = 0;
                    }

                    /* Packet size in bytes */
                    psize = *((int32_t *)(pak_buf + 8));
                    qDebug() << "packet size: " << psize;

                    if ((psize + 16) == ret) {
                        qDebug() << "PACKET SIZE SUCCESS";
                    }

                    /* packet type */
                    ptype = *((int32_t *)(pak_buf + 12));
                    qDebug() << "Packet type: " << ptype;

                    /* samples per channel */
                    psamps = *((int32_t *)(pak_buf + 16));
                    qDebug() << "samples: " << psamps;

                    /* number of samples */
                    ptsamps = *((int64_t *)(pak_buf + 20));
                    qDebug() << "total samps: " << ptsamps;

                    /* timestamp */
                    pts = *((double *)(pak_buf + 28));
                    qDebug() << "timestamp: " << pts;
                }

                if (p_status) {
                    /* deser packet and send to gui */
                    qDebug() << "deser";
                }

                if (ret >= (psize + 16)) {
                    /* Check end of packet? */
                    pend = *((uint64_t *)(pak_buf + psize + 8));
                    if (pend != (uint64_t)0x0001020304050607) {
                        qDebug() << "Bad packet end";
                    }
                } else {
                    qDebug() << "packet not large enough for end string";
                }

                p_status = 1;
            }
        }

        condition.wait(&sleepM, 50);
    }
}

Q_EXPORT_PLUGIN2(deweplugin, DewePlugin);
