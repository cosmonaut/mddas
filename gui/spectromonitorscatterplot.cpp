#include <stdint.h>
#include <math.h>
#include <qnumeric.h>
//#include <qwt/qwt_plot_shapeitem.h>
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


static inline QVector<uint> get_divisors(uint);

class SpectrogramMonitorData: public QwtMatrixRasterData {
public:
    SpectrogramMonitorData(uint x_max, uint y_max)
    {
        x_lim = x_max;
        y_lim = y_max;

        this->rebin_vals = NULL;
        this->_hash = NULL;
        this->_x_list = NULL;
        this->_y_list = NULL;

        if (x_lim == y_lim) {
            /* Possible to rebin (square) */
            _can_rebin = true;
        } else {
            /* Cannot rebin */
            _can_rebin = false;
        }

        /* We are not currently rebinning */
        _rebin = false;

        /* create private pixel array */
        this->vals = new uint8_t[x_max*y_max];
        /* Set all pixel vals to 0 */
        memset(vals, 0, x_max*y_max*sizeof(uint8_t));

        /* Create photon list arrays */
        _x_list = new QVector<uint>;
        _y_list = new QVector<uint>;

        /* Set plot size and z scale */
        setInterval( Qt::XAxis, QwtInterval( 0.0, x_max ) );
        setInterval( Qt::YAxis, QwtInterval( 0.0, y_max ) );
        setInterval( Qt::ZAxis, QwtInterval( 0.0, 1.0 ) );
    }

    ~SpectrogramMonitorData() {
        /* IMPORTANT! Delete the vals array */
        delete[] this->vals;
        if (this->rebin_vals) {
            delete[] this->rebin_vals;
        }
        if (this->_hash) {
            delete this->_hash;
        }
        if (this->_x_list) {
            delete this->_x_list;
        }
        if (this->_y_list) {
            delete this->_y_list;
        }
    }

    inline void append(const QVector<MDDASDataPoint> &vec) {
        /* Append new z values */
        for (uint i = 0; i < (uint)vec.size(); i++) {
            if (((uint)vec[i].x() < x_lim) && ((uint)vec[i].y() < y_lim)) {
                /* Keep track of photon list as well */
                _x_list->append((uint)vec[i].x());
                _y_list->append((uint)vec[i].y());

                /* Build image with each photon */
                if (!vals[y_lim*(uint)vec[i].x() + (uint)vec[i].y()]) {
                    vals[y_lim*(uint)vec[i].x() + (uint)vec[i].y()] += 1;
                }

                if (_rebin) {
                    /* If bin is empty add a photon in the rebinned image */
                    if (rebin_vals[_rebin_size*_hash->value((uint)vec[i].x()) + _hash->value((uint)vec[i].y())] < 1) {
                        ++rebin_vals[_rebin_size*_hash->value((uint)vec[i].x()) + _hash->value((uint)vec[i].y())];
                    }
                }
            }
        }
    }
    
    inline void clear() {
        /* Set all pixel values to 0 */
        memset(vals, 0, x_lim*y_lim*sizeof(uint8_t));

        /* If in rebin mode, clear rebin image */
        if (_rebin) {
            memset(rebin_vals, 0, _rebin_size*_rebin_size*sizeof(uint8_t));
        }

        /* Clear photon list */
        _x_list->clear();
        _x_list->squeeze();
        _y_list->clear();
        _y_list->squeeze();
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
        
        // /* Use only data within the plot region */
        // if ( !( xInterval.contains(x) && yInterval.contains(y) ) )
        //     return qQNaN();

        // /* Overloaded value function that returns image z vals from map. */
        // if (x < x_lim && y < y_lim) {
        //     return (double)vals[y_lim*(uint)x + (uint)y];
        // } else {
        //     /* Edge case! */
        //     return (double)0;
        // }


        if (!_rebin) {
            /* Display full resolution image */
            /* Use only data within the plot region */
            if ( !( xInterval.contains(x) && yInterval.contains(y) ) )
                return qQNaN();

            if (x < x_lim && y < y_lim) {
                return (double)vals[y_lim*(uint)x + (uint)y];
            } else {
                /* Edge case! */
                return (double)0.0;
            }
        } else {
            /* Display rebinned image */

            /* Use only data within the plot region */
            if ( !( xInterval.contains(x) && yInterval.contains(y) ) )
                return qQNaN();

            if (x < _rebin_size && y < _rebin_size) {
                return (double)rebin_vals[_rebin_size*(uint)x + (uint)y];
            } else {
                /* Edge case! */
                return (double)0.0;
            }
        }
    }

