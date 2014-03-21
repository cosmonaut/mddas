#ifndef _SPECTROMONITORSCATTERPLOT_H_
#define _SPECTROMONITORSCATTERPLOT_H_

#include <qwt/qwt_plot.h>

class QwtPlotSpectrogram;
class MDDASPlotConfig;
class MDDASDataPoint;
class Zoomer;

class SpectroMonitorScatterPlot : public QwtPlot
{
    Q_OBJECT

public:
    SpectroMonitorScatterPlot(QWidget *parent = NULL, uint x_max = 0, uint y_max = 0);
    virtual ~SpectroMonitorScatterPlot();
    
    virtual QSize sizeHint() const;
    void configure(MDDASPlotConfig);
    //void appendPoints( const QVector<QPointF> &);
    void appendPoints( const QVector<MDDASDataPoint> &);
    QVector<uint> getRebinDivisors(void);
                                                
public slots:
    void clear();
    void rebin(uint);

signals:
    void divisorsChanged();


private:
    QwtPlotSpectrogram *d_curve;
    Zoomer *_z;
    QVector<uint> _divisors;
    bool _can_rebin;
    bool _binned;
    uint _x_max;
    uint _y_max;
};

#endif // _SPECTROMONITORSCATTERPLOT_H_
