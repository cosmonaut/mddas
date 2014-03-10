#ifndef _SPECTROSCATTERPLOT_H_
#define _SPECTROSCATTERPLOT_H_

#include <qwt_plot.h>

class QwtPlot;
class QwtPlotSpectrogram;
class QwtLinearColorMap;
class QwtColorMap;
class MDDASPlotConfig;
class MDDASDataPoint;
class Zoomer;

class SpectroScatterPlot : public QwtPlot
{
    Q_OBJECT

public:
    SpectroScatterPlot(QWidget *parent = NULL, uint x_max = 0, uint y_max = 0);
    virtual ~SpectroScatterPlot();

    virtual QSize sizeHint() const;

    void configure(MDDASPlotConfig);
    void appendPoints( const QVector<MDDASDataPoint> &);
    void clear();
    QVector<uint> getRebinDivisors(void);
public slots:
    void replotWithScale();
    void setColorMap(int);
    void setColorMapMode(bool);
    void rebin(uint);

signals:
    void divisorsChanged();

private slots:

private:
    QwtPlotSpectrogram *d_curve;
    Zoomer *_z;
    /* colormap scale 0 linear, 1 log */
    uint _cm_mode;
    /* color map index */
    uint _cm_index;
    QVector<uint> _divisors;
    bool _can_rebin;
    bool _binned;
    uint _x_max;
    uint _y_max;
};

#endif // _SPECTROSCATTERPLOT_H_