    /* Rebin the image by an integer factor. Rebinning is currently
       only allowed for square images. */
    inline int rebin(uint factor) {
        uint i = 0;
        uint px = 0;
        //uint temp_v = 0;

        /* Save axis size of rebin image */
        _rebin_size = x_lim/factor;

        if (_can_rebin) {
            if (factor > 1) {
                /* Create rebinned image buffer */
                if (this->rebin_vals) {
                    delete[] this->rebin_vals;
                    this->rebin_vals = NULL;
                }
                this->rebin_vals = new uint8_t[_rebin_size*_rebin_size];
                /* Initialize image */
                memset(rebin_vals, 0, _rebin_size*_rebin_size*sizeof(uint8_t));

                /* Create hash for rebin image. This is a lookup for
                   rebin pixel location from full res pixel
                   location. */
                if (this->_hash) {
                    delete this->_hash;
                    this->_hash = NULL;
                }
                _hash = new QHash<uint, uint>;
                /* Populate hash */
                for (i = 0; i < x_lim; i++) {
                    _hash->insert((uint)i, px);

                    if ((i%factor) == (factor - 1)) {
                        px++;
                    }
                }

                /* Populate rebin image from photon list. Note that
                   for large images this method is faster than
                   building the rebin from the full resolution
                   image. */
                for (i = 0; i < (uint)_x_list->size(); i++) {
                    if (this->rebin_vals[_rebin_size*_hash->value(_x_list->value(i)) + _hash->value(_y_list->value(i))] < 1) {
                        this->rebin_vals[_rebin_size*_hash->value(_x_list->value(i)) + _hash->value(_y_list->value(i))] = 1;
                    }
                }

                /* We are in rebinned mode now... */
                _rebin = true;

                /* Update axes to reflect rebin image size */
                setInterval( Qt::XAxis, QwtInterval( 0.0, _rebin_size ) );
                setInterval( Qt::YAxis, QwtInterval( 0.0, _rebin_size ) );
                setInterval( Qt::ZAxis, QwtInterval( 0.0, 1.0 ) );
            } else {
                /* Back to full resolution. */
                _rebin = false;
                if (this->rebin_vals) {
                    delete[] this->rebin_vals;
                    this->rebin_vals = NULL;
                }

                setInterval( Qt::XAxis, QwtInterval( 0.0, x_lim) );
                setInterval( Qt::YAxis, QwtInterval( 0.0, y_lim ) );
                setInterval( Qt::ZAxis, QwtInterval( 0.0, 1.0 ) );
            }
        } else {
            /* Return an error if we can't rebin */
            return(-1);
        }

        return(0);
    }


private:
    uint8_t *vals; /* matrix of pixels */
    uint x_lim;
    uint y_lim;

    uint8_t *rebin_vals;
    bool _can_rebin;
    bool _rebin;
    uint _rebin_size;
    QHash<uint, uint> *_hash;
    /* Vectors to hold photon list */
    QVector<uint> *_x_list;
    QVector<uint> *_y_list;
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

    /* We will use this to show spec rectangles later... */
    // QRectF myrect(4096, 4096, 100, 100);
    
    // QwtPlotShapeItem *shape = new QwtPlotShapeItem;
    // shape->setPen(Qt::white, 1.0, Qt::SolidLine);
    // shape->setRect(myrect);
    
    // shape->attach(this);

    //(void) new Zoomer(canvas(), x_max);
    _z = new Zoomer((QwtPlotCanvas *)canvas(), x_max);

    if (x_max == y_max) {
        _can_rebin = true;
        _divisors = get_divisors(x_max);
        emit divisorsChanged();
    } else {
        _can_rebin = false;
        _divisors = get_divisors(1);
        emit divisorsChanged();
    }

    _binned = false;

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
    /* No need to delete old data -- Qwt handles this! */
    //delete d_curve->data();

    d_curve->setData(new SpectrogramMonitorData(pc.getXMax(), pc.getYMax()));

    /* Store new plot limits */
    _x_max = pc.getXMax();
    _y_max = pc.getYMax();

    setAxisScale(xBottom, 0, pc.getXMax());
    setAxisScale(yLeft, 0, pc.getYMax());

    if (_x_max == _y_max) {
        /* Do rebin logic */
        _can_rebin = true;
        _divisors = get_divisors(pc.getXMax());
        /* emit divisor change... */
        emit divisorsChanged();
    } else {
        /* can't rebin... */
        _can_rebin = false;
        _divisors = get_divisors(1);
        emit divisorsChanged();
    }

    _z->setZoomBase();

    replot();
}

QVector<uint> SpectroMonitorScatterPlot::getRebinDivisors(void) {
    return(_divisors);
}

void SpectroMonitorScatterPlot::rebin(uint factor) {
    SpectrogramMonitorData *data = static_cast<SpectrogramMonitorData *>( d_curve->data() );
    int status = 0;

    if (_can_rebin) {
        if (!_binned && (factor == 1)) {
            /* No need to rebin */
            //qDebug() << "don't need to rebin";
        } else {
            /* Request a rebin */
            status = data->rebin(factor);
            if (status < 0) {
                qDebug() << "ERROR: Plot failed to rebin!";
            } else {
                replot();

                if (factor > 1) {
                    _binned = true;
                    this->setAxisScale(xBottom, 0, (double)(_x_max/factor));
                    this->setAxisScale(yLeft, 0, (double)(_y_max/factor));
                } else {
                    /* full resolution */
                    _binned = false;
                    setAxisScale(xBottom, 0, _x_max);
                    setAxisScale(yLeft, 0, _y_max);

                }
            }
        }
    } else {
        qDebug() << "can't rebin!";
    }

    _z->setZoomBase();

    replot();
}

/* Helper to find all divisors of an axis size */
static inline QVector<uint> get_divisors(uint n) {
    uint i = 2;
    QVector<uint> divs;

    /* We add 1 as an option to go back to full resolution */
    divs.append(1);

    while(i < sqrt(n)) {
        if(n%i == 0) {
            divs.append(i);
        }

        i++;
    }
    if(i*i == n) {
        divs.append(i);
    }

    return(divs);
}
