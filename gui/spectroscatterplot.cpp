#include <qnumeric.h>
#include <qwt_plot.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_spectrogram.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_rescaler.h>
#include <qwt_color_map.h>
#include <qwt_scale_engine.h>
#include <qwt_scale_widget.h>
#include <qwt_matrix_raster_data.h>
#include <algorithm>
#include <QDebug>
#include <QColor>

#include "spectroscatterplot.h"
#include "colormaps.h"
#include "mddasplotconfig.h"
#include "mddasdatapoint.h"
#include "zoomer.h"


class SpectrogramData: public QwtMatrixRasterData {
public:
    SpectrogramData(uint x_max, uint y_max) {
        _max_v = 0;
        x_lim = x_max;
        y_lim = y_max;

        /* Create the private array */
        /* Note: this is mapped via index magic */
        this->vals = new uint[x_max*y_max];
        /* Init the private array to 0 */
        memset(vals, 0, x_max*y_max*sizeof(unsigned int));

        /* Set the axes intervals */
        setInterval( Qt::XAxis, QwtInterval( 0.0, x_max ) );
        setInterval( Qt::YAxis, QwtInterval( 0.0, y_max ) );
        setInterval( Qt::ZAxis, QwtInterval( 0.0, 1.0 ) );
    }

    ~SpectrogramData() {
        /* IMPORTANT! Delete the vals array */
        delete[] this->vals;
        //this->vals = 0;
    }

    inline void updateScale() {
        /* Update the scale to reflect the largest Z value */
        //setInterval( Qt::ZAxis, QwtInterval( 0.0, (double)*std::max_element(vals, vals + x_lim*y_lim)));
        setInterval( Qt::ZAxis, QwtInterval( 0.1, (double)_max_v));
    }

    inline void append(const QVector<MDDASDataPoint> &vec) {

        /* Append new z vals. */
        for (uint i = 0; i < (uint)vec.size(); i++) {
            if (((uint)vec[i].x() < x_lim) && ((uint)vec[i].y() < y_lim)) {
                vals[y_lim*(uint)vec[i].x() + (uint)vec[i].y()] += 1;
                /* This handles max amplitude without having to loop
                   over the entire matrix */
                if (vals[y_lim*(uint)vec[i].x() + (uint)vec[i].y()] > _max_v) {
                    _max_v += 1;
                }
                
            }
        }
    }

    inline QRectF pixelHint(const QRectF &area) const {
        Q_UNUSED( area )
        
        QRectF rect;

        const QwtInterval intervalX = interval( Qt::XAxis );
        const QwtInterval intervalY = interval( Qt::YAxis );
        if ( intervalX.isValid() && intervalY.isValid() ) {
            rect = QRectF( intervalX.minValue(), intervalY.minValue(),
                           1.0, 1.0 );
        }
        
        return rect;
    }    

    inline void clear() {
        /* Set all values to 0. */
        memset(vals, 0, x_lim*y_lim*sizeof(unsigned int));
        /* Set max amplitude to 0 */
        _max_v = 0;
    }        

    /* Overloaded value function that returns image z vals from map. */
    virtual double value(double x, double y) const {
        const QwtInterval xInterval = interval( Qt::XAxis );
        const QwtInterval yInterval = interval( Qt::YAxis );
        
        /* Use only data within the plot region */
        if ( !( xInterval.contains(x) && yInterval.contains(y) ) )
            return qQNaN();

        if (x < x_lim && y < y_lim) {
            return (double)vals[y_lim*(uint)x + (uint)y];
        } else {
            /* Edge case! */
            return (double)0.0;
        }

        //return (double)vals[y_lim *(uint)x + (uint)y];
    }

private:
    uint *vals; /* matrix of image z values */
    uint x_lim;
    uint y_lim;
    uint _max_v;
};


