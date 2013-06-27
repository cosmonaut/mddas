#include <qnumeric.h>
#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_grid.h>
#include <qwt/qwt_plot_canvas.h>
#include <qwt/qwt_plot_spectrogram.h>
#include <qwt/qwt_plot_layout.h>
#include <qwt/qwt_color_map.h>
#include <qwt/qwt_scale_widget.h>
#include <qwt/qwt_matrix_raster_data.h>

#include "mddasplotconfig.h"
#include "mddasdatapoint.h"
#include "spectromonitorscatterplot.h"
#include "zoomer.h"


class SpectrogramMonitorData: public QwtMatrixRasterData {
public:
    SpectrogramMonitorData(uint x_max, uint y_max)
    {
        x_lim = x_max;
        y_lim = y_max;

        /* create private pixel array */
        this->vals = new uint[x_max*y_max];
        /* Set all pixel vals to 0 */
        memset(vals, 0, x_max*y_max*sizeof(unsigned int));

        /* Set plot size and z scale */
        setInterval( Qt::XAxis, QwtInterval( 0.0, x_max ) );
        setInterval( Qt::YAxis, QwtInterval( 0.0, y_max ) );
        setInterval( Qt::ZAxis, QwtInterval( 0.0, 1.0 ) );
    }

    ~SpectrogramMonitorData() {
        /* IMPORTANT! Delete the vals array */
        delete[] this->vals;
    }

    inline void append(const QVector<MDDASDataPoint> &vec) {
        /* Append new z values */
        for (uint i = 0; i < (uint)vec.size(); i++) {
            if (((uint)vec[i].x() < x_lim) && ((uint)vec[i].y() < y_lim)) {
                vals[y_lim*(uint)vec[i].x() + (uint)vec[i].y()] += 1;
            }
        }
    }
    
    inline void clear() {
        /* Set all pixel values to 0 */
        memset(vals, 0, x_lim*y_lim*sizeof(unsigned int));
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

    virtual double value(double x, double y) const {
        /* Overloaded value function that returns image z vals from map. */
        const QwtInterval xInterval = interval( Qt::XAxis );
        const QwtInterval yInterval = interval( Qt::YAxis );
        
        /* Use only data within the plot region */
        if ( !( xInterval.contains(x) && yInterval.contains(y) ) )
            return qQNaN();

        /* Overloaded value function that returns image z vals from map. */
        if (x < x_lim && y < y_lim) {
            return (double)vals[y_lim*(uint)x + (uint)y];
        } else {
            /* Edge case! */
            return (double)0;
        }
    }

private:
    uint *vals; /* matrix of pixels */
    uint x_lim;
    uint y_lim;
};

class ColorMap: public QwtLinearColorMap {
public:
    ColorMap():
        QwtLinearColorMap(Qt::black, Qt::white) {
    }
};

SpectroMonitorScatterPlot::SpectroMonitorScatterPlot(QWidget *parent, uint x_max, uint y_max): 
    QwtPlot(parent),
    d_curve(NULL)
{
    setFrameStyle(QFrame::NoFrame);
    setLineWidth(0);
    //setCanvasLineWidth(0);

    ((QwtPlotCanvas*)canvas())->setLineWidth(0);
    updateLayout();

    plotLayout()->setAlignCanvasToScales(true);

    QwtPlotGrid *grid = new QwtPlotGrid;
    grid->setMajorPen(QPen(Qt::gray, 0, Qt::DotLine));
    grid->attach(this);

    setCanvasBackground(QColor(0, 0, 0)); 

    setAxisScale(xBottom, 0, x_max);
    setAxisScale(yLeft, 0, y_max);

    d_curve = new QwtPlotSpectrogram();
    d_curve->setData( new SpectrogramMonitorData(x_max, y_max) );
    d_curve->setRenderHint(QwtPlotItem::RenderAntialiased, false);
    d_curve->setColorMap(new ColorMap());
    d_curve->attach(this);

    setAutoReplot(false);

    //(void) new Zoomer(canvas(), x_max);
    _z = new Zoomer((QwtPlotCanvas *)canvas(), x_max);

    replot();
}

SpectroMonitorScatterPlot::~SpectroMonitorScatterPlot() {
    //delete d_curve;
}

void SpectroMonitorScatterPlot::appendPoints(const QVector<MDDASDataPoint> &v) {
    SpectrogramMonitorData *data = static_cast<SpectrogramMonitorData *>( d_curve->data() );
    data->append(v);
    
    //replot();
}

void SpectroMonitorScatterPlot::clear() {
    SpectrogramMonitorData *data = static_cast<SpectrogramMonitorData *>( d_curve->data() );
    data->clear();
    replot();
}

QSize SpectroMonitorScatterPlot::sizeHint() const {
    //return QSize(300,300);
    return maximumSize();
}

void SpectroMonitorScatterPlot::configure(MDDASPlotConfig pc) {
    // qDebug() << "deleting old data";
    delete d_curve->data();
    // qDebug() << "deleted old data";
    //QString str;
    //str.setNum(pc.getXMax());

    //qDebug() << "specmon x max: " << pc.getXMax();
    d_curve->setData(new SpectrogramMonitorData(pc.getXMax(), pc.getYMax()));
    //qDebug() << "made new data";
    setAxisScale(xBottom, 0, pc.getXMax());
    setAxisScale(yLeft, 0, pc.getYMax());

    _z->setZoomBase();

    replot();
}

