#include <QVBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QGroupBox>
#include "specplot.h"
#include "collapsedplotbox.h"
#include "spectromonitorscatterplot.h"
#include "mddasplotconfig.h"
#include "mddasdatapoint.h"

CollPlotBox::CollPlotBox(QWidget *parent):
    QWidget(parent) {

    QVBoxLayout *layout = new QVBoxLayout(this);
    QHBoxLayout *hbox = new QHBoxLayout();

    QWidget *hFiller = new QWidget;
    hFiller->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    _refreshTimer = new QTimer(this);
    _refreshTimer->setInterval(1000);

    _plot = new SpecPlot(this, 2, 1, 1);

    QwtText mon_title("Plotty-Plot");
    QFont font = _plot->title().font();
    font.setPointSize(8);
    mon_title.setFont(font);
    _plot->setTitle(mon_title);

    _replotTimer = new QTimer(this);
    _replotTimer->setInterval(200);
    _replotTimer->setSingleShot(false);
    _replotTimer->stop();

    connect(_replotTimer, SIGNAL( timeout() ), _plot, SLOT( replot() ));
//    connect(_refreshRateSpinBox, SIGNAL( valueChanged(double) ), this, SLOT( setRefreshTimer(double) ));
    connect(_refreshTimer, SIGNAL( timeout() ), _plot, SLOT( clear() ));


//    QGroupBox *refreshBox = new QGroupBox(this);
//    refreshBox->setTitle("Refresh");

//    QHBoxLayout *refreshLayout = new QHBoxLayout();
//    refreshLayout->addWidget(new QLabel("Refresh Rate: ", this));
//    refreshLayout->addWidget(_refreshRateSpinBox);

//    refreshBox->setLayout(refreshLayout);


//    QGroupBox *rebinBox = new QGroupBox(this);
//    rebinBox->setTitle("Rebin Factor");

//    _rebinSelector = new QComboBox(this);

//    QHBoxLayout *rebinLayout = new QHBoxLayout();
//    rebinLayout->addWidget(_rebinSelector);

//    rebinBox->setLayout(rebinLayout);

//    connect(_plot, SIGNAL( divisorsChanged() ), this, SLOT( updateDivisors() ));
//    connect(_rebinSelector, SIGNAL( currentIndexChanged(int) ), this, SLOT( doRebin(int) ));

//    hbox->addWidget(refreshBox);
//    hbox->addWidget(rebinBox);
//    hbox->addWidget(hFiller);

    _x_min = 0;
    _x_max = 0;
    _y_min = 0;
    _y_max = 0;

    _hash = new QHash<uint, uint>();
    _hash->clear();
    for (int i = 0; i < (int)_x_max; i++) {
        _hash->insert(i,i);
    }
    


    layout->addWidget(_plot);
    layout->addLayout(hbox);
    setLayout(layout);

    _plot->replot();

    setMaximumSize(500,500);
}

void CollPlotBox::activate(bool b) {
    if (b) {
        // Start replot timer
        _replotTimer->start();
        _refreshTimer->start();
//        _refreshRateSpinBox->setEnabled(false);
    } else {
        // stop replot timer
        _replotTimer->stop();
        _refreshTimer->stop();
//        _refreshRateSpinBox->setEnabled(true);
    }
    _active = b;
}

void CollPlotBox::setRefreshTimer(double d) {
    if (_refreshTimer) {
        _refreshTimer->setInterval(int(d*1000.0));
    }
}

void CollPlotBox::configure(MDDASPlotConfig pc) {
    //QString str;
    //str.setNum(pc.getXMax());
    //qDebug() << "specmonbox x: " << pc.getXMax();
    //str.setNum(pc.getYMax());
    //qDebug() << "specmonbox y: " << pc.getYMax();
    _plot->configure(pc);
}

void CollPlotBox::clear() {
    _plot->clear();
}

void CollPlotBox::append(const QVector<MDDASDataPoint> &dv) {
    _plot->appendPoints(dv);
    
    
    SpecCurveData *data = static_cast<SpecCurveData *>( d_curve->data() );

    QVector<QwtIntervalSample> samples = data->samples();
    QVector<double> newvals(samples.size());

    for (int i = 0; i < dv.size(); i++) {
        if ((dv[i].x() < (int)_x_max) && dv[i].x() >= (int)_x_min) {
            if ((dv[i].y() < (int)_y_max) && dv[i].y() >= (int)_y_min) {
                
                newvals[_hash->value(dv[i].y())] += 1;

            }
        }
    }
}

void CollPlotBox::replot() {
    _plot->replot();
}

bool CollPlotBox::isActive() {
    return _active;
}

void CollPlotBox::updateDivisors() {
//    int i = 0;
//    QVector<uint> divs;

//    /* List of possible integer divisors */
//    divs = _plot->getRebinDivisors();

//    /* Must disconnect the combo before we change items */
//    disconnect(_rebinSelector, SIGNAL( currentIndexChanged(int) ), this, SLOT( doRebin(int) ));

//    _rebinSelector->clear();

//    for (i = 0; i < divs.size(); i++) {
//        _rebinSelector->addItem(QString::number(divs.value(i)));
//    }

//    /* Reconnect combobox now that we're done */
//    connect(_rebinSelector, SIGNAL( currentIndexChanged(int) ), this, SLOT( doRebin(int) ));
//    qDebug() << "update divs success";
}

/* Command plot to rebin. Note that this is a summing rebin */
void CollPlotBox::doRebin(int index) {
    //qDebug() << "rebin: " << index << " factor: " << (_rebinSelector->itemText(index)).toUInt();
    //_plot->rebin(_rebinSelector->itemText(index).toUInt());
    
    _rebin_f = index;
    _hash->clear();
    _hash->squeeze();
    for (i = _y_min; (uint)i < _y_max; i++) {
        _hash->insert((uint)i, px);

        if (((i - _y_min)%_rebin_f) == (_rebin_f - 1)) {
            px++;
        }
    }
}