SpectroScatterPlot::SpectroScatterPlot(QWidget *parent, uint x_max, uint y_max): 
    QwtPlot(parent),
    d_curve(NULL) 
{
    setFrameStyle(QFrame::NoFrame);
    setLineWidth(0);
    //setCanvasLineWidth(0);

    /* Color map mode linear */
    _cm_mode = 0;
    /* Default colormap index 0 */
    _cm_index = 0;

    ((QwtPlotCanvas*)canvas())->setLineWidth(0);
    updateLayout();

    plotLayout()->setCanvasMargin(0);
    plotLayout()->setAlignCanvasToScales(true);

    updateCanvasMargins();

    QwtPlotGrid *grid = new QwtPlotGrid;
    grid->setMajorPen(QPen(Qt::gray, 0, Qt::DotLine));
    grid->attach(this);

    setCanvasBackground(QColor(0, 0, 0)); 

    /* For now we always assume the low limit is 0 */
    setAxisScale(xBottom, 0, x_max);
    setAxisScale(yLeft, 0, y_max);

    d_curve = new QwtPlotSpectrogram();
    d_curve->setData( new SpectrogramData(x_max, y_max) );
    d_curve->setRenderHint(QwtPlotItem::RenderAntialiased, false);
    //d_curve->setRenderThreadCount(2);
    d_curve->setColorMap(new GISTEarthColorMap());
    d_curve->attach(this);

    const QwtInterval zInterval = d_curve->data()->interval( Qt::ZAxis );

    QwtScaleWidget *rightAxis = axisWidget(QwtPlot::yRight);
    rightAxis->setTitle("Counts");
    rightAxis->setColorBarEnabled(true);
    rightAxis->setColorMap( zInterval, new GISTEarthColorMap());

    setAxisScale(QwtPlot::yRight, zInterval.minValue(), zInterval.maxValue() );
    enableAxis(QwtPlot::yRight);

    setAutoReplot(false);

    //(void) new Zoomer(canvas(), x_max);
    _z = new Zoomer((QwtPlotCanvas*)canvas(), x_max);

    setColorMap(0);
    
    replot();
}

SpectroScatterPlot::~SpectroScatterPlot() {
    //delete d_curve;
}

void SpectroScatterPlot::appendPoints(const QVector<MDDASDataPoint> &v) {
    SpectrogramData *data = static_cast<SpectrogramData *>( d_curve->data() );
    data->append(v);
    // data->updateScale();

    // const QwtInterval zInterval = d_curve->data()->interval( Qt::ZAxis );

    // QwtScaleWidget *rightAxis = axisWidget(QwtPlot::yRight);
    // //rightAxis->setTitle("Intensity");
    // rightAxis->setColorBarEnabled(true);
    // rightAxis->setColorMap( zInterval, new ColorMap());

    // setAxisScale(QwtPlot::yRight, zInterval.minValue(), zInterval.maxValue() );
    
    //replot();
}

void SpectroScatterPlot::clear() {
    SpectrogramData *data = static_cast<SpectrogramData *>( d_curve->data() );
    data->clear();
    data->updateScale();

    /* Fake stuff to set the interval back to 0-1 */
    const QwtInterval zInterval = QwtInterval(0.1, 1);

    QwtScaleWidget *rightAxis = axisWidget(QwtPlot::yRight);
    //rightAxis->setColorBarEnabled(true);
    //rightAxis->setColorMap( zInterval, new GISTEarthColorMap());
    if (zInterval.isValid()) {
        //qDebug() << zInterval.minValue() << " " << zInterval.maxValue();
        rightAxis->setColorMap(zInterval, const_cast<QwtColorMap *>(rightAxis->colorMap()));
        //rightAxis->setColorBarInterval(zInterval);
    } else {
        //rightAxis->setColorBarInterval(QwtInterval(0.1, 1.0));
        rightAxis->setColorMap(QwtInterval(0.1, 1.0), const_cast<QwtColorMap *>(rightAxis->colorMap()));
    }

    setAxisScale(QwtPlot::yRight, 0.1, 1);

    replot();
}

QSize SpectroScatterPlot::sizeHint() const {
    return QSize(500,500);
    //return maximumSize();
}

void SpectroScatterPlot::configure(MDDASPlotConfig pc) {
    /* No need to delete old data -- Qwt handles this! */
    //delete d_curve->data();

    d_curve->setData(new SpectrogramData(pc.getXMax(), pc.getYMax()));
    //d_curve->setColorMap(new ColorMap());
    //qDebug() << "specmon made new data";
    setAxisScale(xBottom, 0, pc.getXMax());
    setAxisScale(yLeft, 0, pc.getYMax());

    _z->setZoomBase();

    replot();
}

