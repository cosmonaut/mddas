#include <qnumeric.h>
#include <qwt_plot.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_shapeitem.h>
#include <qwt_plot_spectrogram.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_rescaler.h>
#include <qwt_plot_shapeitem.h>
#include <qwt_color_map.h>
#include <qwt_scale_engine.h>
#include <qwt_scale_widget.h>
#include <qwt_matrix_raster_data.h>
#include <algorithm>
#include <QDebug>
#include <QColor>

#include "boxpicker.h"
#include "spectroscatterplot.h"
#include "colormaps.h"
#include "mddasplotconfig.h"
#include "mddasdatapoint.h"
#include "zoomer.h"



static inline QVector<uint> get_divisors(uint);

class SpectrogramData: public QwtMatrixRasterData {
public:
    SpectrogramData(uint x_max, uint y_max) {
        _max_v = 0;
        x_lim = x_max;
        y_lim = y_max;
        _rebin_max_v = 0;

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

        /* Create the private array */
        /* Note: this is mapped via index magic */
        this->vals = new uint[x_max*y_max];
        /* Create photon list arrays */
        _x_list = new QVector<uint>;
        _y_list = new QVector<uint>;
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

    inline void updateScale() {
        /* Update the scale to reflect the largest Z value */
        //setInterval( Qt::ZAxis, QwtInterval( 0.0, (double)*std::max_element(vals, vals + x_lim*y_lim)));

        if (!_rebin) {
            setInterval( Qt::ZAxis, QwtInterval( 0.1, (double)_max_v));
        } else {
            setInterval( Qt::ZAxis, QwtInterval( 0.1, (double)_rebin_max_v));
        }
    }

    inline void append(const QVector<MDDASDataPoint> &vec) {

        /* Append new z vals. */
        for (uint i = 0; i < (uint)vec.size(); i++) {
            if (((uint)vec[i].x() < x_lim) && ((uint)vec[i].y() < y_lim)) {
                /* Keep track of photon list as well */
                _x_list->append((uint)vec[i].x());
                _y_list->append((uint)vec[i].y());

                vals[y_lim*(uint)vec[i].x() + (uint)vec[i].y()] += 1;
                if (_rebin) {
                    ++rebin_vals[_rebin_size*_hash->value((uint)vec[i].x()) + _hash->value((uint)vec[i].y())];

                    if (rebin_vals[_rebin_size*_hash->value((uint)vec[i].x()) + _hash->value((uint)vec[i].y())] > _rebin_max_v) {
                        ++_rebin_max_v;
                    }
                }

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
        if (_rebin) {
            memset(rebin_vals, 0, _rebin_size*_rebin_size*sizeof(unsigned int));
            _rebin_max_v = 0;
        }
        /* Set max amplitude to 0 */
        _max_v = 0;

        /* Clear photon list */
        _x_list->clear();
        _x_list->squeeze();
        _y_list->clear();
        _y_list->squeeze();
    }        

    /* Overloaded value function that returns image z vals from map. */
    virtual double value(double x, double y) const {
        const QwtInterval xInterval = interval( Qt::XAxis );
        const QwtInterval yInterval = interval( Qt::YAxis );
        

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
        uint temp_v = 0;

        /* Save axis size of rebin image */
        _rebin_size = x_lim/factor;

        if (_can_rebin) {
            if (factor > 1) {
                /* Create rebinned image buffer */
                if (this->rebin_vals) {
                    delete[] this->rebin_vals;
                    this->rebin_vals = NULL;
                }
                this->rebin_vals = new uint[_rebin_size*_rebin_size];
                /* Initialize image */
                memset(rebin_vals, 0, _rebin_size*_rebin_size*sizeof(unsigned int));

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

                /* This commented method is left as an
                   example. Building the rebinned image is faster via
                   a photon list. The commneted method rebuilds the
                   rebinned image from the full resolution image but
                   it painfully slow for a GUI... */
                /* Populate rebin image... */
                // for (tx = 0; tx < x_lim; tx++) {
                //     for (ty = 0; ty < y_lim; ty++) {
                //         this->rebin_vals[rebin_size*_hash->value(tx) + _hash->value(ty)] = this->vals[y_lim*tx + ty];
                //         if (this->rebin_vals[rebin_size*_hash->value(tx) + _hash->value(ty)] > _rebin_max_v) {
                //             _rebin_max_v = this->rebin_vals[rebin_size*_hash->value(tx) + _hash->value(ty)];
                //         }
                //     }
                // }

                /* Reset _rebin_max_v because it will be recalced */
                _rebin_max_v = 0;

                /* Populate rebin image from photon list. Note that
                   for large images this method is faster than
                   building the rebin from the full resolution
                   image. */
                for (i = 0; i < (uint)_x_list->size(); i++) {
                    temp_v = ++this->rebin_vals[_rebin_size*_hash->value(_x_list->value(i)) + _hash->value(_y_list->value(i))];

                    if (temp_v > _rebin_max_v) {
                        _rebin_max_v = temp_v;
                    }

                }

                /* We are in rebinned mode now... */
                _rebin = true;

                /* Update axes to reflect rebin image size */
                setInterval( Qt::XAxis, QwtInterval( 0.0, _rebin_size ) );
                setInterval( Qt::YAxis, QwtInterval( 0.0, _rebin_size ) );
                setInterval( Qt::ZAxis, QwtInterval( 0.0, 1.0 ) );
                updateScale();

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
                updateScale();

            }
        } else {
            /* Return an error if we can't rebin */
            return(-1);
        }

        return(0);
    }

private:
    uint *vals; /* matrix of image z values */
    /* Array that holds rebinned image */
    uint *rebin_vals;
    uint x_lim;
    uint y_lim;
    uint _max_v;
    bool _can_rebin;
    bool _rebin;
    uint _rebin_size;
    /* Holds the largest z value in a rebinned image */
    uint _rebin_max_v;
    /* Lookup map for rebin image */
    QHash<uint, uint> *_hash;
    /* Vectors to hold photon list */
    QVector<uint> *_x_list;
    QVector<uint> *_y_list;
};


SpectroScatterPlot::SpectroScatterPlot(QWidget *parent, uint x_max, uint y_max): 
    QwtPlot(parent),
    d_curve(NULL),
    box1(NULL),
    box2(NULL),
    box3(NULL)
{
    setFrameStyle(QFrame::NoFrame);
    setLineWidth(0);
    //setCanvasLineWidth(0);

    _x_max = x_max;
    _y_max = y_max;

    /* rebin factor 1 by default */
    _rebin_f = 1;

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

    /* Rectangles for CHESS widget */
    box1 = new QwtPlotShapeItem();
    box1->setPen(QColor(102, 153, 204), 1.8, Qt::SolidLine);
    box1->attach(this);

    box2 = new QwtPlotShapeItem();
    box2->setPen(QColor(153, 204, 187), 1.8, Qt::SolidLine);
    box2->attach(this);

    box3 = new QwtPlotShapeItem();
    box3->setPen(QColor(255, 102, 85), 1.8, Qt::SolidLine);
    box3->attach(this);


    //(void) new Zoomer(canvas(), x_max);
    _z = new Zoomer((QwtPlotCanvas*)canvas(), x_max);
    _z->setEnabled(true);


    /* picker for boxes */
    _box1Picker = new BoxPicker(this->xBottom, this->yLeft, this->canvas());
    connect(_box1Picker, SIGNAL(selected(const QPointF&)), this, SLOT(on1Selected(const QPointF&)));
    _box1Picker->setColor(QColor(102, 153, 204));
    _box1Picker->setEnabled(false);

    _box2Picker = new BoxPicker(this->xBottom, this->yLeft, this->canvas());
    connect(_box2Picker, SIGNAL(selected(const QPointF&)), this, SLOT(on2Selected(const QPointF&)));
    _box2Picker->setColor(QColor(153, 204, 187));
    _box2Picker->setEnabled(false);

    _box3Picker = new BoxPicker(this->xBottom, this->yLeft, this->canvas());
    connect(_box3Picker, SIGNAL(selected(const QPointF&)), this, SLOT(on3Selected(const QPointF&)));
    _box3Picker->setColor(QColor(255, 102, 85));
    _box3Picker->setEnabled(false);

    setColorMap(0);
    
    replot();

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
}

SpectroScatterPlot::~SpectroScatterPlot() {
    //delete d_curve;
}

void SpectroScatterPlot::on1Selected(const QPointF& p) {
    int tempx = 0;
    int tempy = 0;

    uint newx = 0;
    uint newy = 0;

    if (_binned) {
        tempx = ((uint)p.x()*_rebin_f - (uint)(box1rect.width()/2.0));
        tempy = ((uint)p.y()*_rebin_f - (uint)(box1rect.height()/2.0));
    } else {
        tempx = ((uint)p.x() - (uint)(box1rect.width()/2.0));
        tempy = ((uint)p.y() - (uint)(box1rect.height()/2.0));
    }

    if ((tempx < 0) || tempy < 0) {
        _box1Picker->setEnabled(false);
        _z->setEnabled(true);
        emit boxpos(0, 0, 0);
        return;
    }

    if ((tempx + (uint)(box1rect.width()/2.0)) > _x_max || (tempy + (uint)(box1rect.height()/2.0)) > _y_max) {
        _box1Picker->setEnabled(false);
        _z->setEnabled(true);
        emit boxpos(0, 0, 0);
        return;
    }

    newx = (uint)tempx;
    newy = (uint)tempy;

    emit boxpos(1, newx, newy);

    _box1Picker->setEnabled(false);
    _z->setEnabled(true);
}

void SpectroScatterPlot::on2Selected(const QPointF& p) {
    int tempx = 0;
    int tempy = 0;

    uint newx = 0;
    uint newy = 0;

    if (_binned) {
        tempx = ((uint)p.x()*_rebin_f - (uint)(box2rect.width()/2.0));
        tempy = ((uint)p.y()*_rebin_f - (uint)(box2rect.height()/2.0));
    } else {
        tempx = ((uint)p.x() - (uint)(box2rect.width()/2.0));
        tempy = ((uint)p.y() - (uint)(box2rect.height()/2.0));
    }

    if ((tempx < 0) || tempy < 0) {
        _box2Picker->setEnabled(false);
        _z->setEnabled(true);
        emit boxpos(0, 0, 0);
        return;
    }

    if ((tempx + (uint)(box2rect.width()/2.0)) > _x_max || (tempy + (uint)(box2rect.height()/2.0)) > _y_max) {
        _box2Picker->setEnabled(false);
        _z->setEnabled(true);
        emit boxpos(0, 0, 0);
        return;
    }

    newx = (uint)tempx;
    newy = (uint)tempy;

    emit boxpos(2, newx, newy);

    _box2Picker->setEnabled(false);
    _z->setEnabled(true);
}

void SpectroScatterPlot::on3Selected(const QPointF& p) {
    int tempx = 0;
    int tempy = 0;

    uint newx = 0;
    uint newy = 0;

    if (_binned) {
        tempx = ((uint)p.x()*_rebin_f - (uint)(box3rect.width()/2.0));
        tempy = ((uint)p.y()*_rebin_f - (uint)(box3rect.height()/2.0));
    } else {
        tempx = ((uint)p.x() - (uint)(box3rect.width()/2.0));
        tempy = ((uint)p.y() - (uint)(box3rect.height()/2.0));
    }

    if ((tempx < 0) || tempy < 0) {
        _box3Picker->setEnabled(false);
        _z->setEnabled(true);
        emit boxpos(0, 0, 0);
        return;
    }

    if ((tempx + (uint)(box3rect.width()/2.0)) > _x_max || (tempy + (uint)(box3rect.height()/2.0)) > _y_max) {
        _box3Picker->setEnabled(false);
        _z->setEnabled(true);
        emit boxpos(0, 0, 0);
        return;
    }

    newx = (uint)tempx;
    newy = (uint)tempy;

    emit boxpos(3, newx, newy);

    _box3Picker->setEnabled(false);
    _z->setEnabled(true);
}

void SpectroScatterPlot::setPicker(uint p) {
    if (p == 0) {
        _box1Picker->setEnabled(false);
        _box2Picker->setEnabled(false);
        _box3Picker->setEnabled(false);
        _z->setEnabled(true);
    } else if (p == 1) {
        _z->setEnabled(false);
        _box2Picker->setEnabled(false);
        _box3Picker->setEnabled(false);
        _box1Picker->setEnabled(true);
    } else if (p == 2) {
        _z->setEnabled(false);
        _box1Picker->setEnabled(false);
        _box3Picker->setEnabled(false);
        _box2Picker->setEnabled(true);
    } else if (p == 3) {
        _z->setEnabled(false);
        _box1Picker->setEnabled(false);
        _box2Picker->setEnabled(false);
        _box3Picker->setEnabled(true);
    }
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

QVector<uint> SpectroScatterPlot::getRebinDivisors(void) {
    return(_divisors);
}

void SpectroScatterPlot::rebin(uint factor) {
    SpectrogramData *data = static_cast<SpectrogramData *>( d_curve->data() );
    int status = 0;

    if (_can_rebin) {
        if (!_binned && (factor == 1)) {
            /* No need to rebin */
            _rebin_f = 1;
            //qDebug() << "don't need to rebin";
        } else {
            /* Request a rebin */
            status = data->rebin(factor);
            if (status < 0) {
                qDebug() << "ERROR: Plot failed to rebin!";
                _rebin_f = 1;
            } else {
                replot();

                if (factor > 1) {
                    _binned = true;
                    this->setAxisScale(xBottom, 0, (double)(_x_max/factor));
                    this->setAxisScale(yLeft, 0, (double)(_y_max/factor));
                    _rebin_f = factor;
                    /* Fix boxes */
                    //refreshBoxes();
                } else {
                    /* full resolution */
                    _binned = false;
                    setAxisScale(xBottom, 0, _x_max);
                    setAxisScale(yLeft, 0, _y_max);
                    _rebin_f = 1;
                    //refreshBoxes();

                }
            }
        }
    } else {
        qDebug() << "can't rebin!";
        _rebin_f = 1;
    }

    refreshBoxes();

    _z->setZoomBase();

    replot();
}

void SpectroScatterPlot::refreshBoxes(void) {
    if (_binned) {
        box1->setRect(QRectF(box1rect.x()/_rebin_f,
                             box1rect.y()/_rebin_f,
                             box1rect.width()/_rebin_f,
                             box1rect.height()/_rebin_f));

        box2->setRect(QRectF(box2rect.x()/_rebin_f,
                             box2rect.y()/_rebin_f,
                             box2rect.width()/_rebin_f,
                             box2rect.height()/_rebin_f));

        box3->setRect(QRectF(box3rect.x()/_rebin_f,
                             box3rect.y()/_rebin_f,
                             box3rect.width()/_rebin_f,
                             box3rect.height()/_rebin_f));

    } else {
        box1->setRect(QRectF(box1rect.x(),
                             box1rect.y(),
                             box1rect.width(),
                             box1rect.height()));

        box2->setRect(QRectF(box2rect.x(),
                             box2rect.y(),
                             box2rect.width(),
                             box2rect.height()));

        box3->setRect(QRectF(box3rect.x(),
                             box3rect.y(),
                             box3rect.width(),
                             box3rect.height()));

    }        
    
    replot();
}

void SpectroScatterPlot::setBox1(uint x_min, uint x_max, uint y_min, uint y_max) {
    /* Test limits */
    if (x_min > _x_max) {
        return;
    }

    if (x_max > _x_max) {
        return;
    }

    if (y_min > _y_max) {
        return;
    }

    if (y_max > _y_max) {
        return;
    }


    if (_binned) {
        box1rect.setRect(x_min,
                         y_min,
                         x_max - x_min,
                         y_max - y_min);
        box1->setRect(box1rect);
    } else {
        // QRectF myrect(4096, 4096, 100, 100);
        //QRectF r(x_min, y_min, (x_max - x_min), (y_max - y_min));
        box1rect.setRect(x_min, y_min, (x_max - x_min), (y_max - y_min));
        box1->setRect(box1rect);
    }
    
    refreshBoxes();
}

void SpectroScatterPlot::setBox2(uint x_min, uint x_max, uint y_min, uint y_max) {
    /* Test limits */
    if (x_min > _x_max) {
        return;
    }

    if (x_max > _x_max) {
        return;
    }

    if (y_min > _y_max) {
        return;
    }

    if (y_max > _y_max) {
        return;
    }


    if (_binned) {
        box2rect.setRect(x_min,
                         y_min,
                         x_max - x_min,
                         y_max - y_min);
        box2->setRect(box2rect);
    } else {
        // QRectF myrect(4096, 4096, 100, 100);
        //QRectF r(x_min, y_min, (x_max - x_min), (y_max - y_min));
        box2rect.setRect(x_min, y_min, (x_max - x_min), (y_max - y_min));
        box2->setRect(box2rect);
    }

    refreshBoxes();        
}

void SpectroScatterPlot::setBox3(uint x_min, uint x_max, uint y_min, uint y_max) {
    /* Test limits */
    if (x_min > _x_max) {
        return;
    }

    if (x_max > _x_max) {
        return;
    }

    if (y_min > _y_max) {
        return;
    }

    if (y_max > _y_max) {
        return;
    }


    if (_binned) {
        box3rect.setRect(x_min,
                         y_min,
                         x_max - x_min,
                         y_max - y_min);
        box3->setRect(box3rect);
    } else {
        // QRectF myrect(4096, 4096, 100, 100);
        //QRectF r(x_min, y_min, (x_max - x_min), (y_max - y_min));
        box3rect.setRect(x_min, y_min, (x_max - x_min), (y_max - y_min));
        box3->setRect(box3rect);
    }

    refreshBoxes();        
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
