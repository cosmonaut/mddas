#include <QButtonGroup>
#include <QPushButton>
#include <QCheckBox>
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



    boxBox = new QGroupBox(this);
    boxBox->setTitle("Box Select");

    QHBoxLayout *boxLayout = new QHBoxLayout();

    boxButtonGroup = new QButtonGroup(this);

    QPushButton* box1Button = new QPushButton("1" , this);
    QPushButton* box2Button = new QPushButton("2" , this);
    QPushButton* box3Button = new QPushButton("3" , this);
    //QPushButton* zoomButton = new QPushButton("Zoom" , this);

    box1Button->setCheckable(true);
    box2Button->setCheckable(true);
    box3Button->setCheckable(true);

    boxLayout->addWidget(box1Button);
    boxLayout->addWidget(box2Button);
    boxLayout->addWidget(box3Button);
    //boxLayout->addWidget(zoomButton);

    boxButtonGroup->addButton(box1Button, 1);
    boxButtonGroup->addButton(box2Button, 2);
    boxButtonGroup->addButton(box3Button, 3);
        
    boxButtonGroup->setExclusive(true);

    //boxBox->addWidget(boxButtonGroup);
    boxBox->setLayout(boxLayout);
    
    QHBoxLayout *boxHLayout = new QHBoxLayout();
    boxHLayout->addWidget(boxBox);

    QWidget *boxFiller = new QWidget;
    boxFiller->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    boxHLayout->addWidget(boxFiller);

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
    layout->addLayout(boxHLayout);
    setLayout(layout);

    connect(_plot, SIGNAL( boxpos(uint, uint, uint) ), this, SLOT( boxChanged(uint, uint, uint) ));
    connect(boxButtonGroup, SIGNAL( buttonPressed(int) ), this, SLOT( boxButtonPressed(int) ));

    _plot->replot();

    //setFixedSize(500,500);
    setMaximumSize(500,500);
}

void SpecBox::activate(bool b) {
    if (b) {
        boxChanged(0, 0, 0);
        boxBox->setEnabled(false);

        // Start replot timer
        _replotTimer->start();
    } else {
        boxBox->setEnabled(true);
        // stop replot timer
        _replotTimer->stop();
    }
    _active = b;
}

void SpecBox::boxChanged(uint box, uint x, uint y) {
    if (box == 0) {
        /* turn off pickers */
        _plot->setPicker(0);

        boxButtonGroup->setExclusive(false);
        if (boxButtonGroup->checkedId() != -1) {
            boxButtonGroup->checkedButton()->setChecked(false);
        }
        boxButtonGroup->setExclusive(true);

        return;
    }

    /* emit box setting for specplotbox */
    emit setBoxXY(box, x, y);

    boxButtonGroup->setExclusive(false);
    if (boxButtonGroup->checkedId() != -1) {
        boxButtonGroup->checkedButton()->setChecked(false);
    }
    boxButtonGroup->setExclusive(true);

}

void SpecBox::boxButtonPressed(int id) {
    _plot->setPicker((uint)id);
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

    /* Must disconnect the combo before we change items */
    disconnect(_rebinSelector, SIGNAL( currentIndexChanged(int) ), this, SLOT( doRebin(int) ));

    _rebinSelector->clear();

    for (i = 0; i < divs.size(); i++) {
        _rebinSelector->addItem(QString::number(divs.value(i)));
    }

    /* Reconnect combobox now that we're done */
    connect(_rebinSelector, SIGNAL( currentIndexChanged(int) ), this, SLOT( doRebin(int) ));
}

/* Command plot to rebin. Note that this is a summing rebin */
void SpecBox::doRebin(int index) {
    //qDebug() << "rebin: " << index << " factor: " << (_rebinSelector->itemText(index)).toUInt();
    _plot->rebin(_rebinSelector->itemText(index).toUInt());
}

void SpecBox::setBox1(uint x_min, uint x_max, uint y_min, uint y_max) {
    //qDebug() << "setting box 1!";
    _plot->setBox1(x_min, x_max, y_min, y_max);
}

void SpecBox::setBox2(uint x_min, uint x_max, uint y_min, uint y_max) {
    //qDebug() << "setting box 2!";
    _plot->setBox2(x_min, x_max, y_min, y_max);
}

void SpecBox::setBox3(uint x_min, uint x_max, uint y_min, uint y_max) {
    //qDebug() << "setting box 3!";
    _plot->setBox3(x_min, x_max, y_min, y_max);
}