void SpectroScatterPlot::replotWithScale() {
    SpectrogramData *data = static_cast<SpectrogramData *>( d_curve->data() );
    data->updateScale();

    const QwtInterval zInterval = d_curve->data()->interval( Qt::ZAxis );

    QwtScaleWidget *rightAxis = axisWidget(QwtPlot::yRight);
    //rightAxis->setColorBarEnabled(true);
    //rightAxis->setColorMap( zInterval, new GISTEarthColorMap());
    if (zInterval.isValid()) {
        if (zInterval.width() == 0) {
            //rightAxis->setColorBarInterval(QwtInterval(0.1, 1));
            rightAxis->setColorMap(QwtInterval(0.1, 1), const_cast<QwtColorMap *>(rightAxis->colorMap()));
            //setAxisScale(QwtPlot::yRight, 0, 1 );
            setAxisScale(QwtPlot::yRight, 0.1, 1 );
        } else {
            //rightAxis->setColorBarInterval( zInterval);
            rightAxis->setColorMap( zInterval, const_cast<QwtColorMap *>(rightAxis->colorMap()));
            setAxisScale(QwtPlot::yRight, zInterval.minValue(), zInterval.maxValue() );
        }
    } else {
        rightAxis->setColorMap(QwtInterval(0.1, 1), const_cast<QwtColorMap *>(rightAxis->colorMap()));
        //rightAxis->setColorBarInterval(QwtInterval(0.1, 1));
        //setAxisScale(QwtPlot::yRight, 0, 1 );
        setAxisScale(QwtPlot::yRight, 0.1, 1 );
    }

    //setAxisScale(QwtPlot::yRight, zInterval.minValue(), zInterval.maxValue() );
    replot();
}

void SpectroScatterPlot::setColorMap(int cm) {
    _cm_index = cm;
    const QwtInterval zInterval = d_curve->data()->interval( Qt::ZAxis );
    QwtScaleWidget *rightAxis = axisWidget(QwtPlot::yRight);

    if (_cm_mode) {
        setAxisScaleEngine(QwtPlot::yRight, new QwtLogScaleEngine());
    } else {
        setAxisScaleEngine(QwtPlot::yRight, new QwtLinearScaleEngine());
    }
    
    /* Based on index of combobox in specbox.cpp */
    switch(cm) {
    case 0: {
        d_curve->setColorMap(new GISTEarthColorMap(_cm_mode));
        rightAxis->setColorMap(zInterval, new GISTEarthColorMap(_cm_mode));
        break;
    }
    case 1: {
        d_curve->setColorMap(new GISTSternColorMap(_cm_mode));
        rightAxis->setColorMap(zInterval, new GISTSternColorMap(_cm_mode));

        break;
    }
    case 2: {
        d_curve->setColorMap(new GISTHeatColorMap(_cm_mode));
        rightAxis->setColorMap(zInterval, new GISTHeatColorMap(_cm_mode));

        break;
    }
    case 3: {
        d_curve->setColorMap(new SpectroColorMap(_cm_mode));
        rightAxis->setColorMap(zInterval, new SpectroColorMap(_cm_mode));

        break;
    }
    case 4: {
        d_curve->setColorMap(new GISTGrayColorMap(_cm_mode));
        rightAxis->setColorMap(zInterval, new GISTGrayColorMap(_cm_mode));

        break;
    }
    case 5: {
        d_curve->setColorMap(new GISTYargColorMap(_cm_mode));
        rightAxis->setColorMap(zInterval, new GISTYargColorMap(_cm_mode));

        break;
    }
    case 6: {
        d_curve->setColorMap(new SpectralColorMap(_cm_mode));
        rightAxis->setColorMap(zInterval, new SpectralColorMap(_cm_mode));

        break;
    }
    case 7: {
        d_curve->setColorMap(new SpectralTwoColorMap(_cm_mode));
        rightAxis->setColorMap(zInterval, new SpectralTwoColorMap(_cm_mode));

        break;
    }
    case 8: {
        d_curve->setColorMap(new BoneColorMap(_cm_mode));
        rightAxis->setColorMap(zInterval, new BoneColorMap(_cm_mode));

        break;
    }
    case 9: {
        d_curve->setColorMap(new IshiharaColorMap(_cm_mode));
        rightAxis->setColorMap(zInterval, new IshiharaColorMap(_cm_mode));

        break;
    }
    case 10: {
        d_curve->setColorMap(new BobbyColorMap(_cm_mode));
        rightAxis->setColorMap(zInterval, new BobbyColorMap(_cm_mode));

        break;
    }
    default: {
        d_curve->setColorMap(new GISTEarthColorMap(_cm_mode));
        rightAxis->setColorMap(zInterval, new GISTEarthColorMap(_cm_mode));

        break;
    }
    }


    replotWithScale();
}        

void SpectroScatterPlot::setColorMapMode(bool cm_mode) {
    if (cm_mode) {
        _cm_mode = 0;
    } else {
        _cm_mode = 1;
    }
    // if (cm_mode > 0) {
    //     _cm_mode = 1;
    // } else {
    //     _cm_mode = 0;
    // }

    setColorMap(_cm_index);
}
