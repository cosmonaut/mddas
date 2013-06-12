#ifndef SAMPLINGTHREADBASE_H
#define SAMPLINGTHREADBASE_H

#include <QThread>
#include <QVector>

#include "samplingthreadinterface.h"
#include "mddasdatainterface.h"
#include "mddasdatapoint.h"
#include "mddasplotconfig.h"

//template <typename T> class QVector;

class SamplingThreadPlugin : public QThread, SamplingThreadInterface {
    Q_OBJECT
    Q_INTERFACES(SamplingThreadInterface)

public:
    SamplingThreadPlugin();
    ~SamplingThreadPlugin();

    void sample();
    void pause(bool);

    /* Data buffer functions */
    int bufCount();
    bool bufIsEmpty();
    QVector<MDDASDataPoint> bufDequeue();
    void bufEnqueue(const QVector<MDDASDataPoint> &);
    int totalCounts();
    MDDASPlotConfig* getPlotConfig();

protected:
    void run();
    QMutex mutex;
    QWaitCondition condition;
    bool abort;
    bool pauseSampling;
    MDDASDataInterface _di;
    MDDASPlotConfig *_pc;

private:
};
    

#endif

