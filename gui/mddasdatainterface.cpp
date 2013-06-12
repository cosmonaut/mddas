#include <QVector>
#include <QQueue>
#include "mddasdatainterface.h"

MDDASDataInterface::MDDASDataInterface() {
    _queue = new QQueue<QVector<MDDASDataPoint> >();
    _totalCounts = 0;
}

int MDDASDataInterface::bufCount() {
    mutex.lock();
    int c = _queue->count();
    mutex.unlock();
    return c;
}

bool MDDASDataInterface::bufIsEmpty() {
    mutex.lock();
    bool b = _queue->isEmpty();
    mutex.unlock();
    return b;
}

QVector<MDDASDataPoint> MDDASDataInterface::bufDequeue() {
    mutex.lock();
    QVector<MDDASDataPoint> v = _queue->dequeue();
    mutex.unlock();
    return v;
}

void MDDASDataInterface::bufEnqueue(const QVector<MDDASDataPoint> &mdp) {
    mutex.lock();
    _queue->enqueue(mdp);
    mutex.unlock();
}

int MDDASDataInterface::totalCounts() {
    int t = 0;

    //qDebug() << "mddas calling total counts";
    mutex.lock();
    t = _totalCounts;
    mutex.unlock();
    //qDebug() << "mddas called total counts";
    return t;
}

int MDDASDataInterface::addCounts(int c) {
    int t = 0;

    mutex.lock();
    _totalCounts += c;
    t = _totalCounts;
    mutex.unlock();

    return t;
}
