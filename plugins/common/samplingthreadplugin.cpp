//#include <QtGui>
#include <QtWidgets>
#include <QDebug>
#include <QMutex>
#include <QWaitCondition>

#include "samplingthreadplugin.h"

SamplingThreadPlugin::SamplingThreadPlugin() {
    QTime time = QTime::currentTime();
    //qsrand((uint)time.msec());
    unsigned int iseed;
    iseed = time.msec();
    srand(iseed);

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
SamplingThreadPlugin::~SamplingThreadPlugin() {
    mutex.lock();

    abort = true;
    condition.wakeOne();
    mutex.unlock();

    wait();
}

void SamplingThreadPlugin::run() {
    int i = 0;
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

        _di.addCounts(1);
        //for (i = 0; i < 50000; i++) {
        for (i = 0; i < 3000; i++) {
            //v.append(MDDASDataPoint(0,0,10));
            v.append(MDDASDataPoint(rand() % _pc->getXMax(),rand() % _pc->getYMax(),10));
        }
        _di.bufEnqueue(v);
        v.clear();
            
        /* Use a wait condition instead of sleep() so that the thread
           can be awoken. */
        condition.wait(&sleepM, 1);

    }
}

/* Start the sampling */
void SamplingThreadPlugin::sample() {
    QMutexLocker locker(&mutex);

    if (!isRunning()) {
        /* Start thread paused */
        pauseSampling = true;
        start(HighPriority);
    }
}

void SamplingThreadPlugin::pause(bool p) {
    mutex.lock();
    if (!p) {
        condition.wakeOne();
    }
    pauseSampling = p;
    mutex.unlock();
}

int SamplingThreadPlugin::bufCount() {
    return _di.bufCount();
}

bool SamplingThreadPlugin::bufIsEmpty() {
    return _di.bufIsEmpty();
}

QVector<MDDASDataPoint> SamplingThreadPlugin::bufDequeue() {
    return _di.bufDequeue();
}

void SamplingThreadPlugin::bufEnqueue(const QVector<MDDASDataPoint> &mdp) {
    _di.bufEnqueue(mdp);
}

int SamplingThreadPlugin::totalCounts() {
    //qDebug() << "calling totalCounts!" << QThread::currentThread();
    return _di.totalCounts();
    //qDebug() << "called totalCounts!" << QThread::currentThread();
}

MDDASPlotConfig* SamplingThreadPlugin::getPlotConfig() {
    return _pc;
}

//Q_EXPORT_PLUGIN2(samplingthreadplugin, SamplingThreadPlugin);

