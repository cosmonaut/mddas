#ifndef SAMPLINGTHREADINTERFACE_H
#define SAMPLINGTHREADINTERFACE_H

#include <QtPlugin>

class MDDASDataPoint;
class MDDASPlotConfig;

class SamplingThreadInterface {
public:
    //virtual SamplingThreadInterface() {}
    virtual ~SamplingThreadInterface() {}
    /* Start thread */
    virtual void sample() = 0;
    /* pause/unpause thread */
    virtual void pause(bool) = 0;
    virtual int bufCount() = 0;
    virtual bool bufIsEmpty() = 0;
    virtual QVector<MDDASDataPoint> bufDequeue() = 0;
    virtual void bufEnqueue(const QVector<MDDASDataPoint> &) = 0;
    virtual int totalCounts() = 0;
    virtual MDDASPlotConfig* getPlotConfig() = 0;

protected:
    virtual void run() = 0;
};

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(SamplingThreadInterface,
                    "SamplingThreadInterface/1.0");
QT_END_NAMESPACE

#endif

