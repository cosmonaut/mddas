/* Author: Nicholas Nell
   email: nicholas.nell@colorado.edu

   Sampling thread for NSROC's DEWESOFT
*/

#include <QDebug>

#include "socket.h"
#include "deweplugin.h"

#define DEWESOFT_PORT 27500
/* Why? because quakeworld was awesome. */

DewePlugin::DewePlugin() : SamplingThreadPlugin() {
    qDebug() << "Loading dewesoft plugin...";
    _sock = NULL;

    try {
        _serv_sock = new TCPServerSocket(DEWESOFT_PORT, 1);
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

    /* Loop waiting for a client to connect */
    forever {
        if (_serv_sock) {
            ret = _serv_sock->select();
            if (ret == 1) {
                try {
                    _sock = _serv_sock->accept();
                    qDebug() << "accept() client connection";
                    QString addrstr = QString::fromStdString(_sock->getForeignAddress());
                    qDebug() << "Address: " << addrstr;
                    qDebug() << "Port: " << _sock->getForeignPort();

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

        condition.wait(&sleepM, 500);
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

        condition.wait(&sleepM, 1);
    }
}

Q_EXPORT_PLUGIN2(deweplugin, DewePlugin);
