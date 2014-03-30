#ifndef _SPECTROSCATTERPLOT_H_
#define _SPECTROSCATTERPLOT_H_

#include <qwt_plot.h>

class QwtPlot;
class QwtPlotMarker;
class QwtPlotShapeItem;
class QwtPlotSpectrogram;
class QwtLinearColorMap;
class QwtColorMap;
class QPointF;
class MDDASPlotConfig;
class MDDASDataPoint;
class BoxPicker;
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
    void setPicker(uint);

public slots:
    void replotWithScale();
    void setColorMap(int);
    void setColorMapMode(bool);
    void rebin(uint);
    void setBox1(uint, uint, uint, uint);
    void setBox2(uint, uint, uint, uint);
    void setBox3(uint, uint, uint, uint);

signals:
    void divisorsChanged();
    void boxpos(uint, uint, uint);

private slots:
    void on1Selected(const QPointF&);
    void on2Selected(const QPointF&);
    void on3Selected(const QPointF&);

private:
    void refreshBoxes(void);
    QwtPlotSpectrogram *d_curve;
    Zoomer *_z;
    /* colormap scale 0 linear, 1 log */
    uint _cm_mode;
    /* color map index */
    uint _cm_index;
    QVector<uint> _divisors;
    bool _can_rebin;
    bool _binned;
    uint _rebin_f;
    uint _x_max;
    uint _y_max;

    QwtPlotShapeItem *box1;
    QwtPlotShapeItem *box2;
    QwtPlotShapeItem *box3;

    QRectF box1rect;
    QRectF box2rect;
    QRectF box3rect;

    BoxPicker *_box1Picker;
    BoxPicker *_box2Picker;
    BoxPicker *_box3Picker;
};

#endif // _SPECTROSCATTERPLOT_H_
