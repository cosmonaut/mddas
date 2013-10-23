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

    try {
        _sock  = new TCPServerSocket(DEWESOFT_PORT);
    } catch (SocketException &e) {
        qDebug() << e.what();
    }

}

DewePlugin::~DewePlugin() {
    qDebug() << "Closing dewesoft plugin...";
}

void DewePlugin::run() {
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

        condition.wait(&sleepM, 1);
    }
}

Q_EXPORT_PLUGIN2(deweplugin, DewePlugin);
