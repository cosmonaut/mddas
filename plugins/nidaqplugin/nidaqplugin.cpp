/* Author: Nicholas Nell
   email: nicholas.nell@colorado.edu

   Sampling thread for the NI PCI-DIO-32HS and PCI-6601 that act as
   NSROC telemtery.  
*/

#include <QDebug>

#include "nidaqplugin.h"


NIDAQPlugin::NIDAQPlugin() : SamplingThreadPlugin() {
    qDebug() << "NIDAQ";

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
NIDAQPlugin::~NIDAQPlugin() {
}


void NIDAQPlugin::run() {
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
        
        qDebug() << "nidaq!";

        condition.wait(&sleepM, 1);
            

        //qDebug() << "blip" << QThread::currentThread();
        //sleep();
        /* Use a wait condition instead of sleep() so that the thread
           can be awoken. */
        //condition.wait(&sleepM, 100);

    }
}

Q_EXPORT_PLUGIN2(nidaqplugin, NIDAQPlugin);

