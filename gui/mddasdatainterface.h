#ifndef MDDASDATAINTERFACE_H
#define MDDASDATAINTERFACE_H

//#include <QtGui>
#include <QtWidgets>
#include "mddasdatapoint.h"

QT_BEGIN_NAMESPACE
template <typename T> class QQueue;
template <typename T> class QVector;
class QMutex;
QT_END_NAMESPACE



class MDDASDataInterface: public QObject {
    Q_OBJECT
public:
    MDDASDataInterface();

    int bufCount();
    bool bufIsEmpty();
    QVector<MDDASDataPoint> bufDequeue();
    void bufEnqueue(const QVector<MDDASDataPoint> &);

    int totalCounts();
    int addCounts(int);
private:
    /* The datas. */
    QQueue<QVector<MDDASDataPoint> > *_queue;
    int _totalCounts;
    QMutex mutex;

};

    

#endif

