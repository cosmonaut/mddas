//#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_grid.h>
#include <qwt/qwt_plot_canvas.h>
#include <qwt/qwt_plot_histogram.h>
#include <qwt/qwt_plot_layout.h>

#include "specplot.h"
#include "mddasdatapoint.h"
#include "mddasplotconfig.h"


static bool ptwo(uint);


/* Used to plot a spectrum */

class SpecCurveData: public QwtArraySeriesData<QwtIntervalSample>
{
public:
    SpecCurveData() {
        d_samples.clear();
        d_samples.squeeze();
    }
    
    virtual QRectF boundingRect() const {
        /* In this case we must constantly update for replot with autoscale. */
        d_boundingRect = qwtBoundingRect( *this );

        return d_boundingRect;
    }

    /* Set a new array of samples */
    inline void setSamples(const QVector<QwtIntervalSample> &new_samples) {
        d_samples.clear();
        d_samples.squeeze();
        d_samples = new_samples;
    }

    inline void appendValues(const QVector<double> &newvals) {
        for (int i = 0; i < d_samples.size(); i++) {
            d_samples[i].value += newvals[i];
        }
    }

    inline void clear() {
        for (int i = 0; i < d_samples.size(); i++) {
            d_samples[i].value = 0.0;
        }
    }
};


SpecPlot::SpecPlot(QWidget *parent, uint x_max, uint y_max, uint p_max): 
    QwtPlot(parent),
    d_curve(NULL)
{
    setFrameStyle(QFrame::NoFrame);
    setLineWidth(0);

    _x_min = 0;
    _x_max = 0;
    _y_min = 0;
    _y_max = 0;

    _x_interval = 0;
    _y_interval = 0;

    _rebin_f = 0;

    /* The hash allows a fast mapping for rebinning the
       histogram */
    _hash = new QHash<uint, uint>();
    _hash->clear();
    for (int i = 0; i < (int)_x_max; i++) {
        _hash->insert(i, i);
    }

    ((QwtPlotCanvas*)canvas())->setLineWidth(0);
    updateLayout();

    plotLayout()->setAlignCanvasToScales(true);

    QwtPlotGrid *grid = new QwtPlotGrid;
    grid->setMajorPen(QPen(Qt::gray, 0, Qt::DotLine));
    grid->attach(this);
    //grid->enableX(false); // Turn off vertical grid lines

    setCanvasBackground(QColor(0, 0, 0));

    QFont f;
    f.setPointSize(8);
    setAxisFont(xBottom, f);
    setAxisFont(yLeft, f);
    
    setAxisScale(xBottom, 0, _x_max);
    setAxisAutoScale(yLeft); // Autoscale hist data

    d_curve = new QwtPlotHistogram("Spectrum");
    d_curve->setData( new SpecCurveData() );
    d_curve->attach(this);

    d_curve->setStyle( QwtPlotHistogram::Outline );

    QPen pen( Qt::white );    
    d_curve->setPen( pen );
    /* Use this to fill curve */
    //d_curve->setBrush(QBrush(Qt::white));

    setAutoReplot(false);

    replot();
}

SpecPlot::~SpecPlot() {
}

void SpecPlot::configure(MDDASPlotConfig pc) {
    clear();

    if (_y_max > (uint)pc.getYMax()) {
        qDebug() << "Y Max too small for SpecPlot";
        _y_max = pc.getYMax();
    }

    if (_x_max > (uint)pc.getXMax()) {
        qDebug() << "X Max too small for SpecPlot";
        _x_max = pc.getXMax();
    }

    replot();
}

void SpecPlot::appendPoints(const QVector<MDDASDataPoint> &v) {
    SpecCurveData *data = static_cast<SpecCurveData *>( d_curve->data() );

    QVector<QwtIntervalSample> samples = data->samples();
    QVector<double> newvals(samples.size());

    for (int i = 0; i < v.size(); i++) {
        if ((v[i].x() < (int)_x_max) && v[i].x() >= (int)_x_min) {
            if ((v[i].y() < (int)_y_max) && v[i].y() >= (int)_y_min) {
                
                newvals[_hash->value(v[i].y())] += 1;

            }
        }
    }

    data->appendValues(newvals);
}

/* Set box on image from which to build spectrum */
void SpecPlot::setBox(uint x_min, uint x_max, uint y_min, uint y_max) {
    uint x_interval;
    uint y_interval;

    if (y_max < y_min) {
        return;
    }

    if (x_max < x_min) {
        return;
    }

    x_interval = x_max - x_min;
    y_interval = y_max - y_min;

    /* The dispersion direction needs to be a power of 2 for easy
       rebin */
    if (ptwo(y_interval)) {
        _x_min = x_min;
        _x_max = x_max;
        _y_min = y_min;
        _y_max = y_max;

        _x_interval = x_interval;
        _y_interval = y_interval;

        setRebinFactor(1);
        /* emit box change */
        emit boxSizeChanged(_x_min, _x_max, _y_min, _y_max);
    }
}

void SpecPlot::setRebinFactor(uint f) {
    int i = 0;
    uint px = 0;
    double interval_place = 0.0;

    if (_y_interval < 1) {
        _rebin_f = 1;
        return;
    }

    if (!ptwo(f)) {
        _rebin_f = 1;
        return;
    }

    if (f >= (_y_max - _y_min)) {
        _rebin_f = 1;
        return;
    }

    _rebin_f = f;

    /* rebuild hash. */
    _hash->clear();
    _hash->squeeze();
    for (i = _y_min; (uint)i < _y_max; i++) {
        _hash->insert((uint)i, px);

        if (((i - _y_min)%_rebin_f) == (_rebin_f - 1)) {
            px++;
        }
    }

    /* Update plot data */
    SpecCurveData *data = static_cast<SpecCurveData *>( d_curve->data() );
    uint n_bins = _y_interval/_rebin_f;
    QVector<QwtIntervalSample> sample_vec(n_bins);
    double interval_size = (double)_rebin_f;
    
    clear();

        /* Build a new set of intervals based on new number of bins */
    for (i = 0; i < (int)n_bins; i++) {
        sample_vec[i] = QwtIntervalSample(0.0, QwtInterval(interval_place, 
                                                           interval_place + interval_size, 
                                                           QwtInterval::ExcludeMaximum));
        interval_place += interval_size;
    }

    /* Set data */
    data->setSamples(sample_vec);
    replot();
}

void SpecPlot::clear() {
    SpecCurveData *data = static_cast<SpecCurveData *>( d_curve->data() );
    data->clear();
 
    setAxisScale(QwtPlot::yLeft, 0, 1);
    setAxisScale(QwtPlot::xBottom, 0, _y_interval);
    setAxisAutoScale(yLeft); // Autoscale hist data
   
    replot();
}

QSize SpecPlot::sizeHint() const {
    return QSize(500, 500);
}

/* check if a number is a power of two... */
static bool ptwo(uint num) {
    if (!(num & (num - 1)) && num) {
        return true;
    } else {
        return false;
    }
}
