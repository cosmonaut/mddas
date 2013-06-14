#ifndef _INCREMENTALHISTPLOT_H_
#define _INCREMENTALHISTPLOT_H_

#include <QHash>
#include <qwt/qwt_plot.h>

class QwtPlotHistogram;
class MDDASDataPoint;
class MDDASPlotConfig;
class Zoomer;

class IncrementalHistPlot : public QwtPlot
{
    Q_OBJECT

public:
    IncrementalHistPlot(QWidget *parent = NULL, uint x_max = 0, uint y_max = 0, uint p_max = 0);
    virtual ~IncrementalHistPlot();
    virtual QSize sizeHint() const;
    void appendPoints( const QVector<MDDASDataPoint> &);
    void clear();
    void configure(MDDASPlotConfig);
    void setBinSize(int);
    void setNumBins(int);

signals:
    void numBinsChanged(int);
    void binSizeChanged(int);

private:
    uint getClosestFactor(uint n_bins);
    QwtPlotHistogram *d_curve;
    QHash<uint, uint> *_hash;
    uint _p_max;
};

#endif // _INCREMENTALHISTPLOT_H_
