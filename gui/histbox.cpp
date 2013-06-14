#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QGroupBox>

#include "histbox.h"
#include "incrementalhistplot.h"
#include "mddasplotconfig.h"
#include "numberbutton.h"

HistBox::HistBox(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);
    QHBoxLayout *hbox = new QHBoxLayout();

    QWidget *hFiller = new QWidget;
    hFiller->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    _plot = new IncrementalHistPlot(this, 1, 1, 1);

    QwtText hist_title("Pulse Height");
    QFont font = _plot->title().font();
    font.setPointSize(8);
    hist_title.setFont(font);
    _plot->setTitle(hist_title);

    _replotTimer = new QTimer(this);
    _replotTimer->setInterval(200);
    _replotTimer->setSingleShot(false);
    _replotTimer->stop();

    QHBoxLayout *binsLayout = new QHBoxLayout();

    _binsBox = new QGroupBox(this);
    _binsBox->setTitle("Binning");

    _numBins = new NumberButton(_binsBox, "Bins", QString::null, 0, 100000, 1);
    _numBins->setValue(1);
    _numBins->setChecked(true);
    _binSize = new NumberButton(_binsBox, "Bin Size", QString::null, 0, 100000, 1);
    _binSize->setValue(1);
    _binSize->setEnabled(false);

    connect(_numBins, SIGNAL( toggled(bool) ), _binSize, SLOT( setUnchecked(bool) ));
    connect(_binSize, SIGNAL( toggled(bool) ), _numBins, SLOT( setUnchecked(bool) ));
    connect(_numBins, SIGNAL( toggled(bool) ), _binSize, SLOT( setAntiEnabled(bool) ));
    connect(_binSize, SIGNAL( toggled(bool) ), _numBins, SLOT( setAntiEnabled(bool) ));

    connect(_plot, SIGNAL( numBinsChanged(int) ), _numBins, SLOT( setValue(int) ));
    connect(_plot, SIGNAL( binSizeChanged(int) ), _binSize, SLOT( setValue(int) ));
    
    binsLayout->addWidget(_numBins);
    binsLayout->addWidget(_binSize);
    //binsLayout->addWidget(hFiller);

    _binsBox->setLayout(binsLayout);
    hbox->addWidget(_binsBox);
    hbox->addWidget(hFiller);

    connect(_replotTimer, SIGNAL( timeout() ), this, SLOT( replot() ));

    layout->addWidget(_plot);
    layout->addLayout(hbox);
    setLayout(layout);

    _plot->replot();

    setMaximumSize(500, 500);
}

void HistBox::configure(MDDASPlotConfig pc) {
    _plot->clear();
    _plot->configure(pc);

    _numBins->setValue(pc.getPMax());
    _binSize->setValue(1);
}

void HistBox::activate(bool b) {
    if (b) {
        if (_numBins->isChecked()) {
            _plot->setNumBins(_numBins->value());
        } else {
            _plot->setBinSize(_binSize->value());
        }
        _binsBox->setEnabled(false);
        _replotTimer->start();
    } else {
        /* stop replot timer */
        _replotTimer->stop();
        _binsBox->setEnabled(true);
    }
    _active = b;
}

void HistBox::clear() {
    _plot->clear();
}


void HistBox::append(const QVector<MDDASDataPoint> &dv) {
    /* This is where the fish lives */
    _plot->appendPoints(dv);
}

void HistBox::replot() {
    _plot->replot();
}

bool HistBox::isActive() {
    return _active;
}

