#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_grid.h>
#include <qwt/qwt_plot_canvas.h>
#include <qwt/qwt_plot_histogram.h>
#include <qwt/qwt_plot_layout.h>

#include "incrementalhistplot.h"
#include "mddasdatapoint.h"
#include "mddasplotconfig.h"

class HistoCurveData: public QwtArraySeriesData<QwtIntervalSample>
{
public:
    HistoCurveData() {
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


IncrementalHistPlot::IncrementalHistPlot(QWidget *parent, uint x_max, uint y_max, uint p_max): 
    QwtPlot(parent),
    d_curve(NULL)
{
    setFrameStyle(QFrame::NoFrame);
    setLineWidth(0);

    _p_max = p_max;

    /* The hash allows a fast mapping for rebinning the
       histogram */
    _hash = new QHash<uint, uint>();
    _hash->clear();
    for (uint32_t i = 0; i < _p_max; i++) {
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

    setAxisScale(xBottom, 0, _p_max);
    setAxisAutoScale(yLeft); // Autoscale hist data

    d_curve = new QwtPlotHistogram("Spectrum");
    d_curve->setData( new HistoCurveData() );
    d_curve->attach(this);

    d_curve->setStyle( QwtPlotHistogram::Outline );

    QPen pen( Qt::white );    
    d_curve->setPen( pen );
    /* Use this to fill curve */
    //d_curve->setBrush(QBrush(Qt::white));

    setAutoReplot(false);

    replot();
}

IncrementalHistPlot::~IncrementalHistPlot() {
    delete d_curve;
}

void IncrementalHistPlot::configure(MDDASPlotConfig pc) {
    _p_max = pc.getPMax();
    clear();
    setNumBins(_p_max);

    setAxisScale(xBottom, 0, _p_max);
    replot();
}

void IncrementalHistPlot::appendPoints(const QVector<MDDASDataPoint> &v) {
    HistoCurveData *data = static_cast<HistoCurveData *>( d_curve->data() );

    QVector<QwtIntervalSample> samples = data->samples();
    QVector<double> newvals(samples.size());

    for (int i = 0; i < v.size(); i++) {
        newvals[_hash->value(v[i].p())] += 1;
    }

    data->appendValues(newvals);
}

void IncrementalHistPlot::clear() {
    HistoCurveData *data = static_cast<HistoCurveData *>( d_curve->data() );
    data->clear();
 
    setAxisScale(QwtPlot::yLeft, 0, 1);
    setAxisAutoScale(yLeft); // Autoscale hist data
   
    replot();
}

void IncrementalHistPlot::setNumBins(int n_bins) {
    double interval_size = 0.0;
    double interval_place = 0.0;
    uint interval_count = 0;

    if (n_bins < 0) {
        n_bins = 0;
    }

    if ((uint64_t)n_bins > _p_max) {
        n_bins = _p_max;
    }

    n_bins = getClosestFactor(n_bins);
    //qDebug() << "new n_bins: " << n_bins;
    interval_size = _p_max/n_bins;

    HistoCurveData *data = static_cast<HistoCurveData *>( d_curve->data() );
    QVector<QwtIntervalSample> sample_vec(n_bins);

    /* Clear the plot */
    clear();

    /* Build a new set of intervals based on new number of bins */
    for (int i = 0; i < n_bins; i++) {
        //sample_vec[i] = QwtIntervalSample(0.0, QwtInterval(i, i+1, QwtInterval::ExcludeMaximum));
        sample_vec[i] = QwtIntervalSample(0.0, QwtInterval(interval_place, 
                                                           interval_place + interval_size, 
                                                           QwtInterval::ExcludeMaximum));
        interval_place += interval_size;
    }

    /* Rebuild the hash with new number of bins */
    _hash->clear();
    _hash->squeeze();

    interval_place = 0;
    for (uint i = 0; i < _p_max; i++) {
        if (i >= (interval_place + interval_size)) {
            interval_place += interval_size;
            interval_count++;
        }
        //qDebug() << "bin: " << i << " interval #: " << interval_count;
        _hash->insert(i, interval_count);
    }

    /* Set data */
    data->setSamples(sample_vec);
    replot();

    /* signals for histbox */
    emit numBinsChanged(n_bins);
    emit binSizeChanged((int)interval_size);
}

void IncrementalHistPlot::setBinSize(int bin_size) {
    int n_bins = 0;
    n_bins = _p_max/bin_size;
    setNumBins(n_bins);
}

/* Helper for calculating number of bins */
inline bool lessThanAbs(const short a, const short b) {
    return abs(a) <= abs(b);
}

uint IncrementalHistPlot::getClosestFactor(uint n_bins) {
    uint new_n_bins = 0;
    unsigned short i;
    //unsigned short N = (unsigned short)axisInterval(xBottom).width();
    uint N = _p_max;
    QVector<uint> diffs;

    //qDebug() << "N: " << N;

    /* Find numbers that divide histogram axis */
    for (i = 1; i < N + 1; i++) {
        if (N%i == 0) {
            diffs.append(i - n_bins);
        }
    }

    /* Find the closest factor! */
    qSort(diffs.begin(), diffs.end(), lessThanAbs);
    new_n_bins = n_bins + (uint)diffs.front();
    return(new_n_bins);
}

QSize IncrementalHistPlot::sizeHint() const {
    return QSize(500, 500);
}
