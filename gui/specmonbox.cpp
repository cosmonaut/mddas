#include <QVBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QDoubleSpinBox>
#include "specmonbox.h"
#include "spectromonitorscatterplot.h"
#include "mddasplotconfig.h"
#include "mddasdatapoint.h"

SpecMonBox::SpecMonBox(QWidget *parent):
    QWidget(parent) {

    QVBoxLayout *layout = new QVBoxLayout(this);
    QHBoxLayout *hbox = new QHBoxLayout();

    QWidget *hFiller = new QWidget;
    hFiller->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    _refreshRateSpinBox = new QDoubleSpinBox(this);
    _refreshRateSpinBox->setDecimals(1);
    _refreshRateSpinBox->setValue(1.0);
    _refreshRateSpinBox->setRange(0.1, 60.0);
    _refreshRateSpinBox->setSingleStep(1.0);

    _refreshTimer = new QTimer(this);
    _refreshTimer->setInterval(int(1000.0*_refreshRateSpinBox->value()));

    //_plot = new SpectroMonitorScatterPlot(this, getPlotConfig());
    _plot = new SpectroMonitorScatterPlot(this, 1, 1);

    QwtText mon_title("Monitor");
    QFont font = _plot->title().font();
    font.setPointSize(8);
    //mon_title.setFont(QFont("Helvetica", 12, -1));
    mon_title.setFont(font);
    _plot->setTitle(mon_title);

    _replotTimer = new QTimer(this);
    _replotTimer->setInterval(200);
    _replotTimer->setSingleShot(false);
    _replotTimer->stop();

    connect(_replotTimer, SIGNAL( timeout() ), _plot, SLOT( replot() ));
    connect(_refreshRateSpinBox, SIGNAL( valueChanged(double) ), this, SLOT( setRefreshTimer(double) ));
    connect(_refreshTimer, SIGNAL( timeout() ), _plot, SLOT( clear() ));

    hbox->addWidget(new QLabel("Refresh Rate: ", this));
    hbox->addWidget(_refreshRateSpinBox);
    hbox->addWidget(hFiller);

    layout->addWidget(_plot);
    layout->addLayout(hbox);
    setLayout(layout);

    _plot->replot();

    setMaximumSize(500,500);
}

void SpecMonBox::activate(bool b) {
    if (b) {
        // Start replot timer
        _replotTimer->start();
        _refreshTimer->start();
        _refreshRateSpinBox->setEnabled(false);
    } else {
        // stop replot timer
        _replotTimer->stop();
        _refreshTimer->stop();
        _refreshRateSpinBox->setEnabled(true);
    }
    _active = b;
}

void SpecMonBox::setRefreshTimer(double d) {
    if (_refreshTimer) {
        _refreshTimer->setInterval(int(d*1000.0));
    }
}

void SpecMonBox::configure(MDDASPlotConfig pc) {
    
    QString str;
    str.setNum(pc.getXMax());
    qDebug() << "specmonbox x: " << str;
    str.setNum(pc.getYMax());
    qDebug() << "specmonbox y: " << str;
    _plot->configure(pc);
}

void SpecMonBox::clear() {
    _plot->clear();
}

void SpecMonBox::append(const QVector<MDDASDataPoint> &dv) {
    _plot->appendPoints(dv);
}

void SpecMonBox::replot() {
    _plot->replot();
}

bool SpecMonBox::isActive() {
    return _active;
}
