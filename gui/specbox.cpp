#include <QVBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QComboBox>
#include <QGroupBox>
#include <QRadioButton>
#include "specbox.h"
#include "spectroscatterplot.h"
#include "mddasplotconfig.h"
#include "mddasdatapoint.h"

SpecBox::SpecBox(QWidget *parent):
    QWidget(parent) {

    QVBoxLayout *layout = new QVBoxLayout(this);
    QHBoxLayout *hbox = new QHBoxLayout();

    QWidget *hFiller = new QWidget;
    hFiller->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // _refreshRateSpinBox = new QDoubleSpinBox(this);
    // _refreshRateSpinBox->setDecimals(1);
    // _refreshRateSpinBox->setValue(1.0);
    // _refreshRateSpinBox->setRange(0.1, 60.0);
    // _refreshRateSpinBox->setSingleStep(1.0);

    // _refreshTimer = new QTimer(this);
    // _refreshTimer->setInterval(int(1000.0*_refreshRateSpinBox->value()));

    //_plot = new SpectroMonitorScatterPlot(this, getPlotConfig());
    _plot = new SpectroScatterPlot(this, 1, 1);

    QwtText spec_title("Spectrogram");
    QFont font = _plot->title().font();
    font.setPointSize(8);
    //mon_title.setFont(QFont("Helvetica", 12, -1));
    spec_title.setFont(font);
    _plot->setTitle(spec_title);

    _replotTimer = new QTimer(this);
    _replotTimer->setInterval(200);
    _replotTimer->setSingleShot(false);
    _replotTimer->stop();

    connect(_replotTimer, SIGNAL( timeout() ), _plot, SLOT( replotWithScale() ));
    // connect(_refreshRateSpinBox, SIGNAL( valueChanged(double) ), this, SLOT( setRefreshTimer(double) ));
    // connect(_refreshTimer, SIGNAL( timeout() ), _plot, SLOT( clear() ));

    QGroupBox *cmBox = new QGroupBox(this);
    cmBox->setTitle("Colormap");
    QHBoxLayout *cmBoxLayout = new QHBoxLayout();

    _cmSelector = new QComboBox(this);
    _cmSelector->addItem("gist_earth");
    _cmSelector->addItem("gist_stern");
    _cmSelector->addItem("gist_heat");
    _cmSelector->addItem("rainbow");
    _cmSelector->addItem("gray");
    _cmSelector->addItem("yarg");
    _cmSelector->addItem("spectral");
    _cmSelector->addItem("spectral2");
    _cmSelector->addItem("bone");
    _cmSelector->addItem("Ishihara");
    _cmSelector->addItem("Spectrally Retarded");

    cmBoxLayout->addWidget(_cmSelector);
    cmBox->setLayout(cmBoxLayout);

    QGroupBox *scaleBox = new QGroupBox(this);
    scaleBox->setTitle("Scale");

    QRadioButton *linearButton = new QRadioButton("Linear");
    linearButton->setChecked(true);
    QRadioButton *logButton = new QRadioButton("Log");

    QHBoxLayout *scaleLayout = new QHBoxLayout();
    scaleLayout->addWidget(linearButton);
    scaleLayout->addWidget(logButton);

    scaleBox->setLayout(scaleLayout);

    QGroupBox *rebinBox = new QGroupBox(this);
    rebinBox->setTitle("Rebin Factor");
    
    _rebinSelector = new QComboBox(this);

    QHBoxLayout *rebinLayout = new QHBoxLayout();
    rebinLayout->addWidget(_rebinSelector);

    rebinBox->setLayout(rebinLayout);

    connect(_cmSelector, SIGNAL( currentIndexChanged(int) ), _plot, SLOT( setColorMap(int) ));
    connect(linearButton, SIGNAL( toggled(bool) ), _plot, SLOT( setColorMapMode(bool) ));
    connect(_plot, SIGNAL( divisorsChanged() ), this, SLOT( updateDivisors() ));
    connect(_rebinSelector, SIGNAL( currentIndexChanged(int) ), this, SLOT( doRebin(int) ));
    
    hbox->addWidget(cmBox);
    hbox->addWidget(scaleBox);
    hbox->addWidget(rebinBox);
    hbox->addWidget(hFiller);

    layout->addWidget(_plot);
    layout->addLayout(hbox);
    setLayout(layout);

    _plot->replot();

    //setFixedSize(500,500);
    setMaximumSize(500,500);
}

void SpecBox::activate(bool b) {
    if (b) {
        // Start replot timer
        _replotTimer->start();
        // _refreshTimer->start();
        // _refreshRateSpinBox->setEnabled(false);
    } else {
        // stop replot timer
        _replotTimer->stop();
        // _refreshTimer->stop();
        // _refreshRateSpinBox->setEnabled(true);
    }
    _active = b;
}

// void SpecMonBox::setRefreshTimer(double d) {
//     if (_refreshTimer) {
//         _refreshTimer->setInterval(int(d*1000.0));
//     }
// }

void SpecBox::configure(MDDASPlotConfig pc) {
    //qDebug() << "specbox x: " << pc.getXMax();
    //qDebug() << "specbox y: " << pc.getYMax();
    /* Send plot config to plot */
    _plot->configure(pc);
}

void SpecBox::clear() {
    _plot->clear();
}

void SpecBox::append(const QVector<MDDASDataPoint> &dv) {
    /* This is where the fish lives */
    _plot->appendPoints(dv);
}

void SpecBox::replot() {
    _plot->replotWithScale();
}

bool SpecBox::isActive() {
    return _active;
}

void SpecBox::updateDivisors() {
    int i = 0;
    QVector<uint> divs;

    /* List of possible integer divisors */
    divs = _plot->getRebinDivisors();

    _rebinSelector->clear();

    for (i = 0; i < divs.size(); i++) {
        _rebinSelector->addItem(QString::number(divs.value(i)));
    }
}

/* Command plot to rebin. Note that this is a summing rebin */
void SpecBox::doRebin(int index) {
    //qDebug() << "rebin: " << index << " factor: " << (_rebinSelector->itemText(index)).toUInt();
    _plot->rebin(_rebinSelector->itemText(index).toUInt());
}
