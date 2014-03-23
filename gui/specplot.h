#ifndef _SPECPLOT_H_
#define _SPECPLOT_H_

#include <QHash>
#include <qwt/qwt_plot.h>

class QwtPlotHistogram;
class MDDASDataPoint;
class MDDASPlotConfig;

class SpecPlot : public QwtPlot
{
    Q_OBJECT

public:
    SpecPlot(QWidget *parent = NULL, uint x_max = 0, uint y_max = 0, uint p_max = 0);
    virtual ~SpecPlot();
    virtual QSize sizeHint() const;
    void appendPoints( const QVector<MDDASDataPoint> &);
    void clear();
    void configure(MDDASPlotConfig);
    void setBox(uint, uint, uint, uint);
    void setRebinFactor(uint);

signals:
    void boxSizeChanged(uint, uint, uint, uint);

private:
    QwtPlotHistogram *d_curve;
    QHash<uint, uint> *_hash;

    uint _x_min;
    uint _x_max;
    uint _y_min;
    uint _y_max;
    uint _x_interval;
    uint _y_interval;
    uint _rebin_f;
};

#endif // _SPECPLOT_H_
