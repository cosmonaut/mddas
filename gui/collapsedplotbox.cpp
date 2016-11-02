#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QGroupBox>

#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QValidator>
#include <QMessageBox>
#include <QComboBox>

#include "collapsedplotbox.h"
#include "specplot.h"
#include "mddasplotconfig.h"
#include "numberbutton.h"


static QVector<uint> get_divisors(uint);
static bool ptwo(uint);


CollapsedPlotBox::CollapsedPlotBox(QMap<QString, QVariant> settings, QWidget *parent) : QWidget(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);
    QHBoxLayout *hbox = new QHBoxLayout();

    _settings = new QMap<QString, QVariant>();
    *_settings = settings;

    QWidget *hFiller = new QWidget;
    hFiller->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    _plot = new SpecPlot(this, 1, 1, 1);

    QwtText plot_title("Plotty-Plot");
    QFont font = _plot->title().font();
    font.setPointSize(8);
    plot_title.setFont(font);
    _plot->setTitle(plot_title);

    _replotTimer = new QTimer(this);
    _replotTimer->setInterval(200);
    _replotTimer->setSingleShot(false);
    _replotTimer->stop();

    rebinBox = new QGroupBox(this);
    rebinBox->setTitle("Rebin Factor");

    _rebinSelector = new QComboBox(this);

    QHBoxLayout *rebinLayout = new QHBoxLayout();
    rebinLayout->addWidget(_rebinSelector);

    rebinBox->setLayout(rebinLayout);
    
    updateDivisors();

    connect(_rebinSelector, SIGNAL( currentIndexChanged(int) ), this, SLOT( doRebin(int) ));
    
    hbox->addWidget(rebinBox);

    connect(_replotTimer, SIGNAL( timeout() ), this, SLOT( replot() ));


    layout->addWidget(_plot);


    layout->addLayout(hbox);
    setLayout(layout);

    _plot->replot();


    setMaximumSize(500, 500);
}



void CollapsedPlotBox::configure(MDDASPlotConfig pc) {
    _plot->clear();
    _plot->configure(pc);


    /* Easy way to manually test box stuff */ 
    _plot->setBox(0,8192,0,8192);
    _plot->setRebinFactor(1);

}

void CollapsedPlotBox::activate(bool b) {
    if (b) {
        rebinBox->setEnabled(false);
        _replotTimer->start();
    } else {
        /* stop replot timer */
        _replotTimer->stop();
        rebinBox->setEnabled(true);
    }
    _active = b;
}


void CollapsedPlotBox::clear() {
    _plot->clear();
}

void CollapsedPlotBox::append(const QVector<MDDASDataPoint> &dv) {
    /* This is where the fish lives */
    _plot->appendPoints(dv);
}

void CollapsedPlotBox::replot() {
    _plot->replot();
}

bool CollapsedPlotBox::isActive() {
    return _active;
}


QMap<QString, QVariant> CollapsedPlotBox::getSettings(void) {
    QMap<QString, QVariant> m;
    m = (*_settings);
    return(m);
}



/* Helper to find all divisors of an axis size */
static QVector<uint> get_divisors(uint n) {
    uint i = 2;
    QVector<uint> divs;

    /* We add 1 as an option to go back to full resolution */
    divs.append(1);

    //while(i < sqrt(n)) {
    while(i < n/2) {
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


/* check if a number is a power of two... */
static bool ptwo(uint num) {
    if (!(num & (num - 1)) && num) {
        return true;
    } else {
        return false;
    }
}

void CollapsedPlotBox::updateDivisors() {
    //TODO: Add call to _plot->setRebinFactor() 

    int i = 0;
    QVector<uint> divs;
    
    divs.append(1);
    divs.append(2);
    divs.append(4);
    divs.append(8);
    divs.append(16);

    /* Must disconnect the combo before we change items */
    disconnect(_rebinSelector, SIGNAL( currentIndexChanged(int) ), this, SLOT( doRebin(int) ));

    _rebinSelector->clear();

    for (i = 0; i < divs.size(); i++) {
        _rebinSelector->addItem(QString::number(divs.value(i)));
    }

    /* Reconnect combobox now that we're done */
    connect(_rebinSelector, SIGNAL( currentIndexChanged(int) ), this, SLOT( doRebin(int) ));
    qDebug() << "update divs success";
    
    divs.clear();
}


